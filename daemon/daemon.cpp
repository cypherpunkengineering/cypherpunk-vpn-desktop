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
	, _needsReconnect(false)
{
	/*
	// FIXME: Don't hardcode
	ServerInfo hardcoded_servers[] = {
#define SERVER(id, name, country, lat, lon, ip_default, ip_none, ip_strong, ip_stealth) { id, name, country, lat, lon, { { "default", ip_default }, { "none", ip_none }, { "strong", ip_strong }, { "stealth", ip_stealth } } }
		SERVER("freebsd-test.tokyo.vpn.cypherpunk.network", "Tokyo Test, Japan", "jp", 35.683333, 139.683333, "208.111.52.34", "208.111.52.35", "208.111.52.36", "208.111.52.37"),
		SERVER("freebsd2.tokyo.vpn.cypherpunk.network", "Tokyo 2, Japan", "jp", 35.683333, 139.683333, "208.111.52.2", "208.111.52.12", "208.111.52.22", "208.111.52.32"),
		SERVER("honolulu.vpn.cypherpunk.network", "Honolulu, HI, USA", "us", 21.3, -157.816667, "199.68.252.203", "199.68.252.203", "199.68.252.203", "199.68.252.203"),
#undef SERVER
	};
	for (auto& s : hardcoded_servers)
		_servers.emplace(std::pair<std::string, ServerInfo>(s.id, std::move(s)));
	*/
}

int CypherDaemon::Run()
{
	LOG(INFO) << "Running CypherDaemon built on " __TIMESTAMP__;

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
	RPC_disconnect();
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
	if (process == _process)
	{
		if (!error)
		{
			LOG_EX(LogLevel::VERBOSE, true, Location("OpenVPN:STDOUT")) << line;
		}
		else
			LOG(WARNING) << "OpenVPN:STDOUT error: " << error;
	}
}

void CypherDaemon::OnOpenVPNStdErr(OpenVPNProcess* process, const asio::error_code& error, std::string line)
{
	if (process == _process)
	{
		if (!error)
		{
			LOG_EX(LogLevel::WARNING, true, Location("OpenVPN:STDERR")) << line;
		}
		else
			LOG(WARNING) << "OpenVPN:STDERR error: " << error;
	}
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
	config["servers"] = g_settings.servers();
	config["regions"] = g_settings.regions();
	config["certificateAuthorities"] = GetCertificateAuthorities();
	return config;
}

static const std::vector<std::string> g_user_certificate = {
	"-----BEGIN CERTIFICATE-----",
	"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",
	"-----END CERTIFICATE-----",
//	"-----BEGIN CERTIFICATE-----",
//	"MIIEwzCCAqugAwIBAgICEAMwDQYJKoZIhvcNAQEFBQAwbjELMAkGA1UEBhMCSlAx",
//	"DjAMBgNVBAgMBVRva3lvMQ8wDQYDVQQHDAZNaW5hdG8xDDAKBgNVBAoMA3dpejEb",
//	"MBkGA1UECwwSbmV0d29yayBvcGVyYXRpb25zMRMwEQYDVQQDDAp3aXogVlBOIENB",
//	"MB4XDTE2MDUxNDA3MjU1N1oXDTI2MDUxMjA3MjU1N1owXjELMAkGA1UEBhMCSlAx",
//	"DjAMBgNVBAgMBVRva3lvMQwwCgYDVQQKDAN3aXoxGzAZBgNVBAsMEm5ldHdvcmsg",
//	"b3BlcmF0aW9uczEUMBIGA1UEAwwLa2ltQHdpei5iaXowggEiMA0GCSqGSIb3DQEB",
//	"AQUAA4IBDwAwggEKAoIBAQDEWicZ0zyR0eZ9T87i4A/Qjff9eAXPRkVaF4cORfpE",
//	"cgZbdNqFpXQNWlB/UvMmckL5V6sT7mFB/XUgBkgHq45yEzP4W5TS2XozxhP6lqfZ",
//	"Mg2L1PA2nJqkOFdeLNapIVf8ksP4szs+drAzs0n8EjtBVH23oHbx3R9EV7nLhknJ",
//	"OdqAf7u+two3iaG9oSq3zasnYc7jCGRpa7kxAu+iWecCJ8N+IGdNgHQKzNiZyvhK",
//	"imDfqy/5JxWcS0xSyp6OzIe5XfTXIwEKNptl9hOWsg4qe9kS1KPzHz9wh9cJZ+6c",
//	"SCmGBUR0KQi8mOoHb37k04cDJa5WN6UK0kpDKUS/8gxjAgMBAAGjezB5MAkGA1Ud",
//	"EwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wgR2VuZXJhdGVkIENlcnRpZmlj",
//	"YXRlMB0GA1UdDgQWBBRvfK5HvBuCcx39T9FWtGYOcLB46TAfBgNVHSMEGDAWgBSv",
//	"ozGO00BQe0sdO46u9CukB0fyDTANBgkqhkiG9w0BAQUFAAOCAgEAiVAMfBL8xzj/",
//	"O07VxbpDExCGrk5F4T7ecFqrXpq+ALSY0Zh1bQU5yo8Dq7Q5CZxkbf12gCeRja6K",
//	"9De+A5xkjZGUS9f6+UEgN344I/WuttAJvGOE5WNnDiXWtG9e0rPmc67jF4DhvL+I",
//	"rq8Wvq7tFO2yTblsYI/ZYe4aTYRI6L20TRM/MN+zIm+mnQHLJZknz/j6joX0J7Qb",
//	"8ZNq+5ZaelMI1HE7XejZSY0VNKR+qgJcQ3LEi81ixi/RRdzdzGR6b16zZH571b24",
//	"Id5pgvp9as3MTruSAmiiAUIVLqAJIRG+Q/zelfwGTe4WEQRZyR3t0lQlFw5ROirb",
//	"kjSHq+Wx4TearhOki1QGEMvP9Ao6EGtaADklbGgEGpPrWeI+bx4OzJTBi4Qg8yrj",
//	"rE0ASfrhdMTIONnqZh65rAhq2/QsEzg5nLYOSYDltdoz9GxBqyy4J+HIzjywhza2",
//	"PT3PJ6gArTE7XAEcXGNdijV4DxHkOS1np9ObPFEYAA+L0ZLQiO1H0I2t6EeiPynP",
//	"1pJfPVXMbplvzFrnOEKPaUPdW4n8jbG1bBdhJRprGK5oV/AU5clrRBW3bH3UMrkC",
//	"uol9Iro4CpErNVop5ylL3P54ByCojpou/aKiQN3LL2q7EghUQfjVHppYGfRje+DI",
//	"y25N8OB8yRP9OAb8rI+k1Zxj8Y8Pcgs=",
//	"-----END CERTIFICATE-----",
};

static const std::vector<std::string> g_user_private_key = {
	"-----BEGIN RSA PRIVATE KEY-----",
	"MIIJJwIBAAKCAgEAqF3m4RFbr0faXYSoC7Y9PYq4IviLryj85m2g2G8p3UzsCxDv",
	"f2UkEQzAhctsIEGmQuUq4o5TYB/lAWFzi3is7+QMgNe5ygZWkCpXuMZ57hqBZmRL",
	"J+o6CciHZ6rAtXZy5lCvnYCUZpuri1shuHA5dSyfNFpbxozrNHHcow9O2SX900Tv",
	"SIs2+NzlbKVDE0WTmwPTF5FKd49/Vj2ULBR8tmzCjtZl35I+XuWUfWK/DLK5DctL",
	"jgZQU8quNnYkojFb8t9ioXZImfiSNpt1x6CsErw1fpMQB2+kLrihRsgSj/vYjNc5",
	"5wzyJ2s/LkwcmQJXDEUllw98+UF2aT3rnjyiG0A8wKKuVCtssg0D/GyMptDX/4Rg",
	"a+djJBAjzK30jy6rAEgKq1WQk/2fYEQjp3lu4q4ez6YXvcGeM24joN0DKmur9DED",
	"Od6rgnvNMV2GO2PPFWIVuQwei2W2r4cKwaYHWuLYiKDZVLUELBk9CIff7E+CZemn",
	"tosg89uVhEeCPbk9hju6pGKiG9XtVB3ePXDsB9w3u3FUYiPXwjllT12zEqgFRkBb",
	"b9tUARcDpfjXUE4JRdOiRmowLJ6MJAohPUKUpss4UAWiqBUeusqzL+ZlmThIPc75",
	"TRg3HDuAs0y/DZjCmL8yKLykBQY/weEjhRx4sh9Ji1FVhkjuQC8C6wIIh/UCAwEA",
	"AQKCAgA7/fmeiMjalAfC+tnGEpGPtDYYf+eF6lzy3m1JsZKBQD97UfWEt006pgiT",
	"pABLHhlYDMBTKdOblMHM1CSPtdgpQmESJ8wTqF5/0BahyFb5+IfTLDl7Z4J2qfVV",
	"gwpXnnUii+2HeaFnTmC5ryc8yQAwOE4iIXBCN0Q307qCf5ng1iCzfwSkHLbhxhQZ",
	"umlEHK6TtbEp3KNkJsWAvUBm1IX7mpVYwBxcpYeD2NooM19P7v7xY1bwrF9C/B1H",
	"WqGDCYNx7xb94V/NPT0cKBi3oRCvPFDzYQN9ItKlszQEPJYgp+Rpiuce5QYD9br1",
	"jDlEbAkIXjsesG4fgqvmpCGoqII6D5m50rbeeLfHbKj4T7FtPsFqMHVsGDElUsUz",
	"NtRa6BCBMTszJ42zh2Jax7RK5gbJkNBvZ+jYWS+Qz8fJjVHMb5wy7gCPdr0//6qf",
	"gCVvfMDugnUjGgHHUskiQSNURUJn6KCuQjwlyOrWTJGQSXarxOs6E2L+wWmtXd2u",
	"vcDja/SUK65j5iGGUP6LYuIAOI5drgZm/cMIatMa9WlgCGaGG8UyBPDt5hItUwcM",
	"yCOCDnO4E6fZbNuW4M7ohCqHc6m10WHdCMwulD/0o9nXVdjoqa1jCSOMVs+xElb3",
	"RmFd2xBWIqGyzwUt+Da/5pVIeiRhpPQBfPZBR7qgDyHT3WNFAQKCAQEA0qfA5d1+",
	"vVXuoKidqx68KDp2HTG+UdyV1ZLJiD0rlD/WlH/O18W+JVgxt9QvI2ACrCeMeErO",
	"uinI4gAt+gpsmDFS9VfT6Pm/fPxgBU9D+pfxRvxuVvotkIlmK/0MsFmkaVnEf1jD",
	"yF8ZXq07RceT33FcbHOoW6xXYHgbMoD2sX1Qq9GDncnXM+IPzh3HtEnqCGzwzs50",
	"7abAuvwtqVrbSmfFkwceiXyQpoZVrqdADrEP5yiOxdNwyDuMz/36fajNKv44pqhB",
	"S8s0gpg4FkmG0NfcO+kSfN3ZWCgCwIrRCOQpERamPcFxlXlwGKhVlZAWf9mzLF2C",
	"hnqcU30NBObRDQKCAQEAzJvS9hO8nXd/DilTb1JpeQIepmtRQsQPuHpR2+BqC22a",
	"0SMmeybjS4psyx1dNgdWUjDKwuPSJzQb+WheC6DYeMn4vB0WPzlcmBa5jBRqiexi",
	"P7fLdmHUni7lyjKR32q9dRqJKeUoxHiHjakBq5x0MyALhXq4GiOtJi/G+PHUQ5Al",
	"xegUh2T62T4V5uPE/aKpfL9sT3Km/Ih34oScNCQjWDqwAygVyA/hsUC20kwEH4iM",
	"17Dp1+AoMA62akCZ4FvqhLE0/CAesPYc6SQ6u39HQnNPsxcCc8BPwgUBQw3dW4fz",
	"HJzlrYrR6ZNGrJ+BGHCA2/n9q4Ffx8I5BKJWUtNIiQKCAQBSIOMc+2khSjJ54qNu",
	"BtKW4IwSP8WSxuyH0u3NtwOZjfYL+XRcPZUvnB3uLMSgBxujoNusPoYwoH/YVPeX",
	"556FC53rV22gBFb61K5fA5NeTQTdhydBs9I16sux1LuwuZJXHI92ktXp9eG/PszA",
	"HNpzIBBHnCQEccGzM8BuxUbo1hGwm0O0LfBAIx/EXWnxyWt8E3UjO+zshrnXbOAk",
	"ie67KJAoDXuDYNRIiFE5ga2AzNmFZxOa3x+2gTkaEkwp17j9zRWrLCgg872qOMyN",
	"K+dq3u3XUbxKgHfvXdIM3VI4JDQ6nFj99MBi5XClvGN5py6OdALeBisQYRc3maaM",
	"xay1AoIBADUQsMz7X26jgENG+omjoREuOI7GxIOBX+ZjavmQoVAndACLkj5cXpTH",
	"6OFg0zzg+EVGvD+BYI6kWCD+LW3soFfrYeQ/0vZAxT/4nNS0stDkirKl01H3m3IP",
	"Da+8H9MG1u4ZHLvN4B3ceKOH0pQUdpqP1A0hP8Afwpdlyr/j0D5Zk0JZp4FZ8ikH",
	"jhyctAln93cQGmIchSx3pEgORojLWpNWXTHkYonJfKpA82llJ6iZ/JUwH+XKEAIu",
	"vqad7IqgrBkP8IL3PZ59pg/dQpJAN1YHnAMBk7Q7izPxolsmrGNBxg6ErpisqTZf",
	"6PN6Rrv06aajlmO1oQao26rVA3hlrakCggEAAco401QM+juVn2TP3OuwkSGSs+Gw",
	"zlUNtjMoKmfS+WMOAOC6vOQpBowuJArItP8pAaXwnr7tRSDb7An+Z9Ea6Koz6q4d",
	"zEhu/MirwvvRBOgX/iAMWx1tYEIPJZuqJnxz7Avi2Me01KH3O8f1Jk/c+mhIDoeK",
	"K87HJGnxQuMG2NWHdDsw/Rgng2+Du19XAAdagwXhmOTxJ0APcNLjUhRey75gKjBb",
	"EfGvUrYebaC7yCg57k3O5Nd+uhvSAhsYVx1mszVe7jUCWPDgpTmVFEr+L2klEafw",
	"O/6jVIMQxn3Do36n7KFC3no3HFX4iKwT3VVDhqq02O5A9CYoO64I+3xIsA==",
	"-----END RSA PRIVATE KEY-----",
//	"-----BEGIN PRIVATE KEY-----",
//	"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDEWicZ0zyR0eZ9",
//	"T87i4A/Qjff9eAXPRkVaF4cORfpEcgZbdNqFpXQNWlB/UvMmckL5V6sT7mFB/XUg",
//	"BkgHq45yEzP4W5TS2XozxhP6lqfZMg2L1PA2nJqkOFdeLNapIVf8ksP4szs+drAz",
//	"s0n8EjtBVH23oHbx3R9EV7nLhknJOdqAf7u+two3iaG9oSq3zasnYc7jCGRpa7kx",
//	"Au+iWecCJ8N+IGdNgHQKzNiZyvhKimDfqy/5JxWcS0xSyp6OzIe5XfTXIwEKNptl",
//	"9hOWsg4qe9kS1KPzHz9wh9cJZ+6cSCmGBUR0KQi8mOoHb37k04cDJa5WN6UK0kpD",
//	"KUS/8gxjAgMBAAECggEBALb0LaTBj5lrlDFFEMeS8Qlpjx3NHNSybGJys7PX/kaS",
//	"XFwROJ/4t3bNpV3N46P6KW99gXmTz2mWifDqCWmkL5kZTX5njvccDuJ4+RqwD/uv",
//	"yLF3GtA4AVts5/NnIij7WamM8y8jids86hdyQkiukCniWTWlPc9FMyIR/5ulJ9Fn",
//	"BUEnByNryY9cVx1CSWWQebVu/9tYJVs5Tn71Rpj0h3j9hOYeI+NROD/j4iN0LDJp",
//	"uagg89F2PrhiOjM7mPJQmWbNME3whxgoN29AGo8NbbYZ2VqXeoWz3M/WxlsvP+mT",
//	"W3STGxmatuObojhix43tjPn0jSt3yGrxtFE4SNVuP5kCgYEA7N2K2spxlY4NfxGm",
//	"vZ0/H0z7qBk8lvFsZf1hKNZBT+lQsmxKpXH/q3dKknfitBTxpZtms81tDDM+77XG",
//	"PCfOI/ELXwJG9GFao0PillWH58V0NnrPte8/cfl8Lx4138IagDHStk4PuldiQRWe",
//	"F2s124wZZzQrw8/+rf0RLudB9icCgYEA1DbIWxOtASTUb1VBbXo7pLFOyoLYrJD+",
//	"NPQWMoMWYQ1w/5Kd3stt9CZouv14ml/fIsyXxueFzO4k6dGxkvQDwLtC4XJWjiwX",
//	"n2B/HRZ4hV4xsJ6H0M2A1mZkNUX42aDNUsR8Em6zen8cbxVBTuwY9/RsagMxDZF2",
//	"lU59F+GA+WUCgYALyElr8L4Nrm9Fbt9Yd0X4jJ/IENlOuNunhx8aJO5Cx1xYQ8LC",
//	"0BTjtp9jAcupIZGTp1NIhmNyQ+pRij0+KMy8RPVH2Jkm9uDHVk0jJUYJZW0OeLV0",
//	"W15QkRR4U4xigQlIbzIIF4H4xvgAPM8MYyzequ1okNPMfcAxb3E3YBGL6QKBgBez",
//	"EocRWHXTPiI83DS0vOp0nr8BA9+pxan2RHBZsWsfTCpOnnDeOSZWD8YqPojHAi1p",
//	"ud2Nx6SOR/MQ5wrpU233u81frojsJas35JpEAyupzFTUL4jDGotXHgPRD6yGR8fh",
//	"h5WrZUHd5jgFoKiGt3chheYE+zpvr1WXUWMUXQn9AoGBANkkLXfDDBjcteH5pUwD",
//	"zIuLaKXwJp2Iu7Z+Vj7xcb61W/nFmqNpVJNE/QP/i0Hz3+iDiJ+xFkh/a3ceEQTa",
//	"/Mt3yRZkX6JBLk8IiPL65KTwf4wkeuXQ+0LhvzyxYXhpc2BQnbFaYkWqF+C2Tt7G",
//	"6sn+c32sDV/5PgCJpWUowBLy",
//	"-----END PRIVATE KEY-----",
};

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
	{
		if (name == "firewall")
			ApplyFirewallSettings();
		changed[name] = JsonValue(g_settings[name]);
	}

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
	if (settings.find("servers") != settings.end() || settings.find("regions") != settings.end())
		SendToAllClients(_rpc_client.BuildNotificationData("config", MakeConfigObject()));
	if (settings.find("account") != settings.end())
		SendToAllClients(_rpc_client.BuildNotificationData("account", MakeAccountObject()));
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
		{ "resolv-retry", "infinite" },
		//{ "cipher", g_settings.cipher() },
		//{ "tls-cipher", "TLS-DHE-RSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-CBC-SHA256" },
		{ "auth", "SHA256" },
		{ "redirect-gateway", "def1" },
		{ "route-delay", "0" },
		{ "tls-version-min", "1.2" },
		//{ "remote-cert-tls", "server" },
		{ "remote-cert-eku", "\"TLS Web Server Authentication\"" },
		{ "verify-x509-name", server.at("ovHostname").AsString() + " name" },
		{ "persist-tun", "" },
		{ "auth-user-pass", "" },
	};

	if (g_settings.localPort() == 0)
		config["nobind"] = "";
	else
		config["lport"] = std::to_string(g_settings.localPort());

	const auto& encryption = g_settings.encryption();
	std::string ipKey = "ov" + encryption;
	ipKey[2] = std::toupper(ipKey[2]);
	config["remote"] = server.at(ipKey).AsString() + " " + remotePort;
	if (encryption == "stealth")
	{
		config["tls-cipher"] = "TLS-DHE-RSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-CBC-SHA256";
		config["cipher"] = "AES-128-CBC";
		config["auth"] = "SHA256";
		config["scramble"] = "obfuscate cypherpunk-xor-key";
	}
	else if (encryption == "strong")
	{
		config["tls-cipher"] = "TLS-DHE-RSA-WITH-AES-256-GCM-SHA384:TLS-DHE-RSA-WITH-AES-256-CBC-SHA256";
		config["cipher"] = "AES-256-CBC";
		config["auth"] = "SHA512";
	}
	else if (encryption == "none")
	{
		config["tls-cipher"] = "TLS-DHE-RSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-CBC-SHA256";
		config["cipher"] = "none";
		config["auth"] = "SHA1";
	}
	else // encryption == "default"
	{
		config["tls-cipher"] = "TLS-DHE-RSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-CBC-SHA256";
		config["cipher"] = "AES-128-CBC";
		config["auth"] = "SHA256";
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
	out << "<cert>" << endl;
	for (auto& line : g_user_certificate)
		out << line << endl;
	out << "</cert>" << endl;
	out << "<key>" << endl;
	for (auto& line : g_user_private_key)
		out << line << endl;
	out << "</key>" << endl;
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

	// Access the region early, should trigger an exception if it doesn't exist (before we've done any state changes)
	g_settings.servers().at(g_settings.server());

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
	args.push_back("--management-query-passwords");

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
		WriteOpenVPNProfile(f, g_settings.servers().at(g_settings.server()).AsStruct());
		f.close();
	}
	args.push_back(profile_filename);

	vpn->OnManagementResponse("HOLD", [=](const std::string& line) {
		vpn->SendManagementCommand("\nhold release\n");
	});
	vpn->OnManagementResponse("PASSWORD", [=](const std::string& line) {
		LOG(INFO) << line;
		size_t q1 = line.find('\'');
		size_t q2 = line.find('\'', q1 + 1);
		auto id = line.substr(q1 + 1, q2 - q1 - 1);
		// FIXME: Obviously shouldn't be hardcoded
		vpn->SendManagementCommand("\nusername \"" + id + "\" \"test@test.test\"\n");
		vpn->SendManagementCommand("\npassword \"" + id + "\" \"test123\"\n");
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
		OnOpenVPNProcessExited(vpn);
	});

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
