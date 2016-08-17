#include "config.h"

#include "daemon.h"
#include "logger.h"
#include "logger_file.h"
#include "openvpn.h"
#include "path.h"

#include <signal.h>


class PosixOpenVPNProcess : public OpenVPNProcess
{
public:
	PosixOpenVPNProcess(asio::io_service& io) : OpenVPNProcess(io) {}

	virtual void Run(const std::vector<std::string>& params) override
	{
		std::string cmdline = GetPath(OpenVPNExecutable);
		for (const auto& param : params)
		{
			cmdline += ' ';
			cmdline += param; // FIXME: quote/escape
		}
		// execute cmdline asynchronously - popen?
	}

	virtual void Kill() override
	{

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

int main(int argc, char **argv)
{
	// Instantiate the posix version of the daemon
	g_daemon = new PosixCypherDaemon();

	// Register a signal handler for SIGTERM for clean shutdowns
	signal(SIGTERM, sigterm_handler);

	// Run the daemon synchronously
	return g_daemon->Run();
}
