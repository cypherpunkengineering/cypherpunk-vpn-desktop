#pragma once

#include "config.h"
#include "settings.h"
#include "subprocess.h"

#include <asio.hpp>

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <thread>

class OpenVPNListener;

class OpenVPNProcess : public std::enable_shared_from_this<OpenVPNProcess>
{
	OpenVPNProcess(const OpenVPNProcess&) = delete;
	OpenVPNProcess& operator=(const OpenVPNProcess&) = delete;
	friend class CypherDaemon;

private:
	asio::io_service& _io;
	std::shared_ptr<Subprocess> _process;
	asio::ip::tcp::acceptor _management_acceptor;
	asio::ip::tcp::socket _management_socket;
	std::deque<std::string> _management_write_queue;
	asio::streambuf _management_readbuf;

	OpenVPNListener* _listener;

	bool _management_signaled;

private:
	void HandleManagementWrite(const asio::error_code& error, std::size_t bytes_transferred);
	void HandleManagementReadLine(const asio::error_code& error, std::size_t bytes_transferred);

	void OnStdOut(const asio::error_code& error, std::string line);
	void OnStdErr(const asio::error_code& error, std::string line);

public:
	OpenVPNProcess(asio::io_service& io, OpenVPNListener* listener);
	virtual ~OpenVPNProcess();

public:
	void CopySettings();
	bool CompareSettings();

	int StartManagementInterface();
	void StopManagementInterface();
	void SendManagementCommand(std::string cmd);
	void OnManagementResponse(const std::string& prefix, std::function<void(std::string)> callback);

	// Request clean shutdown (first via management interface, calls Kill if that fails)
	void Shutdown();

	virtual void Run(const std::vector<std::string>& params);
	virtual void Kill();
	// Asynchronously wait for process termination
	//virtual void AsyncWait(std::function<void(const asio::error_code&)> cb);
};

class OpenVPNListener
{
public:
	virtual void OnOpenVPNStdOut(OpenVPNProcess* process, const asio::error_code& error, std::string line) {}
	virtual void OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line) {}
	virtual void OnOpenVPNManagement(OpenVPNProcess* process, const asio::error_code& error, std::string line) {}
	virtual void OnOpenVPNExited(OpenVPNProcess* process, const asio::error_code& error) {}
};
