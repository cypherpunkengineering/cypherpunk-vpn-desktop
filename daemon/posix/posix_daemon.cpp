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
