#pragma once

#include "config.h"
#include "daemon.h"
#include "logger.h"
#include "util.h"
#include <chrono>
#include <functional>
#include <list>
#include <queue>
#include <string>
#include <vector>
#include <unordered_map>
#include <asio.hpp>

#define MINIMUM_PING_INTERVAL_MS 1000
#define MAXIMUM_PING_COUNT 5

// Implemented by platform
extern unsigned short GetPingIdentifier();

class ServerPingerThinger : public std::enable_shared_from_this<ServerPingerThinger>
{
	typedef std::chrono::high_resolution_clock clock;
	typedef clock::time_point time_point;
	typedef clock::duration duration;
	typedef asio::basic_waitable_timer<clock> timer;

public:
	struct Result
	{
		std::string id;
		double average, minimum, maximum;
		size_t replies, timeouts;
	};
	typedef void Callback(std::vector<Result> results);

private:
	struct destination_t
	{
		destination_t(asio::io_service& io, std::string id, std::string ip) noexcept
			: timeout_timer(io), ip(std::move(ip))
		{
			stats.id = std::move(id);
			stats.average = stats.minimum = stats.maximum = 0;
			stats.replies = stats.timeouts = 0;
		}
		destination_t(destination_t&&) = default;
		destination_t& operator=(destination_t&&) = default;

		std::string ip;
		asio::ip::icmp::endpoint endpoint;
		timer timeout_timer;
		time_point send_time;
		unsigned short sequence_number;
		Result stats;
	};

private:
	asio::io_service& _io;
	asio::ip::icmp::resolver _resolver;
	asio::ip::icmp::socket _socket;
	static unsigned short _global_sequence_number;
	timer _global_timeout_timer;
	asio::streambuf _reply_buffer;

	std::string _id;
	std::function<Callback> _callback;
	bool _callback_called;

	size_t _reply_count, _timeout_count;

	std::list<destination_t> _destinations;
	std::queue<destination_t*> _write_queue;
	std::unordered_map<unsigned short, destination_t*> _wait_map;
	std::list<destination_t>::iterator _resolve_iterator;

public:
	ServerPingerThinger(asio::io_service& io)
		: _io(io), _resolver(io), _socket(io, asio::ip::icmp::endpoint(asio::ip::icmp::v4(), 0)), _global_timeout_timer(io), _callback_called(false)
	{
	}

	void Add(std::string id, std::string ip)
	{
		_destinations.emplace_back(_io, std::move(id), std::move(ip));
	}

	void Start(double timeout, std::function<Callback> callback)
	{
		_callback = std::move(callback);
		if (_destinations.size() > 0)
		{
			_global_timeout_timer.expires_at(std::chrono::time_point_cast<clock::duration>(clock::now() + std::chrono::duration<double>(timeout)));
			_global_timeout_timer.async_wait(THIS_CALLBACK(HandleGlobalTimeout));
			_resolve_iterator = _destinations.begin();
			StartNextResolve();
			StartReceive();
		}
		else
			Done();
	}

private:
	union request_header_t
	{
		struct
		{
			unsigned char type;
			unsigned char code;
			unsigned short checksum;
			unsigned short identifier;
			unsigned short sequence_number;
		};
		unsigned char raw[8];
	};
	union request_t
	{
		struct
		{
			request_header_t header;
			unsigned char payload[32];
		};
		unsigned char raw[40];
	};

	void StartNextResolve()
	{
		if (_resolve_iterator != _destinations.end())
		{
			asio::ip::icmp::resolver::query query(asio::ip::icmp::v4(), _resolve_iterator->ip, std::string());
			_resolver.async_resolve(query, THIS_CALLBACK(HandleResolve));
		}
	}
	void HandleResolve(const asio::error_code& error, asio::ip::icmp::resolver::iterator it)
	{
		if (_callback_called)
			return;
		auto self = shared_from_this();
		if (!error)
		{
			_resolve_iterator->endpoint = *it;
			QueueSend(&*_resolve_iterator);
			++_resolve_iterator;
		}
		StartNextResolve();
	}
	void QueueSend(destination_t* pdest)
	{
		bool first = _write_queue.empty();
		_write_queue.push(pdest);
		if (first) StartNextSend();
	}
	void StartNextSend()
	{
		if (_write_queue.empty())
		{
			LOG(WARNING) << "Exhausted ping write queue";
			return;
		}
		destination_t& dest = *_write_queue.front();
		dest.sequence_number = ++_global_sequence_number;

		request_t req;
		req.header.type = 8;
		req.header.code = 0;
		req.header.checksum = 0;
		req.header.identifier = htons(GetPingIdentifier());
		req.header.sequence_number = htons(dest.sequence_number);
		memcpy(req.payload, "abcdefghijklmnopqrstuvwabcdefghi", 32);
		unsigned int checksum = 0;
		static_assert(sizeof(req) % 2 == 0, "payload size must be a multiple of 2");
		for (int i = 0; i < sizeof(req); i += 2)
			checksum += ntohs(*reinterpret_cast<unsigned short*>(req.raw + i));
		checksum  = (checksum >> 16) + (checksum & 0xFFFF);
		checksum += (checksum >> 16);
		req.header.checksum = htons(~checksum);

		asio::streambuf ping;
		std::ostream(&ping).write(reinterpret_cast<const char*>(req.raw), sizeof(req));

		dest.send_time = clock::now();
		_socket.async_send_to(ping.data(), dest.endpoint, THIS_CALLBACK(HandleWrite));

		_wait_map[dest.sequence_number] = &dest;

		dest.timeout_timer.expires_at(dest.send_time + std::chrono::seconds(1));
		dest.timeout_timer.async_wait(std::bind(&ServerPingerThinger::HandleTimeout, shared_from_this(), std::placeholders::_1, &dest, dest.sequence_number));
	}
	void HandleWrite(const asio::error_code& error, size_t bytes_transferred)
	{
		if (_callback_called)
			return;
		auto self = shared_from_this();
		if (error)
		{
			LOG(WARNING) << "Write error pinging " << _write_queue.front()->stats.id << ": " << error << " value=" << error.value() << " category=" << error.category().name();
		}
		_write_queue.pop();
		if (!_write_queue.empty())
			StartNextSend();
		else if (_wait_map.empty())
			Done();
	}
	void StartReceive()
	{
		_reply_buffer.commit(_reply_buffer.size());
		_socket.async_receive(_reply_buffer.prepare(65536), THIS_CALLBACK(HandleReceive));
	}
	void HandleReceive(const asio::error_code& error, size_t bytes_received)
	{
		if (_callback_called)
			return;
		auto self = shared_from_this();
		_reply_buffer.commit(bytes_received);
		unsigned char ipv4[20];
		if (sizeof(ipv4) <= bytes_received)
		{
			std::istream is(&_reply_buffer);
			is.read(reinterpret_cast<char*>(ipv4), sizeof(ipv4));
			if ((ipv4[0] >> 4) == 4 && (ipv4[0] & 0xF) >= 5 && ipv4[9] == 1)
			{
				size_t options_length = (ipv4[0] & 0xF) * 4 - sizeof(ipv4);
				size_t total_length = ntohs(*reinterpret_cast<unsigned short*>(ipv4 + 2));
				if (sizeof(ipv4) + options_length <= bytes_received)
				{
					is.ignore(options_length);
					//if (total_length != bytes_received)
					//  LOG(WARNING) << "ICMP ping response length mismatch: got " << total_length << ", expected " << bytes_received;
					request_header_t header;
					if (sizeof(ipv4) + options_length + sizeof(header) <= bytes_received)
					{
						is.read(reinterpret_cast<char*>(&header), sizeof(header));
						is.ignore(total_length - sizeof(ipv4) - options_length - sizeof(header));

						//LOG(VERBOSE) << "Received ICMP type=" << (unsigned)header.type << " code=" << (unsigned)header.code << " id=" << ntohs(header.identifier) << " seq=" << ntohs(header.sequence_number);
						if (header.type == 0 && header.code == 0 && header.identifier == htons(GetPingIdentifier()))
						{
							auto it = _wait_map.find(ntohs(header.sequence_number));
							if (it != _wait_map.end())
							{
								destination_t& dest = *it->second;
								_wait_map.erase(it->first);

								std::chrono::duration<double> duration = clock::now() - dest.send_time;

								//LOG(INFO) << "Received ping reply from " << dest.stats.id << ": " << (duration.count() * 1000) << "ms";

								dest.timeout_timer.cancel();

								if (dest.stats.replies++ == 0)
									dest.stats.average = dest.stats.minimum = dest.stats.maximum = duration.count();
								else
								{
									dest.stats.average += duration.count();
									if (duration.count() < dest.stats.minimum)
										dest.stats.minimum = duration.count();
									else if (duration.count() > dest.stats.maximum)
										dest.stats.maximum = duration.count();
								}

								if (dest.stats.replies + dest.stats.timeouts < MAXIMUM_PING_COUNT)
								{
									if (MINIMUM_PING_INTERVAL_MS > 0)
									{
										dest.timeout_timer.expires_at(dest.send_time + std::chrono::milliseconds(MINIMUM_PING_INTERVAL_MS));
										dest.timeout_timer.async_wait(std::bind(THIS_CALLBACK(QueueSend), &dest));
									}
									else
										QueueSend(&dest);
								}
								else if (_write_queue.empty() && _wait_map.empty())
									Done();
							}
							else
								LOG(WARNING) << "Unrecognized ping sequence number " << ntohs(header.sequence_number);
						}
					}
					else
						LOG(INFO) << "Missing ICMP header in ping reply";
				}
				else
					LOG(INFO) << "Malformed IPv4 header in ping reply";
			}
			else
				LOG(INFO) << "Non-IPv4 packet in ping reply";
		}
		else
			LOG(INFO) << "Incomplete IPv4 packet in ping reply";
		StartReceive();
	}
	void HandleTimeout(const asio::error_code& error, destination_t* pdest, unsigned short sequence_number)
	{
		if (_callback_called)
			return;
		auto self = shared_from_this();
		destination_t& dest = *pdest;
		if (!error)
		{
			if (sequence_number == dest.sequence_number)
			{
				++dest.stats.timeouts;
				if (dest.stats.replies + dest.stats.timeouts < MAXIMUM_PING_COUNT)
				{
					if (MINIMUM_PING_INTERVAL_MS > 0)
					{
						dest.timeout_timer.expires_at(dest.send_time + std::chrono::milliseconds(MINIMUM_PING_INTERVAL_MS));
						dest.timeout_timer.async_wait(std::bind(THIS_CALLBACK(QueueSend), &dest));
					}
					else
						QueueSend(&dest);
				}
				else if (_write_queue.empty() && _wait_map.empty())
					Done();
			}
		}
	}
	void HandleGlobalTimeout(const asio::error_code& error)
	{
		if (_callback_called)
			return;
		auto self = shared_from_this();
		_socket.cancel();
		_socket.close();
		for (auto& dest : _destinations)
			dest.timeout_timer.cancel();
		Done();
	}
	void Done()
	{
		if (!_callback_called)
		{
			_callback_called = true;
			std::vector<Result> result;
			for (auto& dest : _destinations)
			{
				if (dest.stats.replies > 0)
					dest.stats.average /= dest.stats.replies;
				result.push_back(dest.stats);
			}
			_callback(std::move(result));
		}
	}
};
