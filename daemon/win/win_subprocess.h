#include "config.h"
#include "subprocess.h"

#include <windows.h>
#include <tchar.h>
#include <future>
#include <thread>

#include "win.h"


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

class WinSubprocess : public Subprocess
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

protected:
	std::thread _stdout_read_thread;
	std::thread _stderr_read_thread;

public:
	WinSubprocess(asio::io_service& io) : Subprocess(io), _handle(io), _id(0), _signaled(false), _terminated(false) {}
	virtual ~WinSubprocess()
	{
		if (_stderr_read_thread.joinable()) _stderr_read_thread.detach();
		if (_stdout_read_thread.joinable()) _stdout_read_thread.detach();
	}

protected:
	virtual void DoRun(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd, const std::unordered_map<std::string, std::string>& env, unsigned int flags) override
	{
		// Combine all parameters into a quoted command line string
		std::tstring cmdline = convert<TCHAR>(executable);
		auto last_slash = cmdline.find_last_of(_T("\\/"));
		if (last_slash != cmdline.npos)
		{
			cmdline.erase(0, last_slash + 1);
		}
		for (const auto& arg : args)
		{
			if (!cmdline.empty())
				cmdline.push_back(' ');
			AppendQuotedCommandLineArgument(cmdline, arg);
		}

		std::tstring t_executable = convert<TCHAR>(executable);
		std::tstring t_cwd = convert<TCHAR>(cwd);

		if (env.size())
		{
			LOG(WARNING) << "Environment variables are not yet supported on Windows";
		}

		WinPipe stdin_pipe(true, false);
		WinPipe stdout_pipe(false, true);
		WinPipe stderr_pipe(false, true);

		STARTUPINFO startupinfo = { sizeof(STARTUPINFO), 0 };
		startupinfo.hStdInput = stdin_pipe.read;
		startupinfo.hStdOutput = stdout_pipe.write;
		startupinfo.hStdError = stderr_pipe.write;
		startupinfo.dwFlags |= STARTF_USESTDHANDLES;
		DWORD process_flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
		PROCESS_INFORMATION processinfo = { 0 };

		if (flags & RunAsNetworkUser)
		{
			try
			{
				// Get a user token for the built-in Network Service user.
				Win32Handle network_service_token;
				WIN_CHECK_IF_FALSE(LogonUser, (_T("NETWORK SERVICE"), _T("NT AUTHORITY"), NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, &network_service_token.ref()));
				// Launch OpenVPN as the specified user.
				WIN_CHECK_IF_FALSE(CreateProcessAsUser, (network_service_token, t_executable.c_str(), &cmdline[0], NULL, NULL, TRUE, process_flags, NULL, t_cwd.c_str(), &startupinfo, &processinfo));
				goto success;
			}
			catch (const Win32Exception& e)
			{
				LOG(WARNING) << e << " - retrying without impersonation";
			}
		}
		WIN_CHECK_IF_FALSE(CreateProcess, (t_executable.c_str(), &cmdline[0], NULL, NULL, TRUE, process_flags, NULL, t_cwd.c_str(), &startupinfo, &processinfo));

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

		_stdout_read_thread = std::thread(ReadThread, SHARED_THIS, std::move(stdout_handle), &_stdout_read_thread, &_on_stdout_line);
		_stderr_read_thread = std::thread(ReadThread, SHARED_THIS, std::move(stderr_handle), &_stderr_read_thread, &_on_stderr_line);
		
		//SHARED_LAMBDA([this](const asio::error_code& error, std::string line) { if (_on_stdout_line) _on_stdout_line(error, std::move(line)); }), SHARED_LAMBDA([this]() { if (_stdout_read_thread.joinable()) _stdout_read_thread.join(); }));
		//_stderr_read_thread = std::thread(ReadThread, shared_from_this(), std::move(stderr_handle), SHARED_LAMBDA([this](const asio::error_code& error, std::string line) { if (_on_stderr_line) _on_stderr_line(error, std::move(line)); }), SHARED_LAMBDA([this]() { if (_stderr_read_thread.joinable()) _stderr_read_thread.join(); }));
	}
	virtual Result Wait() override
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
	
public:
	virtual void WriteToStdIn(std::string data) override
	{
		LOG(WARNING) << "Writing to stdin not yet implemented on Windows";
	}

	virtual void AsyncWait(std::function<void(const asio::error_code&, Result result)> cb) override
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
	virtual void Kill(bool force = false) override
	{
		if (_handle.is_open())
		{
			if (!_signaled && !force)
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
private:
	static void ReadThread(std::shared_ptr<WinSubprocess> self, Win32Handle handle, std::thread* thread, std::function<void(const asio::error_code&, std::string)>* cb)
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
				self->_io.post([self, error, cb, line = std::move(line)]() { (*cb)(error, line); });
			}
			else
				self->_io.post([self, error, cb]() { (*cb)(error, std::string()); });
		}
		self->_io.post([self, error, thread, cb]() { (*cb)(error, std::string()); if (thread->joinable()) thread->join(); });
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
			auto it = std::find(pos, end, delim);
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
