#include "config.h"

#include "connection.h"
#include "daemon.h"
#include "path.h"

#include <fstream>


static const std::string g_connection_setting_names[] = {
	"remotePort",
	"location",
	"localPort",
	"mtu",
	"encryption",
	"overrideDNS",
	"optimizeDNS",
	"blockAds",
	"blockMalware",
	"routeDefault",
	"exemptApple",
	//"firewall",
	//"allowLAN",
	//"blockIPv6",
	"runOpenVPNAsRoot",
};

static const bool use_stunnel = false; // NOTE: not implemented, just placeholder


Connection::Connection(asio::io_service& io, ConnectionListener* listener)
	: _io(io)
	, _listener(listener)
	, _state(CREATED)
	, _openvpn_state(OPENVPN_EXITED)
	, _connection_attempts(0)
	, _slow_connection_timer(io)
	, _connection_interval_timer(io)
	, _needsReconnect(false)
	, _cypherplay(false)
{

}

Connection::~Connection()
{
	if (_openvpn_process)
		_openvpn_process->Shutdown();
	_slow_connection_timer.cancel();
	_connection_interval_timer.cancel();
}

bool Connection::EnumFromString(const std::string& str, State& value)
{
	#define VALUE(name) if (str == #name) { value = name; return true; }
	VALUE(CONNECTING)
	VALUE(STILL_CONNECTING)
	VALUE(CONNECTED)
	VALUE(INTERRUPTED)
	VALUE(RECONNECTING)
	VALUE(STILL_RECONNECTING)
	VALUE(DISCONNECTING_TO_RECONNECT)
	VALUE(DISCONNECTING)
	VALUE(DISCONNECTED)
	#undef VALUE
	return false;	
}

const char* Connection::EnumToString(State value)
{
	switch (value)
	{
		#define VALUE(name) case name: return #name;
		VALUE(CONNECTING)
		VALUE(STILL_CONNECTING)
		VALUE(CONNECTED)
		VALUE(INTERRUPTED)
		VALUE(RECONNECTING)
		VALUE(STILL_RECONNECTING)
		VALUE(DISCONNECTING_TO_RECONNECT)
		VALUE(DISCONNECTING)
		VALUE(DISCONNECTED)
		#undef VALUE
		default: return "";
	}
}

bool Connection::EnumFromString(const std::string& str, OpenVPNState& value)
{
	#define VALUE(name) if (str == "OPENVPN_"#name) { value = OPENVPN_##name; return true; }
	VALUE(CONNECTING)
	VALUE(WAIT)
	VALUE(AUTH)
	VALUE(GET_CONFIG)
	VALUE(ASSIGN_IP)
	VALUE(ADD_ROUTES)
	VALUE(CONNECTED)
	VALUE(RECONNECTING)
	VALUE(EXITING)
	VALUE(EXITED)
	#undef VALUE
	return false;
}

const char* Connection::EnumToString(OpenVPNState value)
{
	switch (value)
	{
		#define VALUE(name) case OPENVPN_##name: return #name;
		VALUE(CONNECTING)
		VALUE(WAIT)
		VALUE(AUTH)
		VALUE(GET_CONFIG)
		VALUE(ASSIGN_IP)
		VALUE(ADD_ROUTES)
		VALUE(CONNECTED)
		VALUE(RECONNECTING)
		VALUE(EXITING)
		VALUE(EXITED)
		#undef VALUE
		default: return "";
	}
}


Connection::State Connection::GetState()
{
	return _state;
}

const char* Connection::GetStateString()
{
	return EnumToString(_state);
}

void Connection::OnOpenVPNStdOut(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	if (!error)
	{
		if (line.compare(0, 14, "****** DAEMON ") == 0)
		{
			line.erase(0, 14);
			if (_listener) _listener->OnOpenVPNCallback(process, std::move(line));
		}
		else
		{
			LOG_EX(LogLevel::VERBOSE, true, Location("OpenVPN:STDOUT")) << line;
		}
	}
	else
		LOG(WARNING) << "OpenVPN:STDOUT error: " << error;
}

void Connection::OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	if (!error)
	{
		LOG_EX(LogLevel::WARNING, true, Location("OpenVPN:STDERR")) << line;
	}
	else
		LOG(WARNING) << "OpenVPN:STDERR error: " << error;
}

static inline bool StartsWith(const std::string& str, const char* prefix)
{
	auto it = str.cbegin();
	auto end = str.cend();
	for (;;)
	{
		if (!*prefix) return true;
		if (it == end || *it != *prefix) return false;
		++it;
		++prefix;
	}
}

void Connection::OnOpenVPNManagement(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	if (error)
	{
		LOG(WARNING) << "Management line error " << error;
		return;
	}
	if (line[0] != '>')
		return;
	if (StartsWith(line, ">STATE:"))
	{
		line.erase(0, 7);
		auto params = SplitToVector(line, ',');
		if (params.size() < 2) { LOG(WARNING) << "Unrecognized OpenVPN state"; return; }

		std::string description, local_ip, remote_ip;
		if (params.size() >= 3) description = std::move(params[2]);
		if (params.size() >= 4) local_ip = std::move(params[3]);
		if (params.size() >= 5) remote_ip = std::move(params[4]);

		OpenVPNState state;
		if (!EnumFromString("OPENVPN_" + params[1], state)) { LOG(WARNING) << "Unrecognied OpenVPN state: " << params[1]; return; }
		SetOpenVPNState(state);
	}
	else if (StartsWith(line, ">HOLD:"))
	{
		process->SendManagementCommand("hold release");
	}
	else if (StartsWith(line, ">PASSWORD:"))
	{
		size_t q1 = line.find('\'', 10);
		if (q1 == line.npos) { LOG(ERROR) << "Invalid password request"; return; }
		size_t q2 = line.find('\'', q1 + 1);
		if (q2 == line.npos) { LOG(ERROR) << "Invalid password request"; return; }
		auto id = line.substr(q1 + 1, q2 - q1 - 1);
		process->SendManagementCommand(
			"username \"" + id + "\" \"" + _username + "\"\n"
			"password \"" + id + "\" \"" + _password + "\"");
	}
	else if (StartsWith(line, ">BYTECOUNT:"))
	{
		line.erase(0, 11);
		try
		{
			auto params = SplitToVector(line, ',');
			uint64_t bytes_received = std::stoll(params[0]), bytes_sent = std::stoll(params[1]);
			if (_listener) _listener->OnTrafficStatsUpdated(this, bytes_received, bytes_sent);
		}
		catch (const std::exception& e) { LOG(WARNING) << e; }		
	}
	else if (StartsWith(line, ">LOG:"))
	{
		line.erase(0, 5);
		auto params = SplitToVector(line, ',', 2);
		if (params.size() >= 3)
		{
			LogLevel level;
			if (params[1] == "F") level = LogLevel::CRITICAL;
			else if (params[1] == "N") level = LogLevel::ERROR;
			else if (params[1] == "W") level = LogLevel::WARNING;
			else if (params[1] == "I") level = LogLevel::INFO;
			else /*if (params[1] == "D")*/ level = LogLevel::VERBOSE;
			LOG_EX(level, true, Location("openvpn")) << params[2];
		}
	}
}

void Connection::SetOpenVPNState(OpenVPNState new_openvpn_state)
{
	LOG(INFO) << "OpenVPN state set to " << EnumToString(new_openvpn_state) << ", connection state is " << EnumToString(_state);

	State new_state = _state;

	switch (new_openvpn_state)
	{
		case OPENVPN_CONNECTED:
			switch (_state)
			{
				default:
					LOG(WARNING) << "OpenVPN connected in unexpected state " << EnumToString(_state);
				case CONNECTING:
				case STILL_CONNECTING:
				case RECONNECTING:
				case STILL_RECONNECTING:
					new_state = CONNECTED;
					break;
				case DISCONNECTING:
				case DISCONNECTED:
					if (_openvpn_process)
						_openvpn_process->Shutdown();
					break;
			}
			break;

		case OPENVPN_RECONNECTING:
			LOG(WARNING) << "OpenVPN reconnecting in unexpected state " << EnumToString(_state);
			++_connection_attempts;
			if (_state == CONNECTING && _connection_attempts > MAX_CONNECTION_ATTEMPTS)
				new_state = STILL_CONNECTING;
			else if (_state == RECONNECTING && _connection_attempts > MAX_RECONNECTION_ATTEMPTS)
				new_state = STILL_RECONNECTING;
			break;

		case OPENVPN_EXITING:
			if (_state == CONNECTED)
			{
				new_state = INTERRUPTED;
			}
			break;

		case OPENVPN_EXITED:
			switch (_state)
			{
				case CONNECTED:
					new_state = INTERRUPTED;
					_io.post(WEAK_CALLBACK(DoConnect));
					break;

				case DISCONNECTING_TO_RECONNECT:
					new_state = CONNECTING;
				case CONNECTING:
					ScheduleConnect(std::chrono::seconds(CONNECTION_ATTEMPT_INTERVAL));
					break;
				case STILL_CONNECTING:
					ScheduleConnect(std::chrono::seconds(SLOW_CONNECTION_ATTEMPT_INTERVAL));
					break;

				case INTERRUPTED:
					new_state = RECONNECTING;
				case RECONNECTING:
					ScheduleConnect(std::chrono::seconds(RECONNECTION_ATTEMPT_INTERVAL));
					break;
				case STILL_RECONNECTING:
					ScheduleConnect(std::chrono::seconds(SLOW_RECONNECTION_ATTEMPT_INTERVAL));
					break;

				case DISCONNECTING:
					new_state = DISCONNECTED;
					break;

				default:
					LOG(WARNING) << "OpenVPN exited in unexpected state " << EnumToString(_state);
					break;
			}
			break;

		default:
			break;
	}

	_openvpn_state = new_openvpn_state;
	SetState(new_state);
}

void Connection::SetState(State new_state)
{
	if (new_state != _state)
	{
		LOG(INFO) << "Connection state set to " << EnumToString(new_state);
		switch (new_state)
		{
			case CONNECTED:
			case DISCONNECTING_TO_RECONNECT:
			case DISCONNECTING:
			case DISCONNECTED:
				_connection_attempts = 0;
				_slow_connection_timer.cancel();
				_connection_interval_timer.cancel();
				break;
			default:
				break;
		}
		_state = new_state;
		if (_listener) _listener->OnConnectionStateChanged(this, _state);
	}
}

void Connection::StartConnectionTimer(duration timeout)
{
	_slow_connection_timer.cancel();
	_slow_connection_timer.expires_from_now(timeout);
	_slow_connection_timer.async_wait(WEAK_CALLBACK(OnSlowConnectionTimeout));
}

void Connection::OnSlowConnectionTimeout(const asio::error_code& error)
{
	switch (_state)
	{
		case CONNECTING:
			SetState(STILL_CONNECTING);
			break;
		case RECONNECTING:
			SetState(STILL_RECONNECTING);
			break;
		default:
			break;
	}
}

void Connection::ScheduleConnect(duration minimum_interval)
{
	if (clock::now() - _last_openvpn_launch < minimum_interval)
	{
		_connection_interval_timer.cancel();
		_connection_interval_timer.expires_at(_last_openvpn_launch + minimum_interval);
		_connection_interval_timer.async_wait(WEAK_LAMBDA([this](const asio::error_code& error) { if (!error) DoConnect(); }));
	}
	else
		_io.post(WEAK_CALLBACK(DoConnect));
}

void Connection::OnOpenVPNExited(OpenVPNProcess* process, const asio::error_code& error)
{
	if (process == _openvpn_process.get())
	{
		_openvpn_process.reset();
		SetOpenVPNState(OPENVPN_EXITED);
	}
}

void Connection::CopySettings()
{
	_settings.clear();
	for (auto name : g_connection_setting_names)
	{
		auto it = g_settings.map().find(name);
		if (it != g_settings.map().end())
			_settings[name] = JsonValue(it->second);
	}
	_cypherplay = g_settings.locationFlag() == "cypherplay";
	_server = g_settings.currentLocation();
	const JsonObject& login = g_account.privacy();
	_username = login.at("username").AsString();
	_password = login.at("password").AsString();

	_needsReconnect = false;
}

bool Connection::NeedsReconnect()
{
	if (!_openvpn_process)
	{
		_needsReconnect = false;
		return false;
	}

	if (_needsReconnect)
		return true;

	for (auto name : g_connection_setting_names)
	{
		auto a = _settings.find(name);
		auto b = g_settings.map().find(name);
		if (a != _settings.end())
		{
			if (b == g_settings.map().end())
				goto mismatch;
			if (*a != *b)
				goto mismatch;
		}
		else if (b != g_settings.map().end())
			goto mismatch;
	}
	if (_server != g_settings.currentLocation())
		goto mismatch;
	if (_cypherplay != (g_settings.locationFlag() == "cypherplay"))
		goto mismatch;
	return false;

mismatch:
	_needsReconnect = true;
	return true;
}

bool Connection::Connect(bool force)
{
	switch (_state)
	{
		case CONNECTING:
		case STILL_CONNECTING:
		case CONNECTED:
		case RECONNECTING:
		case STILL_RECONNECTING:
			if (!force && !NeedsReconnect())
				return false;
		case DISCONNECTING:
			SetState(DISCONNECTING_TO_RECONNECT);
			if (_openvpn_process)
				_openvpn_process->Shutdown();
			else
				_io.post(WEAK_CALLBACK(DoConnect));
			return true;
		case INTERRUPTED:
			if (_openvpn_process)
				_openvpn_process->Shutdown();
			else
				_io.post(WEAK_CALLBACK(DoConnect));
			return true;
		default:
			LOG(WARNING) << "Connecting in unhandled state " << EnumToString(_state);
		case DISCONNECTED:
			SetState(CONNECTING);
			_io.post(WEAK_CALLBACK(DoConnect));
			return true;

	}
}

void Connection::Disconnect()
{
	switch (_state)
	{
		case DISCONNECTED:
			return;
		default:
			SetState(DISCONNECTING);
			if (_openvpn_process)
				_openvpn_process->Shutdown();
			if (_openvpn_state == OPENVPN_EXITED)
				SetState(DISCONNECTED);
			break;
	}
}

void Connection::DoConnect()
{
	static int index = 0;
	//index++; // FIXME: Just make GetAvailablePort etc. work properly instead


	switch (_state)
	{
		case INTERRUPTED:
			_connection_attempts = 0;
			SetState(RECONNECTING);
			break;
		case DISCONNECTING_TO_RECONNECT:
		case DISCONNECTED:
			_connection_attempts = 0;
			SetState(CONNECTING);
			break;
		case CONNECTING:
		case RECONNECTING:
			break;
		case STILL_CONNECTING:
		case STILL_RECONNECTING:
			break;
		case CONNECTED:
			LOG(INFO) << "Already connected - DoConnect ignored";
			return;
		default:
			LOG(ERROR) << "DoConnect called in bad state " << EnumToString(_state);
			return;
	}

	if (_openvpn_process)
	{
		LOG(ERROR) << "DoConnect reached while OpenVPN process already exists";
		return;
	}

	CopySettings();
	_openvpn_process = std::make_shared<OpenVPNProcess>(_io, std::shared_ptr<OpenVPNListener>(shared_from_this(), this));

	_last_openvpn_launch = clock::now();
	_connection_interval_timer.cancel();

	if (_connection_attempts == 0)
	{
		if (_state == CONNECTING)
			StartConnectionTimer(std::chrono::seconds(MAX_CONNECTION_TIME));
		else if (_state == RECONNECTING)
			StartConnectionTimer(std::chrono::seconds(MAX_RECONNECTION_TIME));
	}

	++_connection_attempts;

	if (_listener) _listener->OnConnectionAttempt(this, _connection_attempts);

	if (_state == CONNECTING && _connection_attempts > MAX_CONNECTION_ATTEMPTS)
		SetState(STILL_CONNECTING);
	else if (_state == RECONNECTING && _connection_attempts > MAX_RECONNECTION_ATTEMPTS)
		SetState(STILL_RECONNECTING);

	int port = _openvpn_process->StartManagementInterface();

	std::vector<std::string> args;

	args.push_back("--management");
	args.push_back("127.0.0.1");
	args.push_back(std::to_string(port));
	args.push_back("--management-hold");
	args.push_back("--management-client");
	args.push_back("--management-query-passwords");

	args.push_back("--verb");
	args.push_back("4");

#if OS_WIN
	args.push_back("--dev-node");
	args.push_back(g_daemon->GetAvailableAdapter(index));
#elif OS_OSX
	args.push_back("--dev-node");
	args.push_back("utun");
#endif

#if OS_OSX
	args.push_back("--script-security");
	args.push_back("2");

	args.push_back("--up");
	args.push_back(GetPath(ScriptsDir, "up.sh"));
	args.push_back("--down");
	args.push_back(GetPath(ScriptsDir, "down.sh"));
#elif OS_LINUX
	args.push_back("--script-security");
	args.push_back("2");

	args.push_back("--up");
	args.push_back(GetPath(ScriptsDir, "updown.sh"));
	args.push_back("--down");
	args.push_back(GetPath(ScriptsDir, "updown.sh"));
#elif OS_WIN
	args.push_back("--script-security");
	args.push_back("1");
#else
	args.push_back("--script-security");
	args.push_back("1");
#endif

	args.push_back("--config");

	char profile_filename_tmp[32];
	snprintf(profile_filename_tmp, sizeof(profile_filename_tmp), "profile%d.ovpn", index);
	std::string profile_filename = GetPath(ProfileDir, EnsureExists, profile_filename_tmp);
	{
		std::ofstream f(profile_filename.c_str());
		WriteOpenVPNProfile(f);
		f.close();
	}
	args.push_back(profile_filename);

	_openvpn_process->Run(args);
	_openvpn_process->SendManagementCommand("state on\nbytecount 5\nhold release");
}

void Connection::WriteOpenVPNProfile(std::ostream& out)
{
	using namespace std;

	// Parse out protocol and remote port from remotePort setting
	std::string protocol;
	std::string remotePort;
	{
		auto both = SplitToVector(g_settings.remotePort(), ':', 1);
		protocol = std::move(both[0]);
		if (protocol == "tcp")
			protocol = "tcp-client";
		else if (protocol != "udp")
		{
			LOG(WARNING) << "Unrecognized 'remotePort' protocol '" << protocol << "', defaulting to 'udp'";
			protocol = "udp";
		}
		if (both.size() == 2)
		{
			remotePort = std::move(both[1]);
			if (remotePort == "auto")
				remotePort = "7133";
			else
			{
				try
				{
					size_t pos;
					unsigned long port = std::stoul(remotePort, &pos);
					if (pos != remotePort.size())
					{
						LOG(WARNING) << "Failed to fully parse 'remotePort' port '" << remotePort << "', using '" << port << "'";
						remotePort = std::to_string(port);
					}
				}
				catch (...)
				{
					LOG(WARNING) << "Unrecognized 'remotePort' port '" << remotePort << "', defaulting to '7133'";
					remotePort = "7133";
				}
			}
		}
	}

	if (use_stunnel)
	{
		protocol = "tcp";
	}

	// Basic settings
	out << "client" << endl;
	out << "dev tun" << endl;
	out << "proto " << protocol << endl;

	// MTU settings
	const int mtu = g_settings.mtu();
	out << "tun-mtu " << mtu << endl;
	//out << "fragment" << (mtu - 100) << endl;
	out << "mssfix " << (mtu - 220) << endl;

	// Default connection settings
	out << "ping 4" << endl;
	out << "ping-exit 15" << endl;
	out << "resolv-retry 0" << endl;
	out << "persist-remote-ip" << endl;
	//out << "persist-tun" << endl;
	out << "route-delay 0" << endl;
	out << "remap-usr1 SIGTERM" << endl;
	out << "tls-exit" << endl;

	// Default security settings
	out << "tls-cipher TLS-DHE-RSA-WITH-AES-256-GCM-SHA384:TLS-DHE-RSA-WITH-AES-256-CBC-SHA256:TLS-DHE-RSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-CBC-SHA256" << endl;
	out << "auth SHA256" << endl;
	out << "tls-version-min 1.2" << endl;
	out << "remote-cert-eku \"TLS Web Server Authentication\"" << endl;
	out << "verify-x509-name " << _server.at("ovHostname").AsString() << " name" << endl;
	out << "auth-user-pass" << endl;
	out << "ncp-disable" << endl;

	// Default route setting
	if (g_settings.routeDefault())
	{
		out << "redirect-gateway def1 bypass-dhcp";
		if (!g_settings.overrideDNS())
			out << " bypass-dns";
		// TODO: one day, redirect ipv6 traffic as well
		// out << " ipv6";
		out << endl;

		// for now, null route ipv6 instead
		out << "ifconfig-ipv6 fd25::1/64 ::1" << endl;
		out << "route-ipv6 ::/0 ::1" << endl;
	}

	// Local port setting
	if (g_settings.localPort() == 0)
		out << "nobind" << endl;
	else
		out << "lport " << g_settings.localPort() << endl;

	// Overridden encryption settings
	const auto& encryption = g_settings.encryption();
	if (encryption == "stealth")
	{
		out << "cipher AES-128-GCM" << endl;
		out << "scramble obfuscate cypherpunk-xor-key" << endl;
	}
	else if (encryption == "strong")
	{
		out << "cipher AES-256-GCM" << endl;
	}
	else if (encryption == "none")
	{
		out << "cipher none" << endl;
	}
	else // encryption == "default"
	{
		out << "cipher AES-128-GCM" << endl;
	}

	if (protocol == "udp")
	{
		// Always try to send the server a courtesy exit notification in UDP mode
		out << "explicit-exit-notify" << endl;
	}

	// Wait 15s before giving up on a connection and trying the next one
	out << "server-poll-timeout 10s" << endl;


	// Connection remotes; must come after generic connection settings
	if (use_stunnel)
	{
		// stunnel mode
		out << "<connection>" << endl;
		out << "  remote 127.0.0.1 9336" << endl;
		out << "</connection>" << endl;
		out << "route 185.176.52.35 255.255.255.255 net_gateway" << endl;
	}
	else
	{
		std::string ipKey = "ov" + encryption;
		ipKey[2] = std::toupper(ipKey[2]);
		const auto& serverIPs = _server.at(ipKey).AsArray();
		for (const auto& ip : serverIPs)
		{
			out << "<connection>" << endl;
			out << "  remote " << ip.AsString() << ' ' << remotePort << endl;
			out << "</connection>" << endl;
		}
	}

	// Extra routes; currently only used by the "exempt Apple services" setting
#if OS_OSX
	if (g_settings.exemptApple())
	{
		out << "route 17.0.0.0 255.0.0.0 net_gateway" << endl;
	}
#endif

	// Output DNS settings; these are done via dhcp-option switches,
	// and depend on the "Use Cypherpunk DNS" and related settings.

	// Tell OpenVPN to always ignore the pushed DNS (10.10.10.10),
	// which is simply there as a sensible default for dumb clients.
	out << "pull-filter ignore \"dhcp-option DNS 10.10.10.10\"" << endl;
	out << "pull-filter ignore \"route 10.10.10.10 255.255.255.255\"" << endl;

	// Ignore ping settings pushed from the server.
	out << "pull-filter ignore \"ping \"" << endl;
	out << "pull-filter ignore \"ping-restart \"" << endl;
	out << "pull-filter ignore \"ping-exit \"" << endl;

	if (g_settings.overrideDNS())
	{
		int dns_index = 10
			+ (g_settings.blockAds() ? 1 : 0)
			+ (g_settings.blockMalware() ? 2 : 0)
			+ (g_settings.optimizeDNS() || g_settings.locationFlag() == "cypherplay" ? 4 : 0);
		std::string dns_string = std::to_string(dns_index);
		out << "dhcp-option DNS 10.10.10." << dns_string << endl;
		out << "route 10.10.10." << dns_string << endl;
#if OS_LINUX
		// On Linux, simulate secondary/tertiary DNS servers in order to push any prior DNS out of the list
		out << "dhcp-option DNS 10.10.11." << dns_string << endl;
		out << "dhcp-option DNS 10.10.12." << dns_string << endl;
		out << "route 10.10.11." << dns_string << endl;
		out << "route 10.10.12." << dns_string << endl;
#elif OS_WIN
		// On Windows, add some additional convenience/robustness switches
		out << "register-dns" << endl;
		if (g_settings.routeDefault())
			out << "block-outside-dns" << endl;
#endif
	}

	// Include hardcoded certificate authority
	out << "<ca>" << endl;
	for (auto& ca : g_config.certificateAuthorities())
		for (auto& line : ca.AsArray())
			out << line.AsString() << endl;
	out << "</ca>" << endl;
}

