#include "pch.hpp"
#include "ProxyUDP.hpp"
#include "Config.hpp"
#include "Hook.hpp"
#include "ProxyClient.hpp"

using namespace boost;
using namespace asio;
using ip::udp;
using namespace web;
using namespace websockets::client;

ProxyUDP::ProxyUDP(io_service& io_service, USHORT port) : socket_(io_service, udp::endpoint(udp::v4(), port))
{
	BOOST_LOG_FUNCTION();

	this->port = port;

	start_receive();

	BOOST_LOG_TRIVIAL(info) << "Created UDP Proxy (listening on port " << port << ")...";
}

void ProxyUDP::start_receive()
{
	socket_.async_receive_from(buffer(received_data, max_length), remote_endpoint_,
	                           boost::bind(&ProxyUDP::handle_receive, this,
	                                       asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void ProxyUDP::handle_receive(const system::error_code& error, size_t bytes_transferred)
{
	if (!error && bytes_transferred > 0)
	{
		unsigned int packet_length = Utils::DecodeInt(received_data + 8, 4);

		Packet packet(received_data, packet_length);
		BOOST_LOG_TRIVIAL(debug) << format("[UDP] [PROXY] <- [GAME (Theater)] %s 0x%08x (%i bytes) {%s}") % packet.category % packet.type % packet.length % packet.data;

		ProxyClient* pClient = &ProxyClient::getInstance();
		pClient->theaterCtx = this;

		Config* config = &Config::getInstance();

		if (config->hook->connectRetail)
		{
			std::fill_n(pClient->udp_send_data, PACKET_MAX_LENGTH, 0);
			memcpy(pClient->udp_send_data, received_data, bytes_transferred);

			socket_.async_send_to(buffer(received_data, bytes_transferred), remote_endpoint_,
			                      boost::bind(&ProxyUDP::handle_send, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
		}
		else
		{
			try
			{
				auto packet_data = new char[bytes_transferred];
				memcpy(packet_data, received_data, bytes_transferred);

				std::string msgcontent = std::string(packet_data, bytes_transferred);
				std::vector<uint8_t> msgbuf(msgcontent.begin(), msgcontent.end());

				auto is = concurrency::streams::container_stream<std::vector<uint8_t>>::open_istream(std::move(msgbuf));

				websocket_outgoing_message msg;
				msg.set_binary_message(is);

				pClient->theater_ws.send(msg);
			}
			catch (const websocket_exception& ex)
			{
				BOOST_LOG_TRIVIAL(error) << "handle_read() - Failed to write to websocket! (" << ex.what() << ")";
				return handle_stop();
			}
		}

		BOOST_LOG_TRIVIAL(debug) << format("[UDP] [PROXY] -> [%s] %s 0x%08x (%i bytes) {%s}") % remote_endpoint_.address().to_string() % packet.category % packet.type % packet.length % packet.data;
		start_receive();
	}
}


void ProxyUDP::handle_send(const system::error_code& error, size_t bytes_transferred)
{
	if (error)
	{
		BOOST_LOG_TRIVIAL(error) << "handle_send() - Failed to write to UDP socket! (" << error.what() << ")";
		handle_stop();
	}
}

void ProxyUDP::handle_stop()
{
	socket_.close();
}
