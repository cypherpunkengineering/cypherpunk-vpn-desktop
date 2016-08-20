#include "config.h"

#include "daemon.h"
#include "logger.h"
#include "logger_file.h"
#include "openvpn.h"
#include "path.h"

#include "win.h"
#include "win_firewall.h"

#include <cstdio>
#include <tchar.h>
#include <direct.h>

#include <string>

#include <windows.h>


#define SERVICE_NAME             _T("CypherVPNService")
#define SERVICE_DISPLAY_NAME     _T("Cypherpunk VPN Background Service")
#ifdef _DEBUG
#define SERVICE_START_TYPE       SERVICE_DEMAND_START
#else
#define SERVICE_START_TYPE       SERVICE_AUTO_START
#endif
#define SERVICE_DEPENDENCIES     NULL // null-separated list
#define SERVICE_ACCOUNT          NULL // LocalSystem
//#define SERVICE_ACCOUNT          _T("NT AUTHORITY\\LocalService")
//#define SERVICE_ACCOUNT          _T("NT AUTHORITY\\NetworkService")
#define SERVICE_PASSWORD         NULL


static constexpr DWORD SERVICE_DEFAULT_CONTROLS_ACCEPTED = 0
	| SERVICE_ACCEPT_STOP
//	| SERVICE_ACCEPT_PAUSE_CONTINUE
	| SERVICE_ACCEPT_SHUTDOWN
//	| SERVICE_ACCEPT_PARAMCHANGE
//	| SERVICE_ACCEPT_NETBINDCHANGE
//	| SERVICE_ACCEPT_HARDWAREPROFILECHANGE
//	| SERVICE_ACCEPT_POWEREVENT
//	| SERVICE_ACCEPT_SESSIONCHANGE
//	| SERVICE_ACCEPT_PRESHUTDOWN
//	| SERVICE_ACCEPT_TIMECHANGE
//	| SERVICE_ACCEPT_TRIGGEREVENT
;

SERVICE_STATUS_HANDLE g_service_status_handle = NULL;
HANDLE g_service_thread_handle = NULL;
HANDLE g_service_wait_handle = NULL;

static FileLogger g_stdout_logger(stdout);
static FileLogger g_stderr_logger(stderr);
static FileLogger g_file_logger;


template<typename RESULT, typename INPUT>
void AppendQuotedCommandLineArgument(std::basic_string<RESULT>& result, const std::basic_string<INPUT>& arg, bool force = false)
{
	static const INPUT special_chars[] = { ' ', '\t', '\n', '\v', '"', 0 };

	if (!force && !arg.empty() && arg.find_first_of(special_chars) == arg.npos)
	{
		result.append(arg.begin(), arg.end());
	}
	else
	{
		result.push_back('"');

		for (auto it = arg.begin(); ; ++it)
		{
			size_t backslashes = 0;
			while (it != arg.end() && *it == '\\')
			{
				++it;
				++backslashes;
			}
			if (it == arg.end())
			{
				result.append(backslashes * 2, '\\');
				break;
			}
			else if (*it == '"')
			{
				result.append(backslashes * 2 + 1, '\\');
				result.push_back(*it);
			}
			else
			{
				result.append(backslashes, '\\');
				result.push_back(*it);
			}
		}

		result.push_back('"');
	}
}


class WinOpenVPNProcess : public OpenVPNProcess
{
public:
	WinOpenVPNProcess(asio::io_service& io) : OpenVPNProcess(io), _process_handle(INVALID_HANDLE_VALUE) {}

	HANDLE _process_handle;

	virtual void Run(const std::vector<std::string>& params) override
	{
		// Combine all parameters into a quoted command line string
		std::tstring cmdline = convert<TCHAR>("cypherpunkvpn-openvpn.exe");
		for (const auto& arg : params)
		{
			if (!cmdline.empty())
				cmdline.push_back(' ');
			AppendQuotedCommandLineArgument(cmdline, arg);
		}

		std::tstring executable = convert<TCHAR>(GetPath(OpenVPNExecutable));
		std::tstring cwd = convert<TCHAR>(GetPath(OpenVPNDir));

		STARTUPINFO startupinfo = { sizeof(STARTUPINFO), 0 };
		PROCESS_INFORMATION processinfo = { 0 };
		BOOL success = FALSE;

		// Get a user token for the built-in Network Service user (seems appropriate for an OpenVPN process)
		HANDLE network_service_token;
		if (LogonUser(_T("NETWORK SERVICE"), _T("NT AUTHORITY"), NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, &network_service_token))
		{
			// Launch OpenVPN as the specified user
			if (CreateProcessAsUser(network_service_token, executable.c_str(), &cmdline[0], NULL, NULL, FALSE, 0, NULL, cwd.c_str(), &startupinfo, &processinfo))
			{
				success = TRUE;
				_process_handle = processinfo.hProcess;
				LOG(INFO) << "Successfully started OpenVPN as network service";
			}
			else
				PLOG(WARNING) << "CreateProcessAsUser failed: " << LastError << " - retrying without impersonation";

			CloseHandle(network_service_token);
		}
		else
			PLOG(WARNING) << "LogonUser failed: " << LastError << " - retrying without impersonation";

		if (!success)
		{
			if (CreateProcess(executable.c_str(), &cmdline[0], NULL, NULL, FALSE, 0, NULL, cwd.c_str(), &startupinfo, &processinfo))
			{
				success = TRUE;
				_process_handle = processinfo.hProcess;
			}
			else
				PrintLastError(CreateProcess);
		}

		if (!success)
		{
			throw "unable to launch openvpn";
		}
	}

	virtual void Kill() override
	{
		if (_process_handle != INVALID_HANDLE_VALUE)
		{
			if (!TerminateProcess(_process_handle, -1))
				PrintLastError(TerminateProcess);
			_process_handle = INVALID_HANDLE_VALUE;
		}
	}
};

class WinCypherDaemon : public CypherDaemon
{
public:
	virtual int Run() override
	{
		if (win_get_tap_adapters().size() == 0)
		{
			LOG(CRITICAL) << "There are no installed TAP adapters on this machine!";
			return -1;
		}
		return CypherDaemon::Run();
	}
	virtual OpenVPNProcess* CreateOpenVPNProcess(asio::io_service& io) override
	{
		return new WinOpenVPNProcess(io);
	}
	virtual int GetAvailablePort(int hint) override
	{
		return hint;
	}
	virtual std::string GetAvailableAdapter(int index) override
	{
		const auto& adapters = win_get_tap_adapters();
		for (const auto& adapter : adapters)
		{
			// FIXME: Just return the first adapter for now; later improve to actually find an available one (see if they have a connected state?)
			return adapter.guid;
		}
		throw "no adapters found";
	}

	std::vector<GUID> _filters;
	bool _firewall_enabled = false;

	virtual bool SetFirewallSettings(bool enable) override
	{
		if (enable != _firewall_enabled)
		{
			FWEngine fw;
			try
			{
				if (enable)
				{
					fw.InstallProvider();
					_filters.push_back(fw.AddFilter(AllowLocalHostFilter<Outgoing, FWP_IP_VERSION_V4>()));
					_filters.push_back(fw.AddFilter(AllowLocalHostFilter<Outgoing, FWP_IP_VERSION_V6>()));
					_filters.push_back(fw.AddFilter(AllowDHCPFilter()));
					//_filters.push_back(fw.AddFilter(AllowDNSFilter<FWP_IP_VERSION_V4>()));
					_filters.push_back(fw.AddFilter(AllowAppFilter<Outgoing, FWP_IP_VERSION_V4>(GetPath(ClientExecutable))));
					_filters.push_back(fw.AddFilter(AllowAppFilter<Outgoing, FWP_IP_VERSION_V4>(GetPath(DaemonExecutable))));
					_filters.push_back(fw.AddFilter(AllowAppFilter<Outgoing, FWP_IP_VERSION_V4>(GetPath(OpenVPNExecutable))));
					_filters.push_back(fw.AddFilter(BlockAllFilter<Outgoing, FWP_IP_VERSION_V4>()));
					_filters.push_back(fw.AddFilter(BlockAllFilter<Outgoing, FWP_IP_VERSION_V6>()));
				}
				else
				{
					fw.RemoveFilters(_filters.rbegin(), _filters.rend());
					fw.RemoveAllFilters();
				}
				_firewall_enabled = enable;
				return true;
			}
			catch (const Win32Exception& e)
			{
				fw.RemoveAllFilters();
				_firewall_enabled = false;
				return false;
			}
		}
	}
};





static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	if (g_daemon)
		return g_daemon->Run();
	return -1;
}

static void SignalExitEvent()
{
	if (g_daemon)
		g_daemon->RequestShutdown();
}

static DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		SignalExitEvent();
		return NO_ERROR;

	case SERVICE_CONTROL_INTERROGATE:
		return NO_ERROR;

	//case SERVICE_CONTROL_PAUSE: return NO_ERROR;
	//case SERVICE_CONTROL_CONTINUE: return NO_ERROR;

	//case SERVICE_CONTROL_PARAMCHANGE:
	//case SERVICE_CONTROL_NETBINDADD:
	//case SERVICE_CONTROL_NETBINDREMOVE:
	//case SERVICE_CONTROL_NETBINDENABLE:
	//case SERVICE_CONTROL_NETBINDDISABLE:
	//case SERVICE_CONTROL_DEVICEEVENT:
	//case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
	//case SERVICE_CONTROL_POWEREVENT:
	//case SERVICE_CONTROL_SESSIONCHANGE:
	//case SERVICE_CONTROL_PRESHUTDOWN:
	//case SERVICE_CONTROL_TIMECHANGE:
	//case SERVICE_CONTROL_TRIGGEREVENT:
	default:
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
}



static BOOL ReportServiceStatus(
	DWORD dwCurrentState,
	DWORD dwControlsAccepted = SERVICE_DEFAULT_CONTROLS_ACCEPTED,
	DWORD dwWin32ExitCode = NO_ERROR,
	DWORD dwCheckPoint = 0,
	DWORD dwWaitHint = 0)
{
	static SERVICE_STATUS status;
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = dwCurrentState;
	status.dwControlsAccepted = dwControlsAccepted;
	status.dwWin32ExitCode = dwWin32ExitCode;
	status.dwServiceSpecificExitCode = 0;
	status.dwCheckPoint = dwCheckPoint;
	status.dwWaitHint = dwWaitHint;
	if (!SetServiceStatus(g_service_status_handle, &status))
	{
		PrintLastError(SetServiceStatus);
		return FALSE;
	}
	return TRUE;
}

static VOID CALLBACK ServiceEndCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	DWORD exit_code = 0;
	if (!GetExitCodeThread(g_service_thread_handle, &exit_code))
		PrintLastError(GetExitCodeThread);

	/*** CLEANUP ***/
	if (g_daemon)
	{
		delete g_daemon;
		g_daemon = nullptr;
	}

	if (!UnregisterWaitEx(g_service_wait_handle, NULL))
	{
		DWORD error = GetLastError();
		if (error != ERROR_IO_PENDING)
			PrintError(UnregisterWaitEx, error);
	}
	g_service_wait_handle = NULL;

	if (!CloseHandle(g_service_thread_handle))
		PrintLastError(CloseHandle);
	g_service_thread_handle = NULL;

	ReportServiceStatus(SERVICE_STOPPED, 0, NO_ERROR, 3);
	g_service_status_handle = NULL;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	if (g_service_status_handle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandlerEx, NULL))
	{
		/*** INIT ***/
		g_daemon = new WinCypherDaemon();

		ReportServiceStatus(SERVICE_RUNNING);

		if (g_service_thread_handle = CreateThread(NULL, 0, ServiceWorkerThread, NULL, CREATE_SUSPENDED, NULL))
		{
			if (RegisterWaitForSingleObject(&g_service_wait_handle, g_service_thread_handle, ServiceEndCallback, NULL, INFINITE, WT_EXECUTEONLYONCE))
			{
				if (ResumeThread(g_service_thread_handle) != (DWORD)-1)
				{
					// Success
					return;
				}
				else
					PrintLastError(ResumeThread);

				if (!UnregisterWait(g_service_wait_handle))
					PrintLastError(UnregisterWait);
			}
			else
				PrintLastError(RegisterWaitForSingleObject);

			if (!TerminateThread(g_service_thread_handle, 0))
				PrintLastError(TerminateThread);

			if (!CloseHandle(g_service_thread_handle))
				PrintLastError(CloseHandle);
		}
		else
			PrintLastError(CreateThread);
	}
}


static BOOL win_install_service(
	_In_ PCTSTR service_name,
	_In_ PCTSTR display_name,
	_In_ DWORD start_type,
	_In_opt_ PCZZTSTR dependencies,
	_In_opt_ PCTSTR user_account,
	_In_opt_ PCTSTR user_password)
{
	BOOL result = FALSE;

	TCHAR path[MAX_PATH];
	DWORD len = GetModuleFileName(NULL, path + 1, ARRAYSIZE(path) - 2);
	if (len > 0 && len < ARRAYSIZE(path) - 2)
	{
		path[0] = '"';
		path[1 + len] = '"';
		path[1 + len + 1] = 0;

		if (SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE))
		{
			if (SC_HANDLE service = CreateService(
				sc_manager,                   // SCManager database
				service_name,                 // Name of service
				display_name,                 // Name to display
				SERVICE_QUERY_STATUS,         // Desired access
				SERVICE_WIN32_OWN_PROCESS,    // Service type
				start_type,                   // Service start type
				SERVICE_ERROR_NORMAL,         // Error control type
				path,                         // Service's binary
				NULL,                         // No load ordering group
				NULL,                         // No tag identifier
				dependencies,                 // Dependencies
				user_account,                 // Service running account
				user_password                 // Password of the account
			))
			{
				_tcprintf(_T("Successfully installed %s.\n"), service_name);
				result = TRUE;

				CloseServiceHandle(service);
			}
			else
				PrintLastError(CreateService);

			CloseServiceHandle(sc_manager);
		}
		else
			PrintLastError(OpenSCManager);
	}
	else
		PrintLastError(GetModuleFileName);

	return result;
}

static BOOL win_uninstall_service(PCTSTR service_name)
{
	BOOL result = FALSE;

	if (SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
	{
		if (SC_HANDLE service = OpenService(sc_manager, service_name, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE))
		{
			SERVICE_STATUS status = {};
			if (ControlService(service, SERVICE_CONTROL_STOP, &status))
			{
				_tcprintf(_T("Stopping %s."), service_name);
				Sleep(500);

				while (QueryServiceStatus(service, &status))
				{
					if (status.dwCurrentState == SERVICE_STOP_PENDING)
					{
						_tcprintf(_T("."));
						Sleep(500);
					}
					else
						break;
				}

				if (status.dwCurrentState == SERVICE_STOPPED)
					_tcprintf(_T("\n%s is stopped.\n"), service_name);
				else
					_tcprintf(_T("\n%s failed to stop.\n"), service_name);
			}

			if (DeleteService(service))
			{
				_tcprintf(_T("%s is removed.\n"), service_name);
				result = TRUE;
			}
			else
				_tcprintf(_T("DeleteService failed with error 0x%08lx\n"), GetLastError());

			CloseServiceHandle(service);
		}
		else
			_tcprintf(L"OpenService failed with error 0x%08lx\n", GetLastError());

		CloseServiceHandle(sc_manager);
	}
	else
		_tcprintf(L"OpenSCManager failed with error 0x%08lx\n", GetLastError());

	return result;
}

static BOOL win_start_service(PCTSTR service_name, DWORD argc = 0, PCTSTR* argv = NULL)
{
	BOOL result = FALSE;

	if (SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
	{
		if (SC_HANDLE service = OpenService(sc_manager, service_name, SERVICE_START | SERVICE_QUERY_STATUS))
		{
			SERVICE_STATUS status = {};
			DWORD new_argc;
			PCTSTR* new_argv;
			if (argc > 0 && argv != NULL)
			{
				new_argc = argc + 1;
				new_argv = (PCTSTR*)LocalAlloc(LMEM_FIXED, sizeof(PCTSTR) * new_argc);
				if (new_argv)
				{
					new_argv[0] = service_name;
					for (DWORD i = 1; i < new_argc; i++)
						new_argv[i] = argv[i - 1];
				}
			}
			else
			{
				new_argc = 0;
				new_argv = NULL;
			}
			if (new_argc == 0 || new_argv)
			{
				if (StartService(service, new_argc, new_argv))
				{
					_tcprintf(_T("Starting %s."), service_name);
					Sleep(300);

					while (QueryServiceStatus(service, &status))
					{
						if (status.dwCurrentState == SERVICE_START_PENDING)
						{
							_tcprintf(_T("."));
							Sleep(300);
						}
						else
							break;
					}

					if (status.dwCurrentState == SERVICE_RUNNING)
					{
						_tcprintf(_T("\n%s is running.\n"), service_name);
						result = TRUE;
					}
					else
						_tcprintf(_T("\n%s failed to start.\n"), service_name);
				}
				else
				{
					DWORD error = GetLastError();
					if (error == ERROR_SERVICE_ALREADY_RUNNING)
						_tcprintf(_T("%s is already running.\n"), service_name);
					else
						PrintError(StartService, error);
				}

				if (new_argv)
					LocalFree(new_argv);
			}
			else
				PrintLastError(LocalAlloc);

			CloseServiceHandle(service);
		}
		else
			PrintLastError(OpenService);

		CloseServiceHandle(sc_manager);
	}
	else
		PrintLastError(OpenSCManager);

	return result;
}

static BOOL win_stop_service(PCTSTR service_name)
{
	BOOL result = FALSE;

	if (SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
	{
		if (SC_HANDLE service = OpenService(sc_manager, service_name, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE))
		{
			SERVICE_STATUS status = {};
			if (ControlService(service, SERVICE_CONTROL_STOP, &status))
			{
				_tcprintf(_T("Stopping %s."), service_name);
				Sleep(300);

				while (QueryServiceStatus(service, &status))
				{
					if (status.dwCurrentState == SERVICE_STOP_PENDING)
					{
						_tcprintf(_T("."));
						Sleep(300);
					}
					else
						break;
				}

				if (status.dwCurrentState == SERVICE_STOPPED)
				{
					_tcprintf(_T("\n%s is stopped.\n"), service_name);
					result = TRUE;
				}
				else
					_tcprintf(_T("\n%s failed to stop.\n"), service_name);
			}
			else
				PrintLastError(ControlService);

			CloseServiceHandle(service);
		}
		else
			PrintLastError(OpenService);

		CloseServiceHandle(sc_manager);
	}
	else
		PrintLastError(OpenSCManager);

	return result;
}

static BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		_putts(_T("Terminating..."));
		SignalExitEvent();
		return TRUE;
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		_putts(_T("Terminating..."));
		SignalExitEvent();
		WaitForSingleObject(g_service_thread_handle, INFINITE);
		return FALSE;
	default:
		return TRUE;
	}
}

static BOOL win_run_service(PCTSTR service_name)
{
	BOOL result = FALSE;

	g_daemon = new WinCypherDaemon();

	if (g_service_thread_handle = CreateThread(NULL, 0, ServiceWorkerThread, NULL, CREATE_SUSPENDED, NULL))
	{
		_putts(_T("Running daemon as a normal process. Press Ctrl-C to exit.\n"));

		if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
		{
			if (ResumeThread(g_service_thread_handle) != (DWORD)-1)
			{
				DWORD exit_code;
				switch (WaitForSingleObject(g_service_thread_handle, INFINITE))
				{
				case WAIT_OBJECT_0:
					if (!GetExitCodeThread(g_service_thread_handle, &exit_code))
						_putts(_T("Daemon exited with an unknown error."));
					else if (exit_code != 0)
						_tcprintf(_T("Daemon exited with error code %d.\n"), exit_code);
					else
					{
						_putts(_T("Daemon terminated successfully."));
						result = TRUE;
					}
					break;
				case WAIT_FAILED:
					PrintLastError(WaitForSingleObject);
					break;
				}
			}
			else
				PrintLastError(ResumeThread);

			if (!SetConsoleCtrlHandler(CtrlHandler, FALSE))
				PrintLastError(SetConsoleCtrlHandler);
		}
		else
			PrintLastError(SetConsoleCtrlHandler);

		if (!result && !TerminateThread(g_service_thread_handle, 0))
			PrintLastError(TerminateThread);

		if (!CloseHandle(g_service_thread_handle))
			PrintLastError(CloseHandle);
		g_service_thread_handle = NULL;
	}
	else
		PrintLastError(CreateThread);

	delete g_daemon;
	g_daemon = nullptr;

	return result;
}

static int win_print_usage()
{
	_putts(_T("Available console commands:\n"));
	_putts(_T("install    Installs the service."));
	_putts(_T("uninstall  Uninstalls the service."));
	_putts(_T("start      Starts the service."));
	_putts(_T("stop       Stops the service."));
	_putts(_T("run        Runs the daemon as a normal process."));
	_putts(_T("addtap     Add a TAP adapter."));
	_putts(_T("removetap  Remove all TAP adapters."));

	return 0;
}

static int ConsoleMain(int argc, TCHAR *argv[])
{
	if (argc >= 2 && argv[1] && argv[1][0])
	{
		const TCHAR* cmd = argv[1];
		while (cmd[0] == '/' || cmd[0] == '-')
			cmd++;

		if (0 == _tcsicmp(cmd, _T("install")))
			return !win_install_service(SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_START_TYPE, SERVICE_DEPENDENCIES, SERVICE_ACCOUNT, SERVICE_PASSWORD);
		if (0 == _tcsicmp(cmd, _T("uninstall")))
			return !win_uninstall_service(SERVICE_NAME);
		if (0 == _tcsicmp(cmd, _T("start")))
			return !win_start_service(SERVICE_NAME);
		if (0 == _tcsicmp(cmd, _T("stop")))
			return !win_stop_service(SERVICE_NAME);
		if (0 == _tcsicmp(cmd, _T("run")))
			return !win_run_service(SERVICE_NAME);
		if (0 == _tcsicmp(cmd, _T("addtap")))
			return !win_install_tap_adapter();
		if (0 == _tcsicmp(cmd, _T("removetap")))
			return !win_uninstall_tap_adapters();
	}
	return win_print_usage();
}


int _tmain(int argc, TCHAR *argv[])
{
	InitPaths(argc > 0 ? convert<char>(argv[0]) : "./daemon.exe");

	_tmkdir(convert<TCHAR>(GetPath(LogDir)).c_str());
	g_file_logger.Open(GetPath(LogDir, "daemon.log"));
	Logger::Push(&g_file_logger);

	SERVICE_TABLE_ENTRY table[] =
	{
		{ SERVICE_NAME, ServiceMain },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(table))
	{
		DWORD error = GetLastError();
		if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
		{
			Logger::Push(&g_stderr_logger);
			int result = ConsoleMain(argc, argv);
#ifdef _DEBUG
			if (result != 0)
			{
				_putts(_T("Press any key to exit..."));
				getchar();
			}
#endif
			return result;
		}
		else
		{
			PrintError(StartServiceCtrlDispatcher, error);
			return 1;
		}
	}
	return 0;
}

