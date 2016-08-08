#include "config.h"

#include "daemon.h"
#include "logger.h"
#include "logger_file.h"
#include "openvpn.h"

// #include "posix.h"

class PosixOpenVPNProcess : public OpenVPNProcess
{
public:
	PosixOpenVPNProcess(asio::io_service& io) : OpenVPNProcess(io) {}

	virtual void Run(const std::vector<std::string>& params) override
	{

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
};

int
main(int argc, char **argv)
{
	g_daemon = new PosixCypherDaemon();
	g_daemon->Run();
	return 0;
}
