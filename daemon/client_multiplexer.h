#pragma once

#include "client.h"


// Multiplexer class to run multiple listener interfaces while acting as a single interface
class ClientInterfaceMultiplexer : public ClientInterface, protected ClientInterfaceListener
{
protected:
	struct ConnectionInfo
	{
		ClientInterface* inner_interface;
		ClientConnectionHandle inner_handle;
		ConnectionInfo() {}
		ConnectionInfo(ClientInterface* i, ClientConnectionHandle h) : inner_interface(i), inner_handle(h) {}
	};
	struct InterfaceInfo
	{
		InterfaceInfo() {}
		InterfaceInfo(InterfaceInfo&&) = default;
		InterfaceInfo& operator=(InterfaceInfo&&) = default;
		std::map<ClientConnectionHandle, ClientConnectionHandle> outer_handles;
	};
	std::vector<std::shared_ptr<ClientInterface>> _inner_interfaces;
	std::map<ClientConnectionHandle, ConnectionInfo> _outer_handle_map;
	std::map<ClientInterface*, InterfaceInfo> _inner_handle_map;
	uintptr_t _outer_handle_count;

public:
	ClientInterfaceMultiplexer(asio::io_service& io)
		: ClientInterface(io), _outer_handle_count(0)
	{}

	void AddClientInterface(std::shared_ptr<ClientInterface> child)
	{
		_inner_interfaces.push_back(std::move(child));
		_inner_handle_map.insert(std::make_pair(_inner_interfaces.back().get(), InterfaceInfo()));
		_inner_interfaces.back()->SetListener(this);
	}
	template<class T, typename... Args>
	void InitializeClientInterface(Args&&... args)
	{
		AddClientInterface(std::make_shared<T>(std::forward<Args>(args)...));
	}

	ClientInterface* GetConnectionInterface(ClientConnectionHandle client) { return _outer_handle_map.at(client).inner_interface; }

	virtual void Listen() override
	{
		std::for_each(_inner_interfaces.begin(), _inner_interfaces.end(), [](const std::shared_ptr<ClientInterface>& i) { i->Listen(); });
	}
	virtual void Stop() override
	{
		std::for_each(_inner_interfaces.rbegin(), _inner_interfaces.rend(), [](const std::shared_ptr<ClientInterface>& i) { i->Stop(); });
	}
	virtual void Send(ClientConnectionHandle client, const char* data, size_t size) override
	{
		const auto& inner = _outer_handle_map.at(client);
		inner.inner_interface->Send(inner.inner_handle, data, size);
	}
	virtual void Disconnect(ClientConnectionHandle client) override
	{
		const auto& inner = _outer_handle_map.at(client);
		inner.inner_interface->Disconnect(inner.inner_handle);
	}

protected:
	virtual void OnClientConnected(ClientInterface* inner_interface, ClientConnectionHandle inner_handle) override
	{
		auto outer_handle = reinterpret_cast<ClientConnectionHandle>(++_outer_handle_count);
		bool first = _outer_handle_map.empty();
		_outer_handle_map.insert(std::make_pair(outer_handle, ConnectionInfo(inner_interface, inner_handle)));
		_inner_handle_map.at(inner_interface).outer_handles.insert(std::make_pair(inner_handle, outer_handle));
		if (_listener)
		{
			if (first) _listener->OnFirstClientConnected(this);
			_listener->OnClientConnected(this, outer_handle);
		}
	}
	virtual void OnClientDisconnected(ClientInterface* inner_interface, ClientConnectionHandle inner_handle) override
	{
		auto outer_handle = _inner_handle_map.at(inner_interface).outer_handles.at(inner_handle);
		_outer_handle_map.erase(outer_handle);
		_inner_handle_map.at(inner_interface).outer_handles.erase(inner_handle);
		bool last = _outer_handle_map.empty();
		if (_listener)
		{
			_listener->OnClientDisconnected(this, outer_handle);
			if (last) _listener->OnLastClientDisconnected(this);
		}
	}
	virtual void OnClientMessageReceived(ClientInterface* inner_interface, ClientConnectionHandle inner_handle, const std::string& message) override
	{
		auto outer_handle = _inner_handle_map.at(inner_interface).outer_handles.at(inner_handle);
		if (_listener) _listener->OnClientMessageReceived(this, outer_handle, std::move(message));
	}
};
