#include "../config.h"

#include "../daemon.h"

#include <stdio.h>
#include <tchar.h>

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

#define PrintError(operation, error) _tcprintf(_T("%s failed with error 0x%08xlx\n"), _T(#operation), error)
#define PrintLastError(operation) PrintError(operation, GetLastError())

bool IsWin64()
{
#ifdef _WIN64
	return true;
#else
	static bool is_wow64 = []() {
		BOOL res = FALSE;
		return IsWow64Process(GetCurrentProcess(), &res) && res;
	}();
	return is_wow64;
#endif
}


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
		PrintLastError(UnregisterWaitEx);
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
		g_daemon = new CypherDaemon();

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

	g_daemon = new CypherDaemon();

	if (g_service_thread_handle = CreateThread(NULL, 0, ServiceWorkerThread, NULL, CREATE_SUSPENDED, NULL))
	{
		_putts(_T("Running daemon as a normal process. Press Ctrl-C to exit.\n"));

		if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
		{
			if (ResumeThread(g_service_thread_handle) != (DWORD)-1)
			{
				switch (WaitForSingleObject(g_service_thread_handle, INFINITE))
				{
				case WAIT_OBJECT_0:
					_putts(_T("Daemon terminated successfully."));
					result = TRUE;
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

	return 0;
}

static int ConsoleMain(int argc, TCHAR *argv[])
{
	if (argc >= 2 && argv[1] && argv[1][0])
	{
		const TCHAR* cmd = argv[1];
		if (cmd[0] == '/' || cmd[0] == '-')
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
	}
	return win_print_usage();
}


int _tmain(int argc, TCHAR *argv[])
{
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
			// Being run as a console app
			return ConsoleMain(argc, argv);
		}
		else
		{
			PrintError(StartServiceCtrlDispatcher, error);
			return 1;
		}
	}
	return 0;
}

