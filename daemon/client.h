#pragma once

#include "config.h"

#include <memory>
#include <set>
#include <map>

#include <asio.hpp>


class ClientInterface;
class ClientInterfaceListener;
typedef struct ClientConnection *ClientConnectionHandle;

class ClientInterface : public std::enable_shared_from_this<ClientInterface>
{
protected:
	asio::io_service& _io;
	ClientInterfaceListener* _listener;

	ClientInterface(asio::io_service& io) : _io(io), _listener(nullptr) {}
	virtual ~ClientInterface() {}

public:
	virtual void Listen() = 0;
	virtual void Stop() = 0;
	virtual void Send(ClientConnectionHandle client, const char* data, size_t size) = 0;
	virtual void Disconnect(ClientConnectionHandle client) = 0;

	void SetListener(ClientInterfaceListener* listener) { _listener = listener; }
};

class ClientInterfaceListener
{
public:
	virtual void OnFirstClientConnected(ClientInterface* client_interface) {}
	virtual void OnClientConnected(ClientInterface* client_interface, ClientConnectionHandle client) {}
	virtual void OnClientMessageReceived(ClientInterface* client_interface, ClientConnectionHandle client, const std::string& message) {}
	virtual void OnClientDisconnected(ClientInterface* client_interface, ClientConnectionHandle client) {}
	virtual void OnLastClientDisconnected(ClientInterface* client_interface) {}
};

typedef std::set<ClientConnectionHandle> ClientConnectionList;
template<typename T> using ClientConnectionMap = std::map<ClientConnectionHandle, T>;
