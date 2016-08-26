#include "config.h"

#include "daemon.h"
#include "logger.h"
#include "logger_file.h"
#include "openvpn.h"
#include "path.h"

#include <signal.h>


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

void terminate_handler()
{
	LOG(CRITICAL) << "Exiting due to unhandled exception";
}

static FileLogger g_stdout_logger(stdout);
static FileLogger g_stderr_logger(stderr);
static FileLogger g_file_logger;

int main(int argc, char **argv)
{
	std::set_terminate(terminate_handler);

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
