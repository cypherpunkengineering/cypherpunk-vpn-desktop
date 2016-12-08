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


#define SERVICE_NAME             _T("CypherpunkPrivacyService")
#define SERVICE_DISPLAY_NAME     _T("Cypherpunk Privacy Background Service")
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


class WinPipe
{
public:
	Win32Handle read;
	Win32Handle write;
public:
	WinPipe(bool inherit_read, bool inherit_write)
	{
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = (inherit_read || inherit_write) ? TRUE : FALSE;
		sa.lpSecurityDescriptor = NULL;

		WIN_CHECK_IF_FALSE(CreatePipe, (&read.ref(), &write.ref(), &sa, 0));
		if (!inherit_read)
			WIN_CHECK_IF_FALSE(SetHandleInformation, (read, HANDLE_FLAG_INHERIT, 0));
		if (!inherit_write)
			WIN_CHECK_IF_FALSE(SetHandleInformation, (write, HANDLE_FLAG_INHERIT, 0));
	}
};

class WinSubprocess
{
	asio::windows::basic_object_handle<> _handle;
	Win32Handle _job;
	DWORD _id;
	bool _signaled;
	bool _terminated;
public:
	Win32Handle stdin_handle;
	Win32Handle stdout_handle;
	Win32Handle stderr_handle;
public:
	WinSubprocess(asio::io_service& io) : _handle(io), _id(0), _signaled(false), _terminated(false) {}
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd, bool as_network_user)
	{
		// Combine all parameters into a quoted command line string
		std::tstring cmdline = convert<TCHAR>("cypherpunk-privacy-openvpn.exe");
		for (const auto& arg : args)
		{
			if (!cmdline.empty())
				cmdline.push_back(' ');
			AppendQuotedCommandLineArgument(cmdline, arg);
		}

		std::tstring t_executable = convert<TCHAR>(executable);
		std::tstring t_cwd = convert<TCHAR>(cwd);

		WinPipe stdin_pipe(true, false);
		WinPipe stdout_pipe(false, true);
		WinPipe stderr_pipe(false, true);

		STARTUPINFO startupinfo = { sizeof(STARTUPINFO), 0 };
		startupinfo.hStdInput = stdin_pipe.read;
		startupinfo.hStdOutput = stdout_pipe.write;
		startupinfo.hStdError = stderr_pipe.write;
		startupinfo.dwFlags |= STARTF_USESTDHANDLES;
		DWORD flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
		PROCESS_INFORMATION processinfo = { 0 };

		if (as_network_user)
		{
			try
			{
				// Get a user token for the built-in Network Service user.
				Win32Handle network_service_token;
				WIN_CHECK_IF_FALSE(LogonUser, (_T("NETWORK SERVICE"), _T("NT AUTHORITY"), NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, &network_service_token.ref()));
				// Launch OpenVPN as the specified user.
				WIN_CHECK_IF_FALSE(CreateProcessAsUser, (network_service_token, t_executable.c_str(), &cmdline[0], NULL, NULL, TRUE, flags, NULL, t_cwd.c_str(), &startupinfo, &processinfo));
				goto success;
			}
			catch (const Win32Exception& e)
			{
				LOG(WARNING) << e << " - retrying without impersonation";
			}
		}
		WIN_CHECK_IF_FALSE(CreateProcess, (t_executable.c_str(), &cmdline[0], NULL, NULL, TRUE, flags, NULL, t_cwd.c_str(), &startupinfo, &processinfo));
	success:
		// Save the process handle.
		_handle.assign(processinfo.hProcess);
		_id = processinfo.dwProcessId;
		// Don't need the thread handle.
		CloseHandle(processinfo.hThread);
		// Move-assign the pipe handles.
		stdin_handle = std::move(stdin_pipe.write);
		stdout_handle = std::move(stdout_pipe.read);
		stderr_handle = std::move(stderr_pipe.read);

		// Try to assign the new process to a job, so it gets closed automatically when the handle holder dies.
		try
		{
			_job = Win32Handle::Wrap(WIN_CHECK_IF_NULL(CreateJobObject, (NULL, NULL)));
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobinfo = {0};
			jobinfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			WIN_CHECK_IF_FALSE(SetInformationJobObject, (_job, JobObjectExtendedLimitInformation, &jobinfo, sizeof(jobinfo)));
			WIN_CHECK_IF_FALSE(AssignProcessToJobObject, (_job, _handle.native_handle()));
		}
		catch (const Win32Exception& e)
		{
			LOG(WARNING) << "Failed to create job object: " << e;
		}
	}
	DWORD Wait()
	{
		if (!_handle.is_open())
			throw std::exception("Null pointer exception");
		switch (WaitForSingleObject(_handle.native_handle(), INFINITE))
		{
		case WAIT_OBJECT_0:
		{
			DWORD code;
			WIN_CHECK_IF_FALSE(GetExitCodeProcess, (_handle.native_handle(), &code));
			return code;
		}
		case WAIT_FAILED:
			THROW_WIN32EXCEPTION(GetLastError(), WaitForSingleObject);
		default:
			throw std::exception("Unexpected result from WaitForSingleObject");
		}
	}
	void AsyncWait(const std::function<void(const asio::error_code&, DWORD code)>& cb)
	{
		_handle.async_wait([this, cb](const asio::error_code& error) {
			DWORD code;
			if (error)
				cb(error, 0);
			else if (GetExitCodeProcess(_handle.native_handle(), &code))
				cb(error, code);
			else
				cb(asio::error::access_denied, 0);
		});
	}
	void Kill()
	{
		if (_handle.is_open())
		{
			if (!_signaled)
			{
				if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, _id))
					PLOG(WARNING) << "GenerateConsoleCtrlEvent failed: " << LastError;
				_signaled = true;
			}
			else if (!_terminated)
			{
				if (!TerminateProcess(_handle.native_handle(), 9000))
					PLOG(WARNING) << "TerminateProcess failed: " << LastError;
				_terminated = true;
			}
		}
	}
private:
	template<typename RESULT, typename INPUT>
	static void AppendQuotedCommandLineArgument(std::basic_string<RESULT>& result, const std::basic_string<INPUT>& arg, bool force = false)
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
};


class WinOpenVPNProcess : public OpenVPNProcess, public WinSubprocess
{
	std::thread _stdout_read_thread;
	std::thread _stderr_read_thread;
public:
	WinOpenVPNProcess(asio::io_service& io) : OpenVPNProcess(io), WinSubprocess(io) {}

	virtual void Run(const std::vector<std::string>& params) override
	{
		WinSubprocess::Run(GetPath(OpenVPNExecutable), params, GetPath(OpenVPNDir), !g_settings.runOpenVPNAsRoot());

		_stdout_read_thread = std::thread(THIS_CALLBACK(ReadThread), std::move(stdout_handle), [this](const asio::error_code& error, std::string line) {
			g_daemon->OnOpenVPNStdOut(this, error, std::move(line));
		});
		_stderr_read_thread = std::thread(THIS_CALLBACK(ReadThread), std::move(stderr_handle), [this](const asio::error_code& error, std::string line) {
			g_daemon->OnOpenVPNStdErr(this, error, std::move(line));
		});
	}

	virtual void Kill() override
	{
		WinSubprocess::Kill();
	}

	virtual void AsyncWait(std::function<void(const asio::error_code&)> cb) override
	{
		WinSubprocess::AsyncWait([cb = std::move(cb)](const asio::error_code& error, DWORD code) {
			cb(error);
		});
	}

private:
	void ReadThread(Win32Handle handle, std::function<void(const asio::error_code&, std::string)> cb)
	{
		asio::io_service io;
		asio::error_code error;
		asio::streambuf buffer;
		while (size_t size = SyncReadUntil(handle, buffer, '\n', error))
		{
			if (!error)
			{
				std::istream is(&buffer);
				std::string line;
				std::getline(is, line);
				if (line.size() > 0 && *line.crbegin() == '\r')
					line.pop_back();
				_io.post([=, line = std::move(line)]() { cb(error, line); });
			}
			else
				_io.post([=]() { cb(error, std::string()); });
		}
		_io.post([=](){ cb(error, std::string()); });
	}

	static size_t SyncReadUntil(Win32Handle& handle, asio::streambuf& buffer, char delim, asio::error_code& error)
	{
		size_t search_position = 0;
		for (;;)
		{
			// Determine the range of the data to be searched.
			typedef asio::buffers_iterator<asio::streambuf::const_buffers_type> iterator;
			asio::streambuf::const_buffers_type buffers = buffer.data();
			iterator begin = iterator::begin(buffers);
			iterator pos = begin + search_position;
			iterator end = iterator::end(buffers);

			// Look for a match.
			auto it = std::find(pos, end, '\n');
			if (it != end)
			{
				// Found a match. We're done.
				error = asio::error_code();
				return it - begin + 1;
			}
			else
			{
				// No match. Next search can start with the new data.
				search_position = end - begin;
			}

			// Check if buffer is full.
			if (buffer.size() == buffer.max_size())
			{
				error = asio::error::not_found;
				return 0;
			}

			// Need more data.
			size_t bytes_to_read = asio::read_size_helper(buffer, 65536);
			buffer.commit(SyncReadSome(handle, buffer.prepare(bytes_to_read), error));
			if (error)
				return 0;
		}
	}
	static size_t SyncReadSome(Win32Handle& handle, asio::streambuf::mutable_buffers_type& buffers, asio::error_code& error)
	{
		asio::mutable_buffer buffer = buffers;
		void* data = asio::detail::buffer_cast_helper(buffer);
		DWORD bytes_to_read = (DWORD)asio::detail::buffer_size_helper(buffer);
		DWORD bytes_read = 0;
		if (ReadFile(handle, data, bytes_to_read, &bytes_read, NULL))
		{
			error = asio::error_code();
			return bytes_read;
		}
		else
		{
			error = asio::error_code(GetLastError(), asio::error::get_system_category());
			return 0;
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
		try
		{
			FWEngine fw;
			fw.InstallProvider();
		}
		catch (const Win32Exception& e)
		{
			LOG(ERROR) << "Failed to install firewall provider: " << e;
		}
		// TEMPORARY WORKAROUND: we don't remember what rules we might have applied
		// in an earlier session, so delete all rules.
		try
		{
			FWEngine fw;
			FWTransaction tx(fw);
			fw.RemoveAllFilters();
			tx.Commit();
		}
		catch (const Win32Exception& e)
		{
			LOG(ERROR) << "Failed to wipe firewall rules on startup: " << e;
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

	enum FilterIndexes
	{
		allow_lan_ipv4_1,
		allow_lan_ipv4_2,
		allow_lan_ipv4_3,
		allow_lan_ipv4_multicast,
		allow_lan_ipv6,
		allow_lan_ipv6_linklocal,
		allow_lan_ipv6_multicast,

		allow_client,
		allow_daemon,
		allow_openvpn,
		allow_localhost_ipv4,
		allow_localhost_ipv6,
		allow_dhcp_ipv4,
		allow_dhcp_ipv6,
		block_ipv4,
		block_ipv6,

		max_filter,
		first_lan_filter = allow_lan_ipv4_1,
		last_lan_filter = allow_lan_ipv6_multicast,
	};
	GUID _filters[max_filter];
	std::map<uint64_t, GUID> _tap_filters;

	virtual void ApplyFirewallSettings() override
	{
		FWEngine fw;
		FWTransaction tx(fw);

		int success = 0, error = 0;

#define TURN_ON(name, ...) \
		do { \
			if (_filters[name] == zero_guid) { \
				_filters[name] = fw.AddFilter(__VA_ARGS__); success++; \
			} \
		} while(false)
#define TURN_OFF(idx) \
		do { \
			if (_filters[idx] != zero_guid) { \
				try { fw.RemoveFilter(_filters[idx]); success++; _filters[idx] = zero_guid; } \
				catch (const Win32Exception& e) { error++; LOG(ERROR) << "Unable to remove firewall rule #" << idx << ": " << e; } \
			} \
		} while(false)

		bool is_connected;
		switch (_state)
		{
		case CONNECTING:
		case CONNECTED:
		case DISCONNECTING:
		case SWITCHING:
			is_connected = true;
			break;
		default:
			is_connected = false;
			break;
		}

		auto adapters = win_get_tap_adapters();

		auto mode = g_settings.firewall();
		if (!_connections.empty() && (mode == "on" || (is_connected && mode == "auto")))
		{
			try
			{
				if (g_settings.allowLAN())
				{
					TURN_ON(allow_lan_ipv4_1,         AllowIPRangeFilter<Outgoing, IPv4>("192.168.0.0", 16));
					TURN_ON(allow_lan_ipv4_2,         AllowIPRangeFilter<Outgoing, IPv4>("172.16.0.0", 13));
					TURN_ON(allow_lan_ipv4_3,         AllowIPRangeFilter<Outgoing, IPv4>("10.0.0.0", 8));
					TURN_ON(allow_lan_ipv4_multicast, AllowIPRangeFilter<Outgoing, IPv4>("224.0.0.0", 4));
					TURN_ON(allow_lan_ipv6,           AllowIPRangeFilter<Outgoing, IPv6>("fc00::", 7));
					TURN_ON(allow_lan_ipv6_linklocal, AllowIPRangeFilter<Outgoing, IPv6>("fe80::", 10));
					TURN_ON(allow_lan_ipv6_multicast, AllowIPRangeFilter<Outgoing, IPv6>("ff00::", 8));
				}
				else
				{
					for (int i = first_lan_filter; i <= last_lan_filter; i++)
						TURN_OFF(i);
				}

				std::set<uint64_t> tap_filters_to_remove;
				for (auto& p : _tap_filters)
					tap_filters_to_remove.insert(p.first);
				for (auto& adapter : adapters)
				{
					auto it = _tap_filters.find(adapter.luid);
					if (it != _tap_filters.end())
						tap_filters_to_remove.erase(it->first);
					else
					{
						_tap_filters.insert(std::pair<uint64_t, GUID>(adapter.luid, fw.AddFilter(AllowInterfaceFilter<Outgoing, IPv4>(adapter.luid))));
						success++;
					}
				}
				for (auto& luid : tap_filters_to_remove)
				{
					fw.RemoveFilter(_tap_filters.at(luid));
					success++;
				}

				TURN_ON(allow_client,         AllowAppFilter<Outgoing, IPv4>(GetPath(ClientExecutable)));
				TURN_ON(allow_daemon,         AllowAppFilter<Outgoing, IPv4>(GetPath(DaemonExecutable)));
				TURN_ON(allow_openvpn,        AllowAppFilter<Outgoing, IPv4>(GetPath(OpenVPNExecutable)));
				TURN_ON(allow_localhost_ipv4, AllowLocalHostFilter<Outgoing, IPv4>());
				TURN_ON(allow_localhost_ipv6, AllowLocalHostFilter<Outgoing, IPv6>());
				TURN_ON(allow_dhcp_ipv4,      AllowDHCPFilter<IPv4>());
				TURN_ON(allow_dhcp_ipv6,      AllowDHCPFilter<IPv6>());
				TURN_ON(block_ipv6,           BlockAllFilter<Outgoing, IPv4>());
				TURN_ON(block_ipv6,           BlockAllFilter<Outgoing, IPv6>());

				if (success) tx.Commit();
				return;
			}
			catch (const Win32Exception& e)
			{
				LOG(ERROR) << "Unable to apply firewall rule: " << e;
				error++;
			}
		}
		// Firewall is either off, or there was an error applying rules

		// We use a transaction but remove in reverse just in case, so the block filters get removed first
		for (auto& p : _tap_filters)
		{
			try { fw.RemoveFilter(p.second); }
			catch (const Win32Exception& e) { error++; LOG(ERROR) << "Unable to remove firewall rule for adapter" << p.first << ": " << e; }
		}
		_tap_filters.clear();
		for (int i = max_filter - 1; i >= 0; i--)
			TURN_OFF(i);

		if (success) tx.Commit();
		if (error)
		{
			_io.post([this]() {
				SendErrorToAllClients("firewall", "Unable to apply firewall rules; the firewall feature has been disabled.");
				g_settings.firewall("off");
				g_settings.OnChanged({ "firewall" });
			});
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
		return FALSE;
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

