#include "config.h"

#include "posix.h"
#include "daemon.h"
#include "logger.h"
#include "logger_file.h"
#include "openvpn.h"
#include "path.h"

#include <signal.h>
#include <exception>
#include <stdexcept>
#include <cxxabi.h>
#include <unistd.h>


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
	Result _result;
	bool _exited;
public:
	PosixSubprocess(asio::io_service& io)
		: _io(io)
		, stdin_handle(io)
		, stdout_handle(io)
		, stderr_handle(io)
		, _pid(0)
		, _exited(false)
	{

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
	void Kill(int signal = SIGTERM)
	{
		if (_pid)
		{
			kill(_pid, signal);
		}
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
		return std::move(result);
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
		}
	}
};

class PosixOpenVPNProcess : public OpenVPNProcess
{
	FILE* _file;

public:
	PosixOpenVPNProcess(asio::io_service& io) : OpenVPNProcess(io), _file(nullptr) {}
	~PosixOpenVPNProcess()
	{
		Kill();
	}

	virtual void Run(const std::vector<std::string>& params) override
	{
		std::string cmdline = GetPath(OpenVPNExecutable);
		for (const auto& param : params)
		{
			cmdline += ' ';
			// FIXME: proper quoting/escaping
			if (param.find(' ') != param.npos)
				cmdline += "\"" + param + "\"";
			else
				cmdline += param;
		}
		LOG(INFO) << cmdline;
		// execute cmdline asynchronously - popen?
		_file = popen(cmdline.c_str(), "w"); // TODO: later, if parsing output, use "r" instead
	}

	virtual void Kill() override
	{
		if (_file)
		if (!error)
		{
			pclose(_file);
			_file = nullptr;
		}
	}
};

class PosixCypherDaemon : public CypherDaemon
{
public:
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
};

void sigterm_handler(int signal)
{
	if (g_daemon)
		g_daemon->RequestShutdown();
}

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
	g_old_terminate_handler = std::set_terminate(terminate_handler);

	InitPaths(argc > 0 ? argv[0] : "cypherpunkvpn-service");

	g_file_logger.Open(GetPath(LogDir, "daemon.log"));
	Logger::Push(&g_file_logger);
	Logger::Push(&g_stderr_logger);

	// Register a signal handler for SIGTERM for clean shutdowns
	signal(SIGTERM, sigterm_handler);

	// Instantiate the posix version of the daemon
	g_daemon = new PosixCypherDaemon();

	// Run the daemon synchronously
	int result = g_daemon->Run();

	if (result)
		LOG(ERROR) << "Exited daemon with error code " << result;
	else
		LOG(INFO) << "Exited daemon successfully";
}
