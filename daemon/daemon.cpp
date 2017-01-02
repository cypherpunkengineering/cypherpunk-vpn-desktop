#include "config.h"
#include "daemon.h"
#include "openvpn.h"
#include "logger.h"
#include "path.h"
#include "version.h"

#include <thread>
#include <asio.hpp>
#include <jsonrpc-lean/server.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#ifdef WIN32
#include <direct.h> // FIXME: temporary
#else
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#endif


CypherDaemon* g_daemon = nullptr;

using namespace std::placeholders;


CypherDaemon::CypherDaemon()
	: _rpc_client(_json_handler)
	, _state(STARTING)
	, _needsReconnect(false)
{

}

int CypherDaemon::Run()
{
	LOG(INFO) << "Running CypherDaemon version v" VERSION " built on " __TIMESTAMP__;

	_ws_server.set_message_handler(std::bind(&CypherDaemon::OnReceiveMessage, this, _1, _2));
	_ws_server.set_open_handler([this](Connection c) {
		bool first = _connections.empty();
		_connections.insert(c);
		if (first) OnFirstClientConnected();
		OnClientConnected(c);
	});
	_ws_server.set_close_handler([this](Connection c) {
		_connections.erase(c);
		OnClientDisconnected(c);
		if (_connections.empty()) OnLastClientDisconnected();
	});

	{
		using namespace websocketpp::log;
		_ws_server.clear_access_channels(alevel::all);
		_ws_server.clear_error_channels(elevel::all);
		_ws_server.set_error_channels(elevel::fatal | elevel::rerror | elevel::warn);
#ifdef _DEBUG
		_ws_server.set_access_channels(alevel::access_core);
		_ws_server.set_error_channels(elevel::info | elevel::library);
#endif
	}

	_ws_server.init_asio(&_io);
	_ws_server.set_reuse_addr(true);
	_ws_server.listen(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 9337));
	_ws_server.start_accept();

	{
		auto& d = _dispatcher;
		d.AddMethod("get", &CypherDaemon::RPC_get, *this);
		d.AddMethod("connect", &CypherDaemon::RPC_connect, *this);
		d.AddMethod("disconnect", &CypherDaemon::RPC_disconnect, *this);
		d.AddMethod("applySettings", &CypherDaemon::RPC_applySettings, *this);
		d.AddMethod("setAccount", &CypherDaemon::RPC_setAccount, *this);
		d.AddMethod("ping", [](){});
	}

	_state = INITIALIZED;

	g_settings.ReadFromDisk();

	_ws_server.run(); // internally calls _io.run()

	return 0;
}

void CypherDaemon::RequestShutdown()
{
	_io.post([this]() {
		if (_process)
		{
			try
			{
				// Shut down any OpenVPN process first.
				_process->Shutdown();
				_process->AsyncWait([this](const asio::error_code& error) {
					LOG(INFO) << "Stopping message loop";
					if (!_ws_server.stopped())
						_ws_server.stop();
				});
				return;
			}
			catch (const SystemException& e)
			{
				LOG(ERROR) << e;
			}
		}
		// Either no process is running, or something went wrong above.
		if (!_ws_server.stopped())
			_ws_server.stop();
	});
}

void CypherDaemon::SendToClient(Connection connection, const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	_ws_server.send(connection, data->GetData(), data->GetSize(), websocketpp::frame::opcode::TEXT);
}

void CypherDaemon::SendToAllClients(const std::shared_ptr<jsonrpc::FormattedData>& data)
{
	for (const auto& it : _connections)
		SendToClient(it, data);
}

void CypherDaemon::SendErrorToAllClients(const std::string& name, const std::string& description)
{
	SendToAllClients(_rpc_client.BuildNotificationData("error", name, description));
}

void CypherDaemon::OnFirstClientConnected()
{
	// If the kill-switch is set to always on, trigger it when the first client connects
	if (g_settings.firewall() == "on")
		ApplyFirewallSettings();
}

void CypherDaemon::OnClientConnected(Connection c)
{
	SendToClient(c, _rpc_client.BuildNotificationData("config", MakeConfigObject()));
	SendToClient(c, _rpc_client.BuildNotificationData("account", MakeAccountObject()));
	SendToClient(c, _rpc_client.BuildNotificationData("settings", g_settings.map()));
	SendToClient(c, _rpc_client.BuildNotificationData("state", MakeStateObject()));
}

void CypherDaemon::OnClientDisconnected(Connection c)
{

}

void CypherDaemon::OnLastClientDisconnected()
{
	switch (_state)
	{
		case CONNECTED:
		case CONNECTING:
		case SWITCHING:
			_state = DISCONNECTING;
			OnStateChanged(STATE);
			_process->SendManagementCommand("signal SIGTERM");
			break;
		default:
			// If the kill-switch is set to always on, trigger it here so it can be disabled when
			// the last client disconnects (this also happens above inside OnStateChanged(STATE))
			if (g_settings.firewall() == "on")
				ApplyFirewallSettings();
			break;
	}
}

void CypherDaemon::OnReceiveMessage(Connection connection, WebSocketServer::message_ptr msg)
{
	LOG(VERBOSE) << "Received RPC message: " << msg->get_payload();
	try
	{
		auto response = _json_handler.CreateReader(msg->get_payload())->GetResponse();
		return; // this was a response to a previous server->client call; our work is done
	}
	catch (const jsonrpc::Fault&) {}

	auto writer = _json_handler.CreateWriter();
	try
	{
		auto request = _json_handler.CreateReader(msg->get_payload())->GetRequest();
		auto response = _dispatcher.Invoke(request.GetMethodName(), request.GetParameters(), request.GetId());
		try
		{
			response.ThrowIfFault();
			// Special case: the 'get' method responds with a separate notification
			if (request.GetMethodName() == "get")
			{
				SendToClient(connection, _rpc_client.BuildNotificationData(request.GetParameters().front().AsString(), response.GetResult()));
				return;
			}
			if (response.GetId().IsBoolean() && response.GetId().AsBoolean() == false)
				return; // this was a notification; no response is needed
		}
		catch (const jsonrpc::Fault& e)
		{
			LOG(ERROR) << "RPC error: " << e;
		}
		response.Write(*writer);
	}
	catch (const jsonrpc::Fault& e)
	{
		LOG(ERROR) << "RPC error: " << e;
		jsonrpc::Response(e.GetCode(), e.GetString(), jsonrpc::Value()).Write(*writer);
	}
	auto data = writer->GetData();
	if (data->GetSize() > 0)
		SendToClient(connection, data);
}

void CypherDaemon::OnOpenVPNStdOut(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	//if (process == _process.get())
	{
		if (!error)
		{
			if (line.compare(0, 14, "****** DAEMON ") == 0)
			{
				line.erase(0, 14);
				OnOpenVPNCallback(process, std::move(line));
			}
			else
			{
				LOG_EX(LogLevel::VERBOSE, true, Location("OpenVPN:STDOUT")) << line;
			}
		}
		else
			LOG(WARNING) << "OpenVPN:STDOUT error: " << error;
	}
}

void CypherDaemon::OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	//if (process == _process.get())
	{
		if (!error)
		{
			LOG_EX(LogLevel::WARNING, true, Location("OpenVPN:STDERR")) << line;
		}
		else
			LOG(WARNING) << "OpenVPN:STDERR error: " << error;
	}
}

void CypherDaemon::OnOpenVPNCallback(OpenVPNProcess* process, std::string args)
{
	LOG(INFO) << "DAEMON CALLBACK: " << args;
}

void CypherDaemon::OnOpenVPNProcessExited(OpenVPNProcess* process)
{
	if (process == _process.get())
	{
		_process.reset();
		if (_state == SWITCHING)
		{
			_io.post([this](){ DoConnect(); });
		}
		else if (_state != DISCONNECTED)
		{
			_state = DISCONNECTED;
			_needsReconnect = false;
			OnStateChanged(STATE | NEEDSRECONNECT);
		}
	}
}

int CypherDaemon::GetAvailablePort(int hint)
{
	return hint;
}

static inline const char* GetStateString(CypherDaemon::State state)
{
	switch (state)
	{
#define CASE(n) case CypherDaemon::n: return #n;
	CASE(STARTING)
	CASE(DISCONNECTED)
	CASE(CONNECTING)
	CASE(CONNECTED)
	CASE(SWITCHING)
	CASE(DISCONNECTING)
	CASE(EXITING)
	CASE(EXITED)
#undef CASE
	}
	return "";
}

static inline JsonObject FilterJsonObject(const JsonObject& obj, const std::vector<std::string>& keys)
{
	JsonObject result;
	for (auto& s : keys)
	{
		auto it = obj.find(s);
		if (it != obj.end())
			result.insert(std::make_pair(s, it->second));
	}
	return result;
}

static inline JsonObject FilterJsonObject(JsonObject&& obj, const std::vector<std::string>& keys)
{
	JsonObject result;
	for (auto& s : keys)
	{
		auto it = obj.find(s);
		if (it != obj.end())
			result.insert(std::make_pair(s, std::move(it->second)));
	}
	return result;
}

JsonObject CypherDaemon::MakeStateObject()
{
	JsonObject state;
	state["state"] = GetStateString(_state);
	state["needsReconnect"] = _needsReconnect;
	if (_state == CONNECTED)
	{
		state["localIP"] = _localIP;
		state["remoteIP"] = _remoteIP;
		state["bytesReceived"] = _bytesReceived;
		state["bytesSent"] = _bytesSent;
	}
	return state;
}

static const std::vector<std::string> g_certificate_authorities[] = { {
	"-----BEGIN CERTIFICATE-----",
	"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",
	"dsj1LEFcf8DWaj19Dz099BxQgw==",
	"-----END CERTIFICATE-----",
	}, {
	"-----BEGIN CERTIFICATE-----",
	"MIIFiTCCA3GgAwIBAgICEAAwDQYJKoZIhvcNAQELBQAwUTELMAkGA1UEBhMCSVMx",
	"HDAaBgNVBAoME0N5cGhlcnB1bmsgUGFydG5lcnMxJDAiBgNVBAMMG0N5cGhlcnB1",
	"bmsgUGFydG5lcnMgUm9vdCBDQTAeFw0xNjA5MDYxNTI5MzBaFw0yNjA5MDQxNTI5",
	"MzBaMFkxCzAJBgNVBAYTAklTMRwwGgYDVQQKDBNDeXBoZXJwdW5rIFBhcnRuZXJz",
	"MSwwKgYDVQQDDCNDeXBoZXJwdW5rIFBhcnRuZXJzIEludGVybWVkaWF0ZSBDQTCC",
	"AiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBALVblDGKSLfQr8zk37Pmbce1",
	"nJ28hkIf5HvdFUIVY+396Qfjx9YNY3pTl/Bwjb0JwT7KHnHPkLtwdRgT74mPIK1j",
	"TDX4TjDMcWSUD9Bn2BppeHHj10zhEiMGZxlDkorR00FygM+pS6A9u9ack5PHzveY",
	"AOFwNh0SW20CQk+3/Ph+CcHbNeanfNt8U2UKBygyVkRTV3sYkIL6g7GQJ9th6YAJ",
	"mg2p3kU5ZadxslQaQBcM0G9kWBWsYif0IvAjh4rs1B0BHUPZpzsR062DkYHYJeSq",
	"cenfVfByXx9CW/tC/cDhIaD9dZxPschU4rVPShy6yM6B5WjKUfAGTKWdfDG2c/6S",
	"2iELvvRj2VFuBt5XVR39c7eIIvyGcfPrMPvYTQhP5+eGL92wsMqKoxosz4ZWiIHa",
	"Mb9cGjHupJRN1qpjnFN/fwTLm14JjaHklXLXF9ojCHbSWL3aXKX0lTuFOfY7A/zx",
	"hknbCijEQ3pxKLpJY3VjokMhlGrq+BYla+mKpeRKNJ7CgsM6MEO3yiO3n4CF0ZyS",
	"1DGrDAnrAPlA2bDX2LeFNPkt0A3Vv9BV6vgcahIcIRZjs5UVYN9XmErlESXgHm97",
	"Hb5QaYSgDBA4ekEE09dtH1CWKJREdtX38z3iN4pr7XXXlF0lM+aKr9rFeHB/MiWg",
	"PzJHBzmkhwcUYXhGLsVVAgMBAAGjYzBhMB0GA1UdDgQWBBRvC1oTePuUSlByx3pE",
	"MQnjTx5MUDAfBgNVHSMEGDAWgBTjkvrWu+Pe+eyx9dI35+jHACfjbTAPBgNVHRME",
	"CDAGAQH/AgEAMA4GA1UdDwEB/wQEAwIBhjANBgkqhkiG9w0BAQsFAAOCAgEANkiw",
	"o2Lsol6a0OnK52mmVgw2Al73Iak8NP+FGiTW+BFqxeBqiz9X9nI/03Z/keVla4Nx",
	"R0ziKh4sWjSa1ik9/XmjaRQ3c/BeDncwx7R51FmoVcdBMXwYUckVvtt0JOuT2yHP",
	"NekIZfiT+nBz9BPyxvpWZqocFBjcyodtCVgTAEaM2lGwxzypAb/OEX86scjVsDWH",
	"Qwhgl+PDxjDM+LW6bnhCzpL2ZkuliP+xf0DjhADnAyRnR0CDwJO5iUb7OS/RsGId",
	"3p+NmTysyRxWwqE7cFKQdBvgztIvqViwc9a5gPi81zTGXhkuSt3I9a2l+GJtxBKZ",
	"oe9DEFdcjGw7G6+PAfqYAlArranek5ID6VjsDFTTw0LfLHRdn3zdFAlVLSso8DTl",
	"+7hADyo6labKQkWhVcZjMI3I00n5L4/b9kLs34QZCb5qLm7S420/3o9mQemJ3s70",
	"rlqV0qFzAb1TU7d5+RRjcjNoJVplRsemd5278CPggMB8kAZNbYKvdILHsGPI/6Gp",
	"VdkJxpch1U1CSD+LbliqGMvetDak5X2bjJJuYgCZO7FQJIZV6gtvOUREbKtcOM88",
	"sFL7p4bMCtrRxtDMbv7IFCZTcLin8zSgbfZ7fX2RT4sEiPqoSdVyrUw1mW+7duKk",
	"Yw4+ot2O2nGrXr87ECICAR9G2W/7FJR1NGLzHLg=",
	"-----END CERTIFICATE-----",
	}, {
	"-----BEGIN CERTIFICATE-----",
	"MIIFdTCCA12gAwIBAgIJALKRODCNuUoBMA0GCSqGSIb3DQEBCwUAMFExCzAJBgNV",
	"BAYTAklTMRwwGgYDVQQKDBNDeXBoZXJwdW5rIFBhcnRuZXJzMSQwIgYDVQQDDBtD",
	"eXBoZXJwdW5rIFBhcnRuZXJzIFJvb3QgQ0EwHhcNMTYwOTA2MTUyOTAzWhcNMzYw",
	"OTAxMTUyOTAzWjBRMQswCQYDVQQGEwJJUzEcMBoGA1UECgwTQ3lwaGVycHVuayBQ",
	"YXJ0bmVyczEkMCIGA1UEAwwbQ3lwaGVycHVuayBQYXJ0bmVycyBSb290IENBMIIC",
	"IjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAnOeqLGvOjPLxQHLHjfZptz1f",
	"9BUc+TpQsC7UbJKpG4QS1Suk0IT22Gv9daR5ckS/Guqg9qS8fLJ76dnT43QzW0+B",
	"aPDugP9DbU+GqIh7i3xLh7gUzzXw/eYbl55rxB+r0urf+NQX1ifokomUl8CD7cXj",
	"PojYbc7MO2mkG4hMCC8nZfmajj2ZFcgECpuK3ogAy4n+haDxRT6NK8Lmb7R76wmN",
	"vn4CoMGAZtkiQA4xTL5Um3yAKJktCMJAigr2tEzq5aV6taMBcBbzU1OiXeRolBM0",
	"fzq3MkFehj/xL6uCIhe+oIiy5OmFlxIxOkqFlzp1dsmTl6gU0XYrVjDhIGhqYSjs",
	"SH5zSwH8Lxkq9uHvElRcIT3mXDmVQ6Wt8jYqbj/3kWl7jSajY2bDHrn5bjEXzgyq",
	"FNVymXJCnOu9T19tvMAE0W7Cocmad1nL+BzzVaw9B2KjhRgJbl/OT4YAl5GmD9+x",
	"W35Pq4LuSHvui/5Zvb+KZeS1ir4sW1fR2H6p5X0gO5MO7nPqnYUG2BlUDWjT7dHL",
	"aE7BR/nkxpPzJ2h0DdoGZY51QiUtsbiSOYU+YOsxIm696DtCilGZjSa6fMVRD2xT",
	"E2VQ3kUMJQvRUaVD/jdFyh1JpxG/YDciA0r71n/qhgiXcNb9W23lGazdfwQJRhP/",
	"NcVirTJVBMiV2FHzdnUCAwEAAaNQME4wHQYDVR0OBBYEFOOS+ta749757LH10jfn",
	"6McAJ+NtMB8GA1UdIwQYMBaAFOOS+ta749757LH10jfn6McAJ+NtMAwGA1UdEwQF",
	"MAMBAf8wDQYJKoZIhvcNAQELBQADggIBAJswZmMiXxRz5dG6UP3nNTTSJOLyXXiT",
	"JJz2uhQtCXfVakaff5VucSctIq8AoAd/fPueBlJ91lpBDff/e0GEHH3QeRna/VuE",
	"hMqf00kVLxpuco+1/vgZeOZX+4zGtHbeqyktZdHQfXnvIaFA2O9Yo7PSd4adOfCu",
	"8wSJhVQO5SvdlLgfYC0a248QQucI/9AK9KLDTbu8PRYuAjrgTR7k//Ok9s8XCySX",
	"DCaiN3aHwpPN7YC55BATDZYwAmD8ZKa+JRQgQpSlaXN09lL38OkMvLraZ/VPJhOI",
	"YaZjhFyjawyKUJ1bAywm6S1IvFWa8wu3GjDQNzy0W2RXYDXjs1LfTa0HjAXLukA9",
	"noJ41RjLje45BdS1A4DQAVqKjyu385wXU5B2Fb5mFgsavU4Z8WLTi52dqaWX164d",
	"rvLQsvDqUp1Niq064WiEsWQqiFIYcKyBJoBgZALeTQ9s/yTLf8b1GLZ/4sjLly0M",
	"/YjzvlJIHzZizA/ROB5OHiCUrsluoReUlMO93dOVXApkTR1ve0cn7XSV3btVhoO/",
	"iSUzvMksH+3tN26HaEpa8e0oMs3+AhgYLqewtEpBh+3BQBdmBghRJJxR+QOkb4me",
	"hqHTqGsy8pZ6ir3Ro2A0jVuB28bxWzLMERP5eCNkhET37LOEio6YK9DsqdLphX7W",
	"Y8gMhSbb7NTB",
	"-----END CERTIFICATE-----",
} };

static std::vector<JsonValue> GetCertificateAuthorities()
{
	std::vector<JsonValue> result;
	for (auto& ca : g_certificate_authorities)
	{
		result.push_back(std::vector<JsonValue>(ca.begin(), ca.end()));
	}
	return result;
}

JsonObject CypherDaemon::MakeConfigObject()
{
	JsonObject config;
	config["locations"] = g_settings.locations();
	config["regions"] = g_settings.regions();
	config["certificateAuthorities"] = GetCertificateAuthorities();
	return config;
}

JsonObject CypherDaemon::MakeAccountObject()
{
	try
	{
		return g_settings.at("account").AsStruct();
	}
	catch (...)
	{
		return JsonObject();
	}
}

void CypherDaemon::OnStateChanged(unsigned int flags)
{
	if (flags & STATE)
		ApplyFirewallSettings();
	SendToAllClients(_rpc_client.BuildNotificationData("state", MakeStateObject()));
}

void CypherDaemon::OnSettingsChanged(const std::vector<std::string>& names)
{
	std::map<std::string, JsonValue> changed;
	bool firewall_changed = false;
	for (auto& name : names)
	{
		if (name == "firewall" || name == "allowLAN")
			firewall_changed = true;
		changed[name] = JsonValue(g_settings[name]);
	}
	if (firewall_changed)
		ApplyFirewallSettings();
	SendToAllClients(_rpc_client.BuildNotificationData("settings", changed));
}

JsonObject CypherDaemon::RPC_get(const std::string& type)
{
	if (type == "state")
		return MakeStateObject();
	if (type == "settings")
		return g_settings.map();
	if (type == "account")
		return MakeAccountObject();
	if (type == "config")
		return MakeConfigObject();
	throw jsonrpc::InvalidParametersFault();
}

void CypherDaemon::RPC_applySettings(const JsonObject& settings)
{
	// Look up all keys first to throw if there are any invalid settings
	// before we actually make any changes
	//for (auto& p : settings) g_settings.map().at(p.first);

	std::vector<std::string> changed;
	for (auto& p : settings)
	{
		if (g_settings[p.first] != p.second)
		{
			// FIXME: Validate type
			changed.push_back(p.first);
			g_settings[p.first] = JsonValue(p.second);
		}
	}
	if (changed.size() > 0)
		g_settings.OnChanged(changed);

	// FIXME: Temporary workaround, these should be configs instead
	if (settings.find("locations") != settings.end() || settings.find("regions") != settings.end())
		SendToAllClients(_rpc_client.BuildNotificationData("config", MakeConfigObject()));
	if (settings.find("account") != settings.end())
		SendToAllClients(_rpc_client.BuildNotificationData("account", MakeAccountObject()));

	if (!_needsReconnect && (_state == CONNECTING || _state == CONNECTED) && !_process->IsSameServer(g_settings.map()))
	{
		_needsReconnect = true;
		SendToAllClients(_rpc_client.BuildNotificationData("state", JsonObject({{ "needsReconnect", JsonValue(true) }})));
	}
}

void CypherDaemon::RPC_setAccount(const JsonObject& account)
{
	RPC_applySettings({{ "account", account }});
}

void WriteOpenVPNProfile(std::ostream& out, const JsonObject& server)
{
	using namespace std;

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

	const int mtu = g_settings.mtu();

	std::map<std::string, std::string> config = {
		{ "client", "" },
		//{ "nobind", "" },
		{ "dev", "tun" },
		{ "proto", protocol },
		{ "tun-mtu", std::to_string(mtu) },
		//{ "fragment", std::to_string(mtu - 100) },
		{ "mssfix", std::to_string(mtu - 220) },
		{ "ping", "10" },
		{ "ping-exit", "60" },
		//{ "resolv-retry", "infinite" },
		//{ "cipher", g_settings.cipher() },
		{ "tls-cipher", "TLS-DHE-RSA-WITH-AES-256-GCM-SHA384:TLS-DHE-RSA-WITH-AES-256-CBC-SHA256:TLS-DHE-RSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-CBC-SHA256"},
		{ "auth", "SHA256" },
		{ "route-delay", "0" },
		{ "tls-version-min", "1.2" },
		//{ "remote-cert-tls", "server" },
		{ "remote-cert-eku", "\"TLS Web Server Authentication\"" },
		{ "verify-x509-name", server.at("ovHostname").AsString() + " name" },
		//{ "persist-tun", "" },
		{ "auth-user-pass", "" },
	};

	if (g_settings.routeDefault())
		config["redirect-gateway"] = "def1";

	if (g_settings.localPort() == 0)
		config["nobind"] = "";
	else
		config["lport"] = std::to_string(g_settings.localPort());

	const auto& encryption = g_settings.encryption();
	std::string ipKey = "ov" + encryption;
	ipKey[2] = std::toupper(ipKey[2]);
	config["remote"] = server.at(ipKey).AsArray()[0].AsString() + " " + remotePort;
	if (encryption == "stealth")
	{
		config["ncp-ciphers"] = "AES-128-GCM:AES-128-CBC";
		config["scramble"] = "obfuscate cypherpunk-xor-key";
	}
	else if (encryption == "strong")
	{
		config["ncp-ciphers"] = "AES-256-GCM:AES-256-CBC";
	}
	else if (encryption == "none")
	{
		config["cipher"] = "none";
		config["ncp-disable"] = "";
	}
	else // encryption == "default"
	{
		config["ncp-ciphers"] = "AES-128-GCM:AES-128-CBC";
	}

#if OS_OSX
	if (g_settings.exemptApple())
	{
		config["route"] = "17.0.0.0 255.0.0.0 net_gateway";
	}
#endif

	config["pull-filter ignore \"dhcp-option DNS\""] = "";
	if (g_settings.overrideDNS())
	{
		int dns_index = 10
			+ (g_settings.blockAds() ? 1 : 0)
			+ (g_settings.blockMalware() ? 2 : 0);
		std::string dns_ip = "10.10.10." + std::to_string(dns_index);
		config["dhcp-option DNS"] = dns_ip;
#ifdef OS_LINUX
		// XXX: temp hack workaround to force all 3 nameservers to 10.10.10.10 to prevent leaking
		config["dhcp-option  DNS"] = dns_ip;
		config["dhcp-option   DNS"] = dns_ip;
#endif
#if OS_WIN
		config["register-dns"] = "";
		config["block-outside-dns"] = "";
#endif
	}

	// FIXME: Manually translate other settings to OpenVPN parameters
	/*
	for (const auto& e : settings)
	{
		switch (e.second.GetType())
		{
		case jsonrpc::Value::Type::BOOLEAN:
			config[e.first] = e.second.AsBoolean() ? "true" : "false";
			break;
		case jsonrpc::Value::Type::DOUBLE:
			config[e.first] = std::to_string(e.second.AsDouble());
			break;
		case jsonrpc::Value::Type::INTEGER_32:
			config[e.first] = std::to_string(e.second.AsInteger32());
			break;
		case jsonrpc::Value::Type::INTEGER_64:
			config[e.first] = std::to_string(e.second.AsInteger64());
			break;
		case jsonrpc::Value::Type::NIL:
			config[e.first] = "";
			break;
		case jsonrpc::Value::Type::STRING:
			config[e.first] = e.second.AsString();
			break;
		default:
			break;
		}
	}
	*/

	for (const auto& e : config)
	{
		out << e.first;
		if (!e.second.empty())
			out << ' ' << e.second;
		out << endl;
	}

	out << "<ca>" << endl;
	for (auto& ca : g_certificate_authorities)
		for (auto& line : ca)
			out << line << endl;
	out << "</ca>" << endl;
}

bool CypherDaemon::RPC_connect()
{
	// FIXME: Shouldn't simply read raw profile parameters from the params
	const JsonObject& settings = g_settings.map();

	// Access the region early, should trigger an exception if it doesn't exist (before we've done any state changes)
	g_settings.locations().at(g_settings.location());

	{
		static const int MAX_RECENT_ITEMS = 3;
		auto recent = g_settings.recent();
		recent.erase(std::remove(recent.begin(), recent.end(), g_settings.location()), recent.end());
		recent.insert(recent.begin(), g_settings.location());
		if (recent.size() > MAX_RECENT_ITEMS)
			recent.resize(MAX_RECENT_ITEMS);
		g_settings.recent(recent);
		g_settings.OnChanged({ "recent" });
	}

	switch (_state)
	{
		case CONNECTED:
		case CONNECTING:
			// Reconnect only if settings have changed
			if (!_needsReconnect && _process->IsSameServer(settings))
			{
				LOG(INFO) << "No need to reconnect; new server is the same as old";
				return true;
			}
			// fallthrough
		case DISCONNECTING:
			// Reconnect
			_process->stale = true;
			_bytesReceived = _bytesSent = 0;
			_state = SWITCHING;
			OnStateChanged(STATE | BYTECOUNT);
			_process->SendManagementCommand("signal SIGTERM");
			return true;

		case SWITCHING:
			// Already switching; make sure connection is marked as stale
			_process->stale = true;
			return true;

		case DISCONNECTED:
			// Connect normally.
			_bytesReceived = _bytesSent = 0;
			_state = CONNECTING;
			OnStateChanged(STATE | BYTECOUNT);
			_io.post([this](){ DoConnect(); });
			return true;

		default:
			// Can't connect under other circumstances.
			return false;
	}
}

void CypherDaemon::DoConnect()
{
	static int index = 0;
	//index++; // FIXME: Just make GetAvailablePort etc. work properly instead

	_needsReconnect = false;
	OnStateChanged(NEEDSRECONNECT);

	std::shared_ptr<OpenVPNProcess> vpn(CreateOpenVPNProcess(_ws_server.get_io_service()));
	vpn->SetSettings(g_settings.map());

	int port = vpn->StartManagementInterface();

	_process = vpn;

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
	args.push_back(GetAvailableAdapter(index));
#elif OS_OSX
	args.push_back("--dev-node");
	args.push_back("utun");
#endif

#if OS_OSX
	args.push_back("--script-security");
	args.push_back("2");

	args.push_back("--up");
	args.push_back(GetPath(ScriptsDir, "up.sh") + " -9 -d -f -m -w -pradsgnwADSGNW");
	args.push_back("--down");
	args.push_back(GetPath(ScriptsDir, "down.sh") + " -9 -d -f -m -w -pradsgnwADSGNW");
	args.push_back("--route-pre-down");
	args.push_back(GetPath(ScriptsDir, "route-pre-down.sh"));
	args.push_back("--tls-verify");
	args.push_back(GetPath(ScriptsDir, "tls-verify.sh"));
	args.push_back("--ipchange");
	args.push_back(GetPath(ScriptsDir, "ipchange.sh"));
	args.push_back("--route-up");
	args.push_back(GetPath(ScriptsDir, "route-up.sh"));
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

#if OS_WIN
	char profile_basename[32];
	snprintf(profile_basename, sizeof(profile_basename), "profile%d.ovpn", index);
	_mkdir(GetPath(ProfileDir).c_str());
	std::string profile_filename = GetPath(ProfileDir, profile_basename);
#else
	std::string profile_filename = "/tmp/profile.XXXXXX";
	mktemp(&profile_filename[0]);
#endif
	{
		std::ofstream f(profile_filename.c_str());
		WriteOpenVPNProfile(f, g_settings.locations().at(g_settings.location()).AsStruct());
		f.close();
	}
	args.push_back(profile_filename);

	vpn->OnManagementResponse("HOLD", [=](const std::string& line) {
		vpn->SendManagementCommand("hold release");
	});
	vpn->OnManagementResponse("PASSWORD", [=](const std::string& line) {
		LOG(INFO) << line;
		size_t q1 = line.find('\'');
		size_t q2 = line.find('\'', q1 + 1);
		auto id = line.substr(q1 + 1, q2 - q1 - 1);
		vpn->SendManagementCommand(
			"username \"" + id + "\" \"" + vpn->_username + "\"\n"
			"password \"" + id + "\" \"" + vpn->_password + "\"");
	});
	vpn->OnManagementResponse("STATE", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			if (params.size() >= 2)
			{
				const auto& s = params.at(1);
				if (_state == SWITCHING)
				{
					if (s == "CONNECTED")
					{
						// Default handling below
					}
					else if (s == "EXITING")
					{
						if (_process->stale)
						{
							_process.reset();
							DoConnect();
							return;
						}
						// Default handling below
					}
					else
						return;
				}
				if (s == "CONNECTED")
				{
					if (_state == DISCONNECTING)
					{
						_process->SendManagementCommand("signal SIGTERM");
						return;
					}
					if (params.size() >= 5)
					{
						_localIP = params.at(3);
						_remoteIP = params.at(4);
					}
					_state = CONNECTED;
					OnStateChanged(STATE | IPADDRESS);
				}
				else if (s == "RECONNECTING")
				{
					if (_state == CONNECTED)
					{
						_state = CONNECTING;
						OnStateChanged(STATE);
					}
				}
				else if (s == "EXITING")
				{
					_process.reset();
					_state = DISCONNECTED;
					_needsReconnect = false;
					OnStateChanged(STATE | NEEDSRECONNECT);
				}
			}
		}
		catch (const std::exception& e)
		{
			LOG(ERROR) << e;
			_state = DISCONNECTED;
			_needsReconnect = false;
			if (_process)
			{
				_process->Kill();
				_process.reset();
			}
		}
	});
	vpn->OnManagementResponse("BYTECOUNT", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			_bytesReceived = std::stoll(params[0]);
			_bytesSent = std::stoll(params[1]);
			OnStateChanged(BYTECOUNT);
		}
		catch (const std::exception& e) { LOG(WARNING) << e; }
	});
	vpn->OnManagementResponse("LOG", [=](const std::string& line) {
		auto params = SplitToVector(line, ',', 2);
		LogLevel level;
		if (params[1] == "F") level = LogLevel::CRITICAL;
		else if (params[1] == "N") level = LogLevel::ERROR;
		else if (params[1] == "W") level = LogLevel::WARNING;
		else if (params[1] == "I") level = LogLevel::INFO;
		else /*if (params[1] == "D")*/ level = LogLevel::VERBOSE;
		LOG_EX(level, true, Location("openvpn")) << params[2];
	});

	vpn->Run(args);
	vpn->AsyncWait([this, vpn](const asio::error_code& error) {
		OnOpenVPNProcessExited(vpn.get());
	});

	vpn->SendManagementCommand("state on\nbytecount 5\nhold release");
}

void CypherDaemon::RPC_disconnect()
{
	if (_state == CONNECTING || _state == CONNECTED || _state == SWITCHING)
	{
		_state = DISCONNECTING;
		OnStateChanged(STATE);
		_process->SendManagementCommand("signal SIGTERM");
	}
}

template<typename T>
static inline const T& GetMember(const jsonrpc::Value::Struct& obj, const char* name, const T& default_value)
{
	try
	{
		return obj.at(name).AsType<T>();
	}
	catch (...)
	{
		return default_value;
	}
}

/*
bool CypherDaemon::RPC_setFirewall(const jsonrpc::Value::Struct& params)
{
	FirewallFlags flags = Nothing;

	auto mode = GetMember<std::string>(params, "mode", "off");
	auto allowLAN = GetMember<bool>(params, "allowLAN", true);
	if (mode != g_settings.killswitchMode() || allowLAN != g_settings.allowLAN())
	{
		g_settings.killswitchMode(mode);
		g_settings.allowLAN(allowLAN);
		g_settings.OnChanged();
		ApplyFirewallSettings();
	}

	return true;
}
*/
