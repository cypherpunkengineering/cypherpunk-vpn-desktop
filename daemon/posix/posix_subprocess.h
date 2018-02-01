#pragma once

#include "config.h"

#include "subprocess.h"

#include "posix.h"

#include <sys/types.h>
#include <sys/wait.h>


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

template<class AsyncReadStream>
class ASIOLineReader : public std::enable_shared_from_this<ASIOLineReader<AsyncReadStream>>
{
public:
	typedef void callback_t(const asio::error_code& error, std::string line);
private:
	std::shared_ptr<AsyncReadStream> _stream;
	asio::streambuf _readbuf;
	std::function<callback_t> _callback;
public:
	void Run(std::shared_ptr<AsyncReadStream> stream, std::function<callback_t> callback)
	{
		_stream = std::move(stream);
		_callback = std::move(callback);
		AsyncReadLine();
	}
private:
	void AsyncReadLine()
	{
		asio::async_read_until(*_stream, _readbuf, '\n', SHARED_CALLBACK(HandleLine));
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
			_stream.reset();
			_callback = nullptr;
		}
	}
};

template<class AsyncReadStream>
void AsyncReadAllLines(std::shared_ptr<AsyncReadStream> stream, std::function<void(const asio::error_code& error, std::string line)> callback)
{
	auto reader = std::make_shared<ASIOLineReader<AsyncReadStream>>();
	reader->Run(std::move(stream), std::move(callback));
}

class PosixSubprocess : public Subprocess
{
	static std::unordered_map<pid_t, PosixSubprocess*> _map;

	pid_t _pid;
	std::vector<std::function<void(const asio::error_code&, Result)>> _waiters;
	bool _exited;

public:
	asio::posix::stream_descriptor stdin_handle;
	asio::posix::stream_descriptor stdout_handle;
	asio::posix::stream_descriptor stderr_handle;

private:
	std::deque<std::string> _stdin_write_queue;

private:
	static Result MakeResult(int code)
	{
		if (WIFEXITED(code)) return WEXITSTATUS(code);
		if (WIFSIGNALED(code)) return -WTERMSIG(code);
		if (WIFSTOPPED(code)) return Stopped;
		if (WIFCONTINUED(code)) return Continued;
		// Unreachable
		return OtherError;
	}

public:
	PosixSubprocess(asio::io_service& io)
		: Subprocess(io)
		, _pid(0)
		, stdin_handle(io)
		, stdout_handle(io)
		, stderr_handle(io)
		, _exited(false)
	{

	}
	~PosixSubprocess()
	{
		stdin_handle.close();
		stdout_handle.close();
		stderr_handle.close();
		Kill();
		if (_pid) _map.erase(_pid);
	}
	int pid() const { return _pid; }

protected:
	virtual void DoRun(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd, const std::unordered_map<std::string, std::string>& env, unsigned int flags) override
	{
		if (env.size() > 0)
		{
			std::vector<std::string> env_pairs;
			env_pairs.reserve(env.size());
			for (const auto& kvp : env)
				env_pairs.push_back(kvp.first + "=" + kvp.second);
			std::vector<const char*> env_ptrs(env.size() + 1, NULL);
			std::transform(env_pairs.begin(), env_pairs.end(), env_ptrs.begin(), [](const std::string& s) { return s.c_str(); });

			RunInternal(executable.c_str(), MakeArgVector(executable, args).data(), cwd.empty() ? NULL : cwd.c_str(), env_ptrs.data(), flags);
		}
		else
		{
			RunInternal(executable.c_str(), MakeArgVector(executable, args).data(), cwd.empty() ? NULL : cwd.c_str(), NULL, flags);
		}
		AsyncReadAllLines(SHARED_MEMBER(stdout_handle), SHARED_CALLBACK(OnStdOut));
		AsyncReadAllLines(SHARED_MEMBER(stderr_handle), SHARED_CALLBACK(OnStdErr));
	}

private:
	void HandleStdInWritten(const asio::error_code& error, std::size_t bytes_transferred)
	{
		if (!error)
		{
			_stdin_write_queue.pop_front();
			if (!_stdin_write_queue.empty())
				asio::async_write(stdin_handle, asio::buffer(_stdin_write_queue.front()), SHARED_CALLBACK(HandleStdInWritten));
		}
	}
	void OnStdOut(const asio::error_code& error, std::string line)
	{
		if (_on_stdout_line) _on_stdout_line(error, std::move(line));
	}
	void OnStdErr(const asio::error_code& error, std::string line)
	{
		if (_on_stderr_line) _on_stderr_line(error, std::move(line));
	}

public:
	virtual void WriteToStdIn(std::string data) override
	{
		_io.dispatch([this, d = std::move(data)]() {
			bool was_empty = _stdin_write_queue.empty();
			_stdin_write_queue.push_back(std::move(d));
			if (was_empty)
				asio::async_write(stdin_handle, asio::buffer(_stdin_write_queue.front()), SHARED_CALLBACK(HandleStdInWritten));
		});
	}

	virtual Result Wait() override
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

	virtual void AsyncWait(std::function<void(const asio::error_code&, Result)> cb) override
	{
		if (!_pid)
			_io.post([cb = std::move(cb)]() { cb(asio::error::bad_descriptor, Result(0)); });
		else if (_exited)
			_io.post([result = _result, cb = std::move(cb)]() { cb(asio::error_code(), result); });
		else
			_waiters.push_back(std::move(cb));
	}
	void Signal(int signal = SIGTERM)
	{
		if (_pid)
		{
			kill(_pid, signal);
		}
	}
	virtual void Kill(bool force = false) override
	{
		Signal(force ? SIGKILL : SIGTERM);
	}

public:
	static void NotifyTerminated(pid_t pid, int code)
	{
		auto it = _map.find(pid);
		if (it != _map.end())
		{
			auto instance = it->second;
			_map.erase(it);
			instance->NotifyTerminated(code);
		}
		else
			LOG(WARNING) << "Unknown child " << pid << " terminated with status " << MakeResult(code);
	}
	void NotifyTerminated(int code)
	{
		_result = MakeResult(code);
		_exited = true;
		LOG(INFO) << "Subprocess " << _pid << " terminated with status " << _result;
		_io.dispatch([cbs = std::move(_waiters), result = _result]() {
			for (const auto& cb : cbs)
				cb(asio::error_code(), result);
		});
	}

private:
	static std::vector<const char*> MakeArgVector(const std::string& executable, const std::vector<std::string>& args)
	{
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
	void RunInternal(const char* executable, const char* const* args, const char* cwd, const char* const* env, unsigned int flags)
	{
		LOG(INFO) << "Executable: " << executable;
		LOG(INFO) << "Directory: " << cwd;

		PosixPipe stdin_pipe;
		PosixPipe stdout_pipe;
		PosixPipe stderr_pipe;
		PosixPipe status_pipe;

		// This makes use of a common pipe trick to detect the status of the
		// launched subprocess; a successful exec will close the stream, whereas
		// on failure the child will write an error code back to the parent.
		status_pipe.write.SetCloseOnExec();

		_io.notify_fork(asio::io_service::fork_prepare);

		pid_t pid = POSIX_CHECK(fork, ());
		if (pid == 0) // Child
		{
			_io.notify_fork(asio::io_service::fork_child);

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

				if (cwd)
					POSIX_CHECK(chdir, (cwd));

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
			_io.notify_fork(asio::io_service::fork_parent);

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
