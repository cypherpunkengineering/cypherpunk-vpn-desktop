#include "config.h"
#include "daemon.h"
#include "openvpn.h"
#include "logger.h"
#include "path.h"

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
	, _process(nullptr)
	, _next_process(nullptr)
	, _state(STARTING)
	, _firewallMode(Disabled)
{

}

int CypherDaemon::Run()
{
	LOG(INFO) << "Running CypherDaemon built on " __TIMESTAMP__;

	g_settings.ReadFromDisk();

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
		d.AddMethod("ping", [](){});
	}

	_state = INITIALIZED;
	_ws_server.run(); // internally calls _io.run()

	return 0;
}

void CypherDaemon::RequestShutdown()
{
	// FIXME: Cleanly shut down all OpenVPN connections, then exit
	if (!_ws_server.stopped())
		_ws_server.stop();
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

void CypherDaemon::OnFirstClientConnected()
{

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

void CypherDaemon::OnOpenVPNProcessExited(OpenVPNProcess* process)
{
	if (process == _process)
	{
		_process = nullptr;
		if (_state != DISCONNECTED)
		{
			_state = DISCONNECTED;
			OnStateChanged();
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
	return std::move(result);
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
	return std::move(result);
}

JsonObject CypherDaemon::MakeStateObject()
{
	JsonObject state;
	state["state"] = GetStateString(_state);
	if (_state == CONNECTED)
	{
		state["localIP"] = _localIP;
		state["remoteIP"] = _remoteIP;
		state["bytesReceived"] = _bytesReceived;
		state["bytesSent"] = _bytesSent;
	}
	return std::move(state);
}

JsonObject CypherDaemon::MakeConfigObject()
{
	JsonObject config;
	config["servers"] = std::vector<JsonObject> {
		{ { "remote", "208.111.52.1 7133" }, { "country", "jp" }, { "name", "Tokyo 1, Japan" } },
		{ { "remote", "208.111.52.2 7133" }, { "country", "jp" }, { "name", "Tokyo 2, Japan" } },
		{ { "remote", "199.68.252.203 7133" }, { "country", "us" }, { "name", "Honolulu, HI, USA" } },
	};
	return std::move(config);
}

static const std::vector<std::string> g_certificate_authority = {
	"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",
	"dsj1LEFcf8DWaj19Dz099BxQgw=="
};

static const std::vector<std::string> g_user_certificate = {
	"MIIEwzCCAqugAwIBAgICEAMwDQYJKoZIhvcNAQEFBQAwbjELMAkGA1UEBhMCSlAx",
	"DjAMBgNVBAgMBVRva3lvMQ8wDQYDVQQHDAZNaW5hdG8xDDAKBgNVBAoMA3dpejEb",
	"MBkGA1UECwwSbmV0d29yayBvcGVyYXRpb25zMRMwEQYDVQQDDAp3aXogVlBOIENB",
	"MB4XDTE2MDUxNDA3MjU1N1oXDTI2MDUxMjA3MjU1N1owXjELMAkGA1UEBhMCSlAx",
	"DjAMBgNVBAgMBVRva3lvMQwwCgYDVQQKDAN3aXoxGzAZBgNVBAsMEm5ldHdvcmsg",
	"b3BlcmF0aW9uczEUMBIGA1UEAwwLa2ltQHdpei5iaXowggEiMA0GCSqGSIb3DQEB",
	"AQUAA4IBDwAwggEKAoIBAQDEWicZ0zyR0eZ9T87i4A/Qjff9eAXPRkVaF4cORfpE",
	"cgZbdNqFpXQNWlB/UvMmckL5V6sT7mFB/XUgBkgHq45yEzP4W5TS2XozxhP6lqfZ",
	"Mg2L1PA2nJqkOFdeLNapIVf8ksP4szs+drAzs0n8EjtBVH23oHbx3R9EV7nLhknJ",
	"OdqAf7u+two3iaG9oSq3zasnYc7jCGRpa7kxAu+iWecCJ8N+IGdNgHQKzNiZyvhK",
	"imDfqy/5JxWcS0xSyp6OzIe5XfTXIwEKNptl9hOWsg4qe9kS1KPzHz9wh9cJZ+6c",
	"SCmGBUR0KQi8mOoHb37k04cDJa5WN6UK0kpDKUS/8gxjAgMBAAGjezB5MAkGA1Ud",
	"EwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wgR2VuZXJhdGVkIENlcnRpZmlj",
	"YXRlMB0GA1UdDgQWBBRvfK5HvBuCcx39T9FWtGYOcLB46TAfBgNVHSMEGDAWgBSv",
	"ozGO00BQe0sdO46u9CukB0fyDTANBgkqhkiG9w0BAQUFAAOCAgEAiVAMfBL8xzj/",
	"O07VxbpDExCGrk5F4T7ecFqrXpq+ALSY0Zh1bQU5yo8Dq7Q5CZxkbf12gCeRja6K",
	"9De+A5xkjZGUS9f6+UEgN344I/WuttAJvGOE5WNnDiXWtG9e0rPmc67jF4DhvL+I",
	"rq8Wvq7tFO2yTblsYI/ZYe4aTYRI6L20TRM/MN+zIm+mnQHLJZknz/j6joX0J7Qb",
	"8ZNq+5ZaelMI1HE7XejZSY0VNKR+qgJcQ3LEi81ixi/RRdzdzGR6b16zZH571b24",
	"Id5pgvp9as3MTruSAmiiAUIVLqAJIRG+Q/zelfwGTe4WEQRZyR3t0lQlFw5ROirb",
	"kjSHq+Wx4TearhOki1QGEMvP9Ao6EGtaADklbGgEGpPrWeI+bx4OzJTBi4Qg8yrj",
	"rE0ASfrhdMTIONnqZh65rAhq2/QsEzg5nLYOSYDltdoz9GxBqyy4J+HIzjywhza2",
	"PT3PJ6gArTE7XAEcXGNdijV4DxHkOS1np9ObPFEYAA+L0ZLQiO1H0I2t6EeiPynP",
	"1pJfPVXMbplvzFrnOEKPaUPdW4n8jbG1bBdhJRprGK5oV/AU5clrRBW3bH3UMrkC",
	"uol9Iro4CpErNVop5ylL3P54ByCojpou/aKiQN3LL2q7EghUQfjVHppYGfRje+DI",
	"y25N8OB8yRP9OAb8rI+k1Zxj8Y8Pcgs="
};

static const std::vector<std::string> g_user_private_key = {
	"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDEWicZ0zyR0eZ9",
	"T87i4A/Qjff9eAXPRkVaF4cORfpEcgZbdNqFpXQNWlB/UvMmckL5V6sT7mFB/XUg",
	"BkgHq45yEzP4W5TS2XozxhP6lqfZMg2L1PA2nJqkOFdeLNapIVf8ksP4szs+drAz",
	"s0n8EjtBVH23oHbx3R9EV7nLhknJOdqAf7u+two3iaG9oSq3zasnYc7jCGRpa7kx",
	"Au+iWecCJ8N+IGdNgHQKzNiZyvhKimDfqy/5JxWcS0xSyp6OzIe5XfTXIwEKNptl",
	"9hOWsg4qe9kS1KPzHz9wh9cJZ+6cSCmGBUR0KQi8mOoHb37k04cDJa5WN6UK0kpD",
	"KUS/8gxjAgMBAAECggEBALb0LaTBj5lrlDFFEMeS8Qlpjx3NHNSybGJys7PX/kaS",
	"XFwROJ/4t3bNpV3N46P6KW99gXmTz2mWifDqCWmkL5kZTX5njvccDuJ4+RqwD/uv",
	"yLF3GtA4AVts5/NnIij7WamM8y8jids86hdyQkiukCniWTWlPc9FMyIR/5ulJ9Fn",
	"BUEnByNryY9cVx1CSWWQebVu/9tYJVs5Tn71Rpj0h3j9hOYeI+NROD/j4iN0LDJp",
	"uagg89F2PrhiOjM7mPJQmWbNME3whxgoN29AGo8NbbYZ2VqXeoWz3M/WxlsvP+mT",
	"W3STGxmatuObojhix43tjPn0jSt3yGrxtFE4SNVuP5kCgYEA7N2K2spxlY4NfxGm",
	"vZ0/H0z7qBk8lvFsZf1hKNZBT+lQsmxKpXH/q3dKknfitBTxpZtms81tDDM+77XG",
	"PCfOI/ELXwJG9GFao0PillWH58V0NnrPte8/cfl8Lx4138IagDHStk4PuldiQRWe",
	"F2s124wZZzQrw8/+rf0RLudB9icCgYEA1DbIWxOtASTUb1VBbXo7pLFOyoLYrJD+",
	"NPQWMoMWYQ1w/5Kd3stt9CZouv14ml/fIsyXxueFzO4k6dGxkvQDwLtC4XJWjiwX",
	"n2B/HRZ4hV4xsJ6H0M2A1mZkNUX42aDNUsR8Em6zen8cbxVBTuwY9/RsagMxDZF2",
	"lU59F+GA+WUCgYALyElr8L4Nrm9Fbt9Yd0X4jJ/IENlOuNunhx8aJO5Cx1xYQ8LC",
	"0BTjtp9jAcupIZGTp1NIhmNyQ+pRij0+KMy8RPVH2Jkm9uDHVk0jJUYJZW0OeLV0",
	"W15QkRR4U4xigQlIbzIIF4H4xvgAPM8MYyzequ1okNPMfcAxb3E3YBGL6QKBgBez",
	"EocRWHXTPiI83DS0vOp0nr8BA9+pxan2RHBZsWsfTCpOnnDeOSZWD8YqPojHAi1p",
	"ud2Nx6SOR/MQ5wrpU233u81frojsJas35JpEAyupzFTUL4jDGotXHgPRD6yGR8fh",
	"h5WrZUHd5jgFoKiGt3chheYE+zpvr1WXUWMUXQn9AoGBANkkLXfDDBjcteH5pUwD",
	"zIuLaKXwJp2Iu7Z+Vj7xcb61W/nFmqNpVJNE/QP/i0Hz3+iDiJ+xFkh/a3ceEQTa",
	"/Mt3yRZkX6JBLk8IiPL65KTwf4wkeuXQ+0LhvzyxYXhpc2BQnbFaYkWqF+C2Tt7G",
	"6sn+c32sDV/5PgCJpWUowBLy"
};

JsonObject CypherDaemon::MakeAccountObject()
{
	JsonObject account;
	account["name"] = "Cypher";
	account["email"] = "cypher@cypherpunk.com";
	account["plan"] = "free";
	account["certificate"] = std::vector<JsonValue>(g_user_certificate.begin(), g_user_certificate.end());
	account["privateKey"] = std::vector<JsonValue>(g_user_private_key.begin(), g_user_private_key.end());
	return std::move(account);
}

void CypherDaemon::OnStateChanged()
{
	// TODO: Only call if firewall-related state has changed
	ApplyFirewallSettings();
	SendToAllClients(_rpc_client.BuildNotificationData("state", MakeStateObject()));
}

void CypherDaemon::OnSettingsChanged(const std::vector<std::string>& names)
{
	std::map<std::string, JsonValue> changed;
	for (auto& name : names)
		changed[name] = JsonValue(g_settings[name]);
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
	for (auto& p : settings) g_settings.map().at(p.first);

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
}

static void WriteArraySetting(std::ostream& out, const std::string& name, const std::string& type, const std::vector<std::string>& arr)
{
	if (!arr.empty())
	{
		static const std::string dash = "-----";
		auto ensure = [&](const std::string& value, const std::string& str) {
			if (value != str) out << str << std::endl;
		};
		out << '<' << name << '>' << std::endl;
		ensure(arr.front(), dash + "BEGIN " + type + dash);
		for (const auto& line : arr)
			out << line << std::endl;
		ensure(arr.back(), dash + "END " + type + dash);
		out << '<' << '/' << name << '>' << std::endl;
	}
}


void WriteOpenVPNProfile(std::ostream& out, const JsonObject& settings)
{
	using namespace std;

	const int mtu = g_settings.mtu();

	std::map<std::string, std::string> config = {
		{ "client", "" },
		{ "nobind", "" },
		{ "dev", "tun" },
		{ "proto", g_settings.protocol() },
		{ "remote", g_settings.remoteIP() + " " + std::to_string(g_settings.remotePort()) },
		{ "tun-mtu", std::to_string(mtu) },
		//{ "fragment", std::to_string(mtu - 100) },
		{ "mssfix", std::to_string(mtu - 200) },
		{ "ping", "10" },
		{ "ping-exit", "60" },
		{ "resolv-retry", "infinite" },
		{ "cipher", g_settings.cipher() },
		{ "redirect-gateway", "def1" },
		{ "route-delay", "0" },
	};

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

	WriteArraySetting(out, "ca", "CERTIFICATE", g_certificate_authority);
	WriteArraySetting(out, "cert", "CERTIFICATE", g_user_certificate);
	WriteArraySetting(out, "key", "PRIVATE KEY", g_user_private_key);
}

bool CypherDaemon::RPC_connect()
{
	// FIXME: Shouldn't simply read raw profile parameters from the params
	const JsonObject& settings = g_settings.map();

	if (_state == CONNECTED)
	{
		// Check if we're being asked to connect to the same place
		return _process->IsSameServer(settings);
		// TODO: Support switching to different server
	}
	else if (_state != State::DISCONNECTED)
	{
		// Already busy with something else; reject request
		return false;
	}

	_bytesReceived = 0;
	_bytesSent = 0;
	_state = CONNECTING;
	OnStateChanged();

	static int index = 0;
	index++; // FIXME: Just make GetAvailablePort etc. work properly instead

	auto vpn = CreateOpenVPNProcess(_ws_server.get_io_service());

	int port = vpn->StartManagementInterface();

	_process = vpn;

	std::vector<std::string> args;

	args.push_back("--management");
	args.push_back("127.0.0.1");
	args.push_back(std::to_string(port));
	args.push_back("--management-hold");
	args.push_back("--management-client");

	args.push_back("--verb");
	args.push_back("4");

#if OS_WIN
	args.push_back("--dev-node");
	args.push_back(GetAvailableAdapter(index));
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
		WriteOpenVPNProfile(f, settings);
		f.close();
	}
	args.push_back(profile_filename);

	vpn->OnManagementResponse("HOLD", [=](const std::string& line) {
		vpn->SendManagementCommand("\nhold release\n");
	});
	vpn->OnManagementResponse("STATE", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			if (params.size() >= 2)
			{
				const auto& s = params.at(1);
				if (s == "CONNECTED")
				{
					if (_state == DISCONNECTING)
					{
						_process->SendManagementCommand("\nsignal SIGTERM\n");
						return;
					}
					if (params.size() >= 5)
					{
						_localIP = params.at(3);
						_remoteIP = params.at(4);
					}
					_state = CONNECTED;
					OnStateChanged();
				}
				else if (s == "RECONNECTING")
				{
					if (_state == CONNECTED)
					{
						_state = CONNECTING;
						OnStateChanged();
					}
				}
				else if (s == "EXITING")
				{
					_process = nullptr;
					_state = DISCONNECTED;
					OnStateChanged();
				}
			}
		}
		catch (const std::exception& e)
		{
			LOG(ERROR) << e;
			_state = DISCONNECTED;
			if (_process)
			{
				_process->Kill();
				_process = nullptr;
			}
		}
	});
	vpn->OnManagementResponse("BYTECOUNT", [=](const std::string& line) {
		try
		{
			auto params = SplitToVector(line, ',');
			_bytesReceived = std::stoll(params[0]);
			_bytesSent = std::stoll(params[1]);
			OnStateChanged();
		}
		catch (const std::exception& e) { LOG(WARNING) << e; }
	});

	vpn->Run(args);

	vpn->SendManagementCommand("\nstate on\nbytecount 5\nhold release\n");

	//vpn->SendManagementCommand("signal SIGTERM\n");
	//vpn->SendManagementCommand("exit\n");

	//vpn->StopManagementInterface();
	//delete vpn;

	return true;
}

void CypherDaemon::RPC_disconnect()
{
	if (_state == CONNECTING || _state == CONNECTED)
	{
		_state = DISCONNECTING;
		OnStateChanged();
		_process->SendManagementCommand("\nsignal SIGTERM\n");
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
