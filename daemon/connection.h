#pragma once

#include "logger.h"
#include "openvpn.h"
#include "settings.h"

#include <chrono>
#include <functional>
#include <memory>

#include <asio.hpp>


class ConnectionListener;

// A Connection object tracks the time between the user switches the main connection slider 'on'
// and until they switch it back off again, or alternatively until the client app exits.
// This corresponds to the 'connect' state being 'true'.
class Connection : public std::enable_shared_from_this<Connection>, public OpenVPNListener
{
	typedef std::chrono::steady_clock clock;
	typedef clock::time_point time_point;
	typedef clock::duration duration;
	typedef asio::basic_waitable_timer<clock> timer;

public:
	enum State
	{
		// Initial state; created but not connecting yet
		// - Quickly transitions to CONNECTING
		CREATED,

		// Connecting (OpenVPN process launched etc.)
		// - Try connect up to 3 times
		// - Try to connect for up to 20 seconds total
		// - Never pause between connection attempts
		// - If still not connected, transition to STILL_CONNECTING
		CONNECTING,

		// Still connecting (taking long or many attempts)
		// - Infinite number of attempts
		// - Minimum time between start of each attempt: 10 seconds
		// - Individual connection timeout: 60 seconds
		STILL_CONNECTING,

		// Successfully connected
		CONNECTED,

		// Connection failed/interrupted after previously being connected
		// - Current OpenVPN process is discarded; wait for process exit
		// - Afterwards, go to RECONNECTING
		INTERRUPTED,

		// Waiting for an OpenVPN connection attempt to recover connectivity
		// - Infinite number of attempts; just keep relaunching
		// - Minimum time between start of each attempt: 5 seconds
		// - Try to reconnect for up to 60 seconds, then go to STILL_RECONNECTING
		RECONNECTING,

		// Having trouble regaining connectivity (maybe internet is down)
		// - Infinite number of attempts; just keep relaunching
		// - Minimum time between start of attempts: 10 seconds
		// - Individual connection timeout: 60 seconds
		STILL_RECONNECTING,

		// Disconnecting in order to reconnect again
		// - Current OpenVPN process is discarded; wait for process exit
		// - Afterwards, go to CONNECTING
		DISCONNECTING_TO_RECONNECT,

		// Disconnecting from server (intentionally)
		DISCONNECTING,

		// Disconnected from server (can reconnect by calling Connect)
		DISCONNECTED = CREATED,
	};

	// Maximum connection attempts until switching to 'slow' mode
	enum Limits
	{
		MAX_CONNECTION_ATTEMPTS = 1,
		MAX_RECONNECTION_ATTEMPTS = 50,
		MAX_CONNECTION_TIME = 15,
		MAX_RECONNECTION_TIME = 300,
		CONNECTION_ATTEMPT_INTERVAL = 1,
		RECONNECTION_ATTEMPT_INTERVAL = 1,
		SLOW_CONNECTION_ATTEMPT_INTERVAL = 1,
		SLOW_RECONNECTION_ATTEMPT_INTERVAL = 10,
	};

	enum OpenVPNState
	{
		OPENVPN_CONNECTING,
		OPENVPN_TCP_CONNECT,
		OPENVPN_WAIT,
		OPENVPN_AUTH,
		OPENVPN_GET_CONFIG,
		OPENVPN_ASSIGN_IP,
		OPENVPN_ADD_ROUTES,
		OPENVPN_CONNECTED,
		OPENVPN_RECONNECTING,
		OPENVPN_EXITING,
		OPENVPN_EXITED, // Process has exited
	};

private:
	asio::io_service& _io;

	ConnectionListener* _listener;

	// Current state
	State _state;
	OpenVPNState _openvpn_state;

	// Connection attempt counter (1 on first OpenVPN process, reset on successful connection)
	unsigned int _connection_attempts;

	// Timeout timer for triggering the 'STILL_[RE]CONNECTING' states
	timer _slow_connection_timer;
	// Time of last launched OpenVPN process
	time_point _last_openvpn_launch;
	timer _connection_interval_timer;

	// Saved settings
	JsonObject _settings;
	JsonObject _server;
	std::string _username, _password;
	bool _needsReconnect, _cypherplay;

	// Current processes
	std::shared_ptr<OpenVPNProcess> _openvpn_process;
	std::shared_ptr<Subprocess> _stunnel_process;

private:
	virtual void OnOpenVPNStdOut(OpenVPNProcess* process, const asio::error_code& error, std::string line) override;
	virtual void OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line) override;
	virtual void OnOpenVPNManagement(OpenVPNProcess* process, const asio::error_code& error, std::string line) override;
	virtual void OnOpenVPNExited(OpenVPNProcess* process, const asio::error_code& error) override;

	void SetState(State new_state);
	void SetOpenVPNState(OpenVPNState new_openvpn_state);

	void StartConnectionTimer(duration timeout);
	void OnSlowConnectionTimeout(const asio::error_code& error);
	void ScheduleConnect(duration minimum_interval);
	void WriteOpenVPNProfile(std::ostream& out);

	void DoConnect();

public:
	Connection(asio::io_service& io, ConnectionListener* listener);
	~Connection();

	void CopySettings(); // Saves necessary settings from g_settings

	State GetState();
	const char* GetStateString();
	bool NeedsReconnect();

	bool Connect(bool force); // Saves any necessary settings from g_daemon, returns false if already connected to the right server
	void Disconnect(); // Begin disconnecting; once disconnected, can reconnect 
};

class ConnectionListener
{
public:
	virtual void OnConnectionStateChanged(Connection* connection, Connection::State state) {}
	virtual void OnTrafficStatsUpdated(Connection* connection, uint64_t downloaded, uint64_t uploaded) {}
	virtual void OnConnectionAttempt(Connection* connection, int attempt_number) {}
	virtual void OnOpenVPNCallback(OpenVPNProcess* process, std::string line) {}
};
bool EnumFromString(const std::string& str, Connection::State& value);
const char* EnumToString(Connection::State value);
bool EnumFromString(const std::string& str, Connection::OpenVPNState& value);
const char* EnumToString(Connection::OpenVPNState value);
bool EnumFromString(const std::string& str, Connection::ErrorCode& value);
const char* EnumToString(Connection::ErrorCode value);
