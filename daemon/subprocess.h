#pragma once

#include "config.h"

#include <functional>
#include <limits>
#include <memory>

#include <asio.hpp>


class Subprocess : public std::enable_shared_from_this<Subprocess>
{
public:
	enum SubprocessFlags {
		RunAsNetworkUser = 0x100,
	};
	enum State {
		Constructed = 0,
		Running = 1,
		Exited = 2,
	};
	enum ResultCodes {
		NotStarted = INT_MIN,
		StillRunning,
		Stopped,
		Continued,
		OtherError,
		Success = 0,
	};
	typedef int Result;

protected:
	asio::io_service& _io;
	std::function<void(const asio::error_code&, std::string)> _on_stdout_line, _on_stderr_line;
	State _state;
	Result _result;

protected:
	Subprocess(asio::io_service& io) : _io(io), _state(Constructed), _result(NotStarted) {}

	virtual void DoRun(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd, const std::unordered_map<std::string, std::string>& env, unsigned int flags) = 0;

public:
	static std::shared_ptr<Subprocess> Create(asio::io_service& io);

	virtual ~Subprocess() {}

	// Retrieve the current state of the subprocess.
	State GetState() const { return _state; }
	// Set a callback which will receive every line of text written to the subprocess' stdout.
	void SetStdOutListener(std::function<void(const asio::error_code&, std::string)> cb) { _on_stdout_line = std::move(cb); }
	// Set a callback which will receive every line of text written to the subprocess' stderr.
	void SetStdErrListener(std::function<void(const asio::error_code&, std::string)> cb) { _on_stderr_line = std::move(cb); }

	// Run an executable as a subprocess; can only be called once.
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd, const std::unordered_map<std::string, std::string>& env, unsigned int flags) { DoRun(executable, args, cwd, env, flags); }
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd, const std::unordered_map<std::string, std::string>& env) { DoRun(executable, args, cwd, env, 0); }
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd, unsigned int flags) { DoRun(executable, args, cwd, {}, flags); }
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::unordered_map<std::string, std::string>& env, unsigned int flags) { DoRun(executable, args, {}, env, flags); }
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::string& cwd) { DoRun(executable, args, cwd, {}, 0); }
	void Run(const std::string& executable, const std::vector<std::string>& args, const std::unordered_map<std::string, std::string>& env) { DoRun(executable, args, {}, env, 0); }
	void Run(const std::string& executable, const std::vector<std::string>& args, unsigned int flags) { DoRun(executable, args, {}, {}, flags); }
	void Run(const std::string& executable, const std::vector<std::string>& args) { DoRun(executable, args, {}, {}, 0); }
	
	// Write a string to the subprocess' stdin. Can only be called once the subprocess is running.
	virtual void WriteToStdIn(std::string data) = 0;
	// Wait for the subprocess to finish execution, receiving a callback when it does.
	virtual void AsyncWait(std::function<void(const asio::error_code&, Result result)> cb) = 0;
	// Wait synchronously for the subprocess to exit.
	virtual Result Wait() = 0;

	// TODO: Kill etc.
	virtual void Kill(bool force = false) = 0;
};
