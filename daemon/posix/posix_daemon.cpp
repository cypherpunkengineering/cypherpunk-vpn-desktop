#include "config.h"

#include "client_multiplexer.h"
#include "client_websocket.h"
#include "daemon.h"
#include "logger.h"
#include "logger_file.h"
#include "openvpn.h"
#include "path.h"

#include "posix.h"
#include "posix_subprocess.h"

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
#include <sys/stat.h>
#include <unistd.h>

#ifdef OS_LINUX
#include <sys/wait.h>
#endif

void firewall_install();
void firewall_uninstall();
//std::string firewall_enable();
//void firewall_disable(const std::string& token);
void firewall_ensure_enabled();
void firewall_ensure_disabled();
void firewall_enable_anchor(const std::string& anchor);
void firewall_disable_anchor(const std::string& anchor);
bool firewall_anchor_enabled(const std::string& anchor);
void firewall_set_anchor_enabled(const std::string& anchor, bool enable);

std::unordered_map<pid_t, PosixSubprocess*> PosixSubprocess::_map;

std::shared_ptr<Subprocess> Subprocess::Create(asio::io_service& io)
{
	return std::make_shared<PosixSubprocess>(io);
}


class PosixCypherDaemon : public CypherDaemon
{
	asio::signal_set _signals;

public:
	PosixCypherDaemon()
		: _signals(_io)
	{
		// Register signal listeners
		_signals.add(SIGCHLD);
		_signals.add(SIGPIPE);
		_signals.add(SIGINT);
		_signals.add(SIGTERM);		
		_signals.async_wait(THIS_CALLBACK(OnSignal));

		auto client_interface = std::make_shared<ClientInterfaceMultiplexer>(_io);
		client_interface->InitializeClientInterface<WebSocketClientInterface>(_io);
		SetClientInterface(std::move(client_interface));
	}
	virtual void OnBeforeRun() override
	{
		firewall_install();
	}
	virtual void OnAfterRun() override
	{

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
		auto mode = g_settings.firewall();
		if (!_client_connections.empty() && (mode == "on" || (_shouldConnect && mode == "auto")))
		{
#if OS_OSX
			firewall_set_anchor_enabled("100.killswitch", true);
			firewall_set_anchor_enabled("200.exemptLAN", g_settings.allowLAN());
#elif OS_LINUX
			firewall_set_anchor_enabled("100.exemptLAN", g_settings.allowLAN());
			firewall_set_anchor_enabled("500.killswitch", true);
#endif
			firewall_ensure_enabled();
		}
		else
		{
			firewall_ensure_disabled();
#if OS_OSX
			firewall_set_anchor_enabled("100.killswitch", false);
			firewall_set_anchor_enabled("200.exemptLAN", false);
#elif OS_LINUX
			firewall_set_anchor_enabled("100.exemptLAN", false);
			firewall_set_anchor_enabled("500.killswitch", false);
#endif
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
				case SIGINT:
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

unsigned short GetPingIdentifier()
{
	return (unsigned short)::getpid();
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
	// First, make sure we're running as root:cypherpunk
	if (geteuid() != 0)
	{
		int uid = geteuid();
		struct passwd* pw = getpwuid(uid);
		fprintf(stderr, "Error: running as user %s (%d), must be root.\n", pw && pw->pw_name ? pw->pw_name : "<unknown uid>", uid);
		return 1;
	}
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

	// Set default umask (not writable by group or others)
	umask(S_IWGRP | S_IWOTH);

	g_old_terminate_handler = std::set_terminate(terminate_handler);

	InitPaths(argc > 0 ? argv[0] : "cypherpunk-privacy-service");

	// Set up logging
	chmod(GetPath(LogDir, EnsureExists).c_str(), 01777); // fix any permission problem
	g_file_logger.Open(GetFile(LogDir, "daemon.log"));
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
