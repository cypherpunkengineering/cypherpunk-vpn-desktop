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


class PosixOpenVPNProcess : public OpenVPNProcess
{
	FILE* _file;
	pid_t _pid;

public:
	PosixOpenVPNProcess(asio::io_service& io) : OpenVPNProcess(io), _file(nullptr), _pid(0) {}
	~PosixOpenVPNProcess()
	{
		Kill();
	}

	virtual void Run(const std::vector<std::string>& params) override
	{
		std::string openvpn = GetPath(OpenVPNExecutable);
		const char** args = new const char*[params.size() + 1];
		for (size_t i = 0; i < params.size(); i++)
		{
			// TODO: Need any quoting?
			args[i] = params[i].c_str();
		}
		args[params.size()] = NULL;

		int handles[2];
		POSIX_CHECK(pipe, (handles));
		pid_t pid = POSIX_CHECK(fork());
		if (pid == 0)
		{
			// Child; redirect stdout to the write end of the pipe and close the read end, then exec openvpn
			dup2(handles[1], STDOUT_FILENO);
			close(handles[0]);
			close(handles[1]);
			// TODO: Should close any other file handles held by us too
			execv(openvpn.c_str(), args);
			// If we reach here, we failed to launch OpenVPN; immediately exit the child process.
			_exit(9000);
		}
		else
		{
			// Parent
			_pid = pid;
		}

		// execute cmdline asynchronously - popen?
		_file = popen(cmdline.c_str(), "w"); // TODO: later, if parsing output, use "r" instead
	}

	virtual void Kill() override
	{
		if (_file)
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
