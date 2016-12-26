#include "config.h"

#include "posix.h"
#include "daemon.h"
#include "logger.h"
#include "logger_file.h"
#include "openvpn.h"
#include "path.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <stdexcept>

#include <cxxabi.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef OS_LINUX
#include <sys/types.h>
#include <sys/wait.h>
#endif


void pfctl_install();
void pfctl_uninstall();
//std::string pfctl_enable();
//void pfctl_disable(const std::string& token);
void pfctl_ensure_enabled();
void pfctl_ensure_disabled();
void pfctl_enable_anchor(const std::string& anchor);
void pfctl_disable_anchor(const std::string& anchor);
bool pfctl_anchor_enabled(const std::string& anchor);
void pfctl_set_anchor_enabled(const std::string& anchor, bool enable);


class PosixHandle
{
	int _fd;
public:
	PosixHandle() : _fd(0) {}
	explicit PosixHandle(int fd) : _fd(fd) {}
	PosixHandle(const PosixHandle& copy) : _fd(copy._fd ? POSIX_CHECK(dup, (copy._fd)) : 0) {}
	PosixHandle(PosixHandle&& move) : _fd(std::exchange(move._fd, 0)) {}
	~PosixHandle()
	{
		Close();
	}
	PosixHandle& operator=(const PosixHandle& copy)
	{
		Close();
		_fd = copy._fd ? POSIX_CHECK(dup, (copy._fd)) : 0;
		return *this;
	}
	PosixHandle& operator=(PosixHandle&& move)
	{
		Close();
		_fd = std::exchange(move._fd, 0);
		return *this;
	}
	PosixHandle Duplicate()
	{
		return PosixHandle(_fd ? POSIX_CHECK(dup, (_fd)) : 0);
	}
	void SetCloseOnExec()
	{
		POSIX_CHECK(fcntl, (_fd, F_SETFD, POSIX_CHECK(fcntl, (_fd, F_GETFD)) | FD_CLOEXEC));
	}
	void Close()
	{
		if (_fd)
		{
			close(_fd);
			_fd = 0;
		}
	}
	int Release()
	{
		return std::exchange(_fd, 0);
	}
	operator int() const { return _fd; }
};

namespace impl {
	struct PosixPipeWrapper
	{
		PosixHandle read;
		PosixHandle write;
		operator int*() { return reinterpret_cast<int*>(this); }
	};
}

class PosixPipe : public impl::PosixPipeWrapper
{
	static_assert(offsetof(impl::PosixPipeWrapper, read) == 0 && offsetof(impl::PosixPipeWrapper, write) == sizeof(int), "PosixPipe struct offset mismatch");
	static_assert(sizeof(impl::PosixPipeWrapper) == sizeof(int[2]), "PosixPipe struct size mismatch");
public:
	PosixPipe()
	{
		POSIX_CHECK(pipe, (*this));
	}
	PosixPipe(PosixHandle read, PosixHandle write)
	{
		this->read = std::move(read);
		this->write = std::move(write);
	}
	explicit PosixPipe(int read, int write)
	{
		this->read = PosixHandle(read);
		this->write = PosixHandle(write);
	}
	~PosixPipe() {}
	void SetCloseOnExec()
	{
		read.SetCloseOnExec();
		write.SetCloseOnExec();
	}
	void Close()
	{
		read.Close();
		write.Close();
	}
	void Release()
	{
		read.Release();
		write.Release();
	}
};

class PosixSubprocess
{
	asio::io_service& _io;
	pid_t _pid;
public:
	asio::posix::stream_descriptor stdin_handle;
	asio::posix::stream_descriptor stdout_handle;
	asio::posix::stream_descriptor stderr_handle;

	class Result
	{
		int _result;
	public:
		Result() {}
		explicit Result(int result) : _result(result) {}

		bool exited() const { return WIFEXITED(_result); }
		int status() const { return WEXITSTATUS(_result); }
		bool signaled() const { return WIFSIGNALED(_result); }
		int signal() const { return WTERMSIG(_result); }
		bool stopped() const { return WIFSTOPPED(_result); }
		int stopsignal() const { return WSTOPSIG(_result); }
		bool continued() const { return WIFCONTINUED(_result); }
	};
private:
	static std::unordered_map<pid_t, PosixSubprocess*> _map;
	std::vector<std::function<void(const asio::error_code&, Result)>> _waiters;
	Result _result;
	bool _exited;
public:
	PosixSubprocess(asio::io_service& io)
		: _io(io)
		, _pid(0)
		, stdin_handle(io)
		, stdout_handle(io)
		, stderr_handle(io)
		, _exited(false)
	{

	}
	~PosixSubprocess()
	{
		if (_pid) _map.erase(_pid);
	}
	int pid() const { return _pid; }
	// Run("/usr/local/bin/myprogrem", { "arg1", "arg2" })
	void Run(const std::string& executable, const std::vector<std::string>& args)
	{
		Run(executable.c_str(), MakeArgVector(executable, args).data(), NULL);
	}
	// Run("/usr/local/bin/myprogrem", { "arg1", "arg2" }, { "ENV1=value1", "ENV2=value2" })
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::vector<std::string>& env)
	{
		std::vector<const char*> c_env(env.size() + 1, NULL);
		std::transform(env.begin(), env.end(), c_env.begin(), [](const std::string& s) { return s.c_str(); });
		Run(executable.c_str(), MakeArgVector(executable, args).data(), c_env.data());
	}
	// Run("/usr/local/bin/myprogrem", { "arg1", "arg2" }, { { "ENV1", "value1" }, { "ENV2", "value2" } })
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::unordered_map<std::string, std::string>& env)
	{
		std::vector<std::string> env2;
		env2.reserve(env.size());
		for (const auto& kvp : env)
			env2.push_back(kvp.first + "=" + kvp.second);
		Run(executable, args, env2);
	}
	Result Wait()
	{
		if (!_pid)
			THROW_POSIXEXCEPTION(ECHILD, waitpid);
		if (_exited)
			return _result;
		int result_value;
		while (waitpid(_pid, &result_value, 0) == -1)
		{
			if (errno != EINTR)
				THROW_POSIXEXCEPTION(errno, waitpid);
		}
		_result = Result(result_value);
		_exited = true;
		return _result;
	}
	bool Check(Result* result)
	{
		if (!_pid)
			THROW_POSIXEXCEPTION(ECHILD, waitpid);
		if (_exited)
		{
			if (result)
				*result = _result;
			return true; 
		}	
		int result_value;
		int pid;
		while ((pid = waitpid(_pid, &result_value, WNOHANG)) == -1)
		{
			if (errno != EINTR)
				THROW_POSIXEXCEPTION(errno, waitpid);
		}
		if (pid == 0)
			return false;
		if (result)
			*result = Result(result_value);
		return true;
	}
	void AsyncWait(std::function<void(const asio::error_code&, Result)> cb)
	{
		LOG(INFO) << "_pid = " << _pid << ", _exited = " << _exited;
		if (!_pid)
			_io.post([cb = std::move(cb)]() { cb(asio::error::bad_descriptor, Result(0)); });
		else if (_exited)
			_io.post([result = _result, cb = std::move(cb)]() { cb(asio::error_code(), result); });
		else
			_waiters.push_back(std::move(cb));
	}
	void Kill(int signal = SIGTERM)
	{
		if (_pid)
		{
			kill(_pid, signal);
		}
	}

public:
	static void NotifyTerminated(pid_t pid, int result)
	{
		auto it = _map.find(pid);
		if (it != _map.end())
		{
			auto instance = it->second;
			_map.erase(it);
			instance->NotifyTerminated(result);
		}
		else
			LOG(WARNING) << "Unknown child " << pid << " terminated with status " << result;
	}
	void NotifyTerminated(int result)
	{
		LOG(INFO) << "Subprocess " << _pid << " terminated with status " << result;
		_result = Result(result);
		_exited = true;
		_io.dispatch([cbs = std::move(_waiters), result = _result]() {
			for (const auto& cb : cbs)
				cb(asio::error_code(), result);
		});
	}

private:
	static std::vector<const char*> MakeArgVector(const std::string& executable, const std::vector<std::string>& args)
	{
		LOG(INFO) << "MakeArgVector for " << executable << " called with " << args.size() << " args";

		std::vector<const char*> result;
		result.reserve(args.size() + 2);
		size_t last_slash = executable.find_last_of('/');
		if (last_slash != executable.npos)
			result.push_back(&executable[0] + last_slash + 1);
		else
			result.push_back(executable.c_str());
		for (const auto& arg : args)
			result.push_back(arg.c_str());
		result.push_back(NULL);
		return result;
	}
	void Run(const char* executable, const char* const* args, const char* const* env)
	{
		PosixPipe stdin_pipe;
		PosixPipe stdout_pipe;
		PosixPipe stderr_pipe;
		PosixPipe status_pipe;

		// This makes use of a common pipe trick to detect the status of the
		// launched subprocess; a successful exec will close the stream, whereas
		// on failure the child will write an error code back to the parent.
		status_pipe.write.SetCloseOnExec();

		pid_t pid = POSIX_CHECK(fork, ());
		if (pid == 0) // Child
		{
			try
			{
				// Redirect stdin to the read end of stdin_pipe and close the other handles.
				POSIX_CHECK(dup2, (stdin_pipe.read, STDIN_FILENO));
				stdin_pipe.Close();

				// Redirect stdout to the write end of stdout_pipe and close the other handles.
				POSIX_CHECK(dup2, (stdout_pipe.write, STDOUT_FILENO));
				stdout_pipe.Close();

				// Redirect stderr to the write end of stdout_pipe and close the other handles.
				POSIX_CHECK(dup2, (stderr_pipe.write, STDERR_FILENO));
				stderr_pipe.Close();

				// Close unused read end of status pipe.
				status_pipe.read.Close();

				// TODO: Maybe drop root privileges (initgroups+setgid+setuid), if possible?
				// Probably root is required for the up/down scripts though, so can't be done
				// until that is refactored to be actually handled by parent process.

				if (env)
					POSIX_CHECK(execve, (executable, const_cast<char* const*>(args), const_cast<char* const*>(env)));
				else
					POSIX_CHECK(execv, (executable, const_cast<char* const*>(args)));

				// The exec function only returns on error, so the check above should already
				// catch any problem, but put a catchall here anyway.
				THROW_POSIXEXCEPTION(errno, exec);
			}
			catch (const PosixException& e)
			{
				int err = e.value();
				write(status_pipe.write, &err, sizeof(int));
				// If this fails there is very little we can do; the parent process will
				// misinterpret the closed pipe as a successful exec, but can at least
				// later see something wrong with the exit code (below).
			}
			_exit(9000);
		}
		else // Parent
		{
			// Close unused ends of standard stream pipes.
			stdin_pipe.read.Close();
			stdout_pipe.write.Close();
			stderr_pipe.write.Close();

			// Close write end of status pipe.
			status_pipe.write.Close();

			// Spin-read the status pipe until we get something (data or EOF).
			int count, err;
			do {
				count = read(status_pipe.read, &err, sizeof(int));
			} while (count == -1 && (errno == EAGAIN || errno == EINTR));

			// If we got a status, there was a launch error.
			if (count)
			{
				THROW_POSIXEXCEPTION(err, exec);
			}

			// Otherwise, all is well and the exec completed successfully.

			// Explicitly close the status pipe for clarity.
			status_pipe.read.Close();
			
			// Transfer the standard stream pipe handles so they stay open.
			stdin_handle.assign(stdin_pipe.write.Release());
			stdout_handle.assign(stdout_pipe.read.Release());
			stderr_handle.assign(stderr_pipe.read.Release());

			// Keep the PID around.
			_pid = pid;

			// Register in the PID map.
			_map[pid] = this;
		}
	}
};

std::unordered_map<pid_t, PosixSubprocess*> PosixSubprocess::_map;

template<class AsyncReadStream>
class ASIOLineReader
{
public:
	typedef void callback_t(const asio::error_code& error, std::string line);
private:
	AsyncReadStream& _stream;
	asio::streambuf _readbuf;
	std::function<callback_t> _callback;
public:
	ASIOLineReader(AsyncReadStream& stream, std::function<callback_t> callback)
		: _stream(stream), _callback(std::move(callback))
	{
	}
	void Begin()
	{
		AsyncReadLine();
	}
private:
	void AsyncReadLine()
	{
		asio::async_read_until(_stream, _readbuf, '\n', THIS_CALLBACK(HandleLine));
	}
	void HandleLine(const asio::error_code& error, std::size_t bytes_transferred)
	{
		if (!error)
		{
			std::istream is(&_readbuf);
			std::string line;
			std::getline(is, line);
			if (line.size() > 0 && *line.crbegin() == '\r')
				line.pop_back();
			_callback(error, std::move(line));
			AsyncReadLine();
		}
		else
		{
			_callback(error, std::string());
		}
	}
};


class PosixOpenVPNProcess : public OpenVPNProcess, public PosixSubprocess
{
	std::deque<std::string> _stdin_write_queue;
	ASIOLineReader<decltype(stdout_handle)> _stdout_reader;
	ASIOLineReader<decltype(stderr_handle)> _stderr_reader;
	using OpenVPNProcess::_io;
public:
	PosixOpenVPNProcess(asio::io_service& io)
		: OpenVPNProcess(io), PosixSubprocess(io)
		, _stdout_reader(stdout_handle, [this](const asio::error_code& error, std::string line) { g_daemon->OnOpenVPNStdOut(this, error, std::move(line)); })
		, _stderr_reader(stderr_handle, [this](const asio::error_code& error, std::string line) { g_daemon->OnOpenVPNStdErr(this, error, std::move(line)); })
	{

	}
	~PosixOpenVPNProcess()
	{
		stdin_handle.close();
		stdout_handle.close();
		stderr_handle.close();
		Kill();
	}

	virtual void Run(const std::vector<std::string>& params) override
	{
		std::string openvpn = GetPath(OpenVPNExecutable);
		PosixSubprocess::Run(openvpn, params);

		_stdout_reader.Begin();
		_stderr_reader.Begin();
	}

	virtual void Kill() override
	{
		PosixSubprocess::Kill();
	}

	virtual void AsyncWait(std::function<void(const asio::error_code& error)> cb) override
	{
		PosixSubprocess::AsyncWait([cb = std::move(cb)](const asio::error_code& error, PosixSubprocess::Result result) { cb(error); });
	}

	void WriteLineToStdIn(std::string line)
	{
		WriteToStdIn(std::move(line) + '\n');
	}
	void WriteToStdIn(std::string data)
	{
		_io.dispatch([this, d = std::move(data)]() {
			bool was_empty = _stdin_write_queue.empty();
			_stdin_write_queue.push_back(std::move(d));
			if (was_empty)
				asio::async_write(stdin_handle, asio::buffer(_stdin_write_queue.front()), THIS_CALLBACK(HandleLineWritten));
		});
	}
private:
	void HandleLineWritten(const asio::error_code& error, std::size_t bytes_transferred)
	{
		if (!error)
		{
			_stdin_write_queue.pop_front();
			if (!_stdin_write_queue.empty())
				asio::async_write(stdin_handle, asio::buffer(_stdin_write_queue.front()), THIS_CALLBACK(HandleLineWritten));
		}
	}
};

class PosixCypherDaemon : public CypherDaemon
{
	asio::signal_set _signals;
	std::unordered_map<pid_t, PosixOpenVPNProcess*> _process_map;
public:
	PosixCypherDaemon()
		: _signals(_io, SIGCHLD, SIGPIPE, SIGTERM)
	{
		// Register signal listeners
		_signals.async_wait(THIS_CALLBACK(OnSignal));

		pfctl_install();
	}
	virtual OpenVPNProcess* CreateOpenVPNProcess(asio::io_service& io) override
	{
		return new PosixOpenVPNProcess(io);
	}
	virtual int GetAvailablePort(int hint) override
	{
		return hint;
		//popen("netstat -a -n");
		for (int i = hint; ; i++)
		{
			if (1) // /* port i not in use */
				return i;
		}
		throw "unable to find an available port";
	}
	virtual std::string GetAvailableAdapter(int index) override
	{
		throw "not implemented";
	}
	virtual void ApplyFirewallSettings() override
	{
		bool is_connected;
		switch (_state)
		{
		case CONNECTING:
		case CONNECTED:
		case DISCONNECTING:
		case SWITCHING:
			is_connected = true;
			break;
		default:
			is_connected = false;
			break;
		}

		auto mode = g_settings.firewall();
		if (!_connections.empty() && (mode == "on" || (is_connected && mode == "auto")))
		{
			pfctl_set_anchor_enabled("100.killswitch", true);
			pfctl_set_anchor_enabled("200.exemptLAN", g_settings.allowLAN());
			pfctl_ensure_enabled();
		}
		else
		{
			pfctl_ensure_disabled();
			pfctl_set_anchor_enabled("100.killswitch", false);
			pfctl_set_anchor_enabled("200.exemptLAN", false);
		}
	}
	void OnSignal(const asio::error_code& error, int signal)
	{
		if (!error)
		{
			switch (signal)
			{
				case SIGCHLD:
				{
					pid_t pid;
					int status;
					while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
					{
						PosixSubprocess::NotifyTerminated(pid, status);
					}
					break;
				}
				case SIGTERM:
				{
					RequestShutdown();
					break;
				}
			}
			_signals.async_wait(THIS_CALLBACK(OnSignal));
		}
	}
	bool OnOpenVPNCallback_Up(OpenVPNProcess* process, const std::deque<std::string>& argv)
	{
		return true;
	}
	bool OnOpenVPNCallback_Down(OpenVPNProcess* process, const std::deque<std::string>& argv)
	{
		return true;
	}
	bool OnOpenVPNCallback_RoutePreDown(OpenVPNProcess* process, const std::deque<std::string>& argv)
	{
		return true;
	}
	bool OnOpenVPNCallback_TLSVerify(OpenVPNProcess* process, const std::deque<std::string>& argv)
	{
		return true;
	}
	bool OnOpenVPNCallback_IPChange(OpenVPNProcess* process, const std::deque<std::string>& argv)
	{
		return true;
	}
	bool OnOpenVPNCallback_RouteUp(OpenVPNProcess* process, const std::deque<std::string>& argv)
	{
		return true;
	}
	void OnOpenVPNCallback(OpenVPNProcess* process, std::string args) override
	{
		CypherDaemon::OnOpenVPNCallback(process, args);
		std::deque<std::string> argv = SplitToDeque(args, ' ');
		if (argv.size() < 2)
		{
			LOG(ERROR) << "Malformed OpenVPN callback";
		}
		else
		{
			try
			{
				int pid = std::stoi(argv.at(0));
				std::string cmd = std::move(argv.at(1));
				std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return std::tolower(c); });
				argv.pop_front();
				argv.pop_front();
				bool success;
				try
				{
					if (cmd == "up")
						success = OnOpenVPNCallback_Up(process, argv);
					else if (cmd == "down")
						success = OnOpenVPNCallback_Down(process, argv);
					else if (cmd == "route-pre-down")
						success = OnOpenVPNCallback_RoutePreDown(process, argv);
					else if (cmd == "tls-verify")
						success = OnOpenVPNCallback_TLSVerify(process, argv);
					else if (cmd == "ipchange")
						success = OnOpenVPNCallback_IPChange(process, argv);
					else if (cmd == "route-up")
						success = OnOpenVPNCallback_RouteUp(process, argv);
					else
					{
						LOG(ERROR) << "Unrecognized OpenVPN callback: " << cmd;
						success = false;
					}
				}
				catch (...)
				{
					LOG(ERROR) << "Exception during OpenVPN callback";
					success = false;
				}
				// Send result back to OpenVPN by killing placeholder script
				if (success)
					kill(pid, SIGTERM);
				else
					kill(pid, SIGINT);
			}
			catch (...)
			{
				LOG(ERROR) << "Malformed OpenVPN callback";
			}
		}
	}
};

static void (*g_old_terminate_handler)() = nullptr;

void terminate_handler()
{
	std::set_terminate(g_old_terminate_handler);
	LOG(CRITICAL) << "Exiting due to unhandled exception";
	if (std::exception_ptr ep = std::current_exception())
	{
		try
		{
			std::rethrow_exception(ep);
		}
		catch (const std::exception& e)
		{
			LOG(CRITICAL) << "what() = \"" << e.what() << "\"";
		}
		catch (...)
		{
			if (std::type_info* et = abi::__cxa_current_exception_type())
				LOG(CRITICAL) << "Exception type: " << et->name();
		}
	}
	std::abort();
}

static FileLogger g_stdout_logger(stdout);
static FileLogger g_stderr_logger(stderr);
static FileLogger g_file_logger;

int main(int argc, char **argv)
{
	// First, make sure we're running as root:cypherpunk
	if (geteuid() != 0)
	{
		int uid = geteuid();
		struct passwd* pw = getpwuid(uid);
		fprintf(stderr, "Error: running as user %s (%d), must be root.\n", pw && pw->pw_name ? pw->pw_name : "<unknown uid>", uid);
		return 1;
	}
#ifdef OS_OSX
	struct group* gr = getgrnam("cypherpunk");
	if (!gr)
	{
		fprintf(stderr, "Error: group cypherpunk does not exist\n");
		return 2;
	}
	if (setegid(gr->gr_gid) == -1 || setgid(gr->gr_gid) == -1)
	{
		fprintf(stderr, "Error: failed to set group id to %d with error %d\n", gr->gr_gid, errno);
		return 3;
	}
#endif

	g_old_terminate_handler = std::set_terminate(terminate_handler);

	InitPaths(argc > 0 ? argv[0] : "cypherpunk-privacy-service");

	g_file_logger.Open(GetPath(LogDir, "daemon.log"));
	Logger::Push(&g_file_logger);
	Logger::Push(&g_stderr_logger);

	// Instantiate the posix version of the daemon
	g_daemon = new PosixCypherDaemon();

	// Run the daemon synchronously
	int result = g_daemon->Run();

	// Extreme corner case of children dying after our sighandlers..?
	signal(SIGCHLD, SIG_IGN);

	if (result)
		LOG(ERROR) << "Exited daemon with error code " << result;
	else
		LOG(INFO) << "Exited daemon successfully";
}
