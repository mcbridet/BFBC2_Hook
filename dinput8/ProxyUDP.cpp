#include "pch.hpp"
#include "ProxyUDP.hpp"
#include "Config.hpp"
#include "Hook.hpp"
#include "ProxyClient.hpp"

using namespace boost;
using namespace asio;
using asio::ip::udp;
using namespace web;
using namespace web::websockets::client;

ProxyUDP::ProxyUDP(boost::asio::io_service& io_service, USHORT port) : socket_(io_service, udp::endpoint(udp::v4(), port))
{
	BOOST_LOG_FUNCTION();

	this->port = port;

	start_receive();

	BOOST_LOG_TRIVIAL(info) << "Created UDP Proxy (listening on port " << port << ")...";
}

void ProxyUDP::start_receive()
{
	socket_.async_receive_from(asio::buffer(received_data, max_length), remote_endpoint_,
		boost::bind(&ProxyUDP::handle_receive, this,
			asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void ProxyUDP::handle_receive(const system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_NAMED_SCOPE("handle_receive");

	if (!error && bytes_transferred > 0)
	{
		// Only for logging
		// Packet category (fsys, acct, etc...)
		char packet_category[HEADER_LENGTH + 1];
		memcpy(packet_category, received_data, HEADER_LENGTH);
		packet_category[HEADER_LENGTH] = '\0';

		// Packet type (NORMAL, SPLITTED, etc...)
		const unsigned int packet_type = Utils::DecodeInt(received_data + 4, 4);
		const unsigned int packet_length = Utils::DecodeInt(received_data + 8, 4);

		const auto packet_data_raw = new char[packet_length - 12];
		memcpy(packet_data_raw, received_data + 12, packet_length - 12);
		std::string packet_data = Utils::GetPacketData(packet_data_raw);

		BOOST_LOG_TRIVIAL(debug) << boost::format("-> %s %08x%08x {%s} [UDP]") % packet_category % packet_type % packet_length % packet_data;

		ProxyClient* pClient = &ProxyClient::getInstance();
		pClient->theaterCtx = this;

		try
		{
			auto packet_data = new char[bytes_transferred];
			memcpy(packet_data, received_data, bytes_transferred);

			std::string msgcontent = std::string(packet_data, bytes_transferred);
			std::vector<uint8_t> msgbuf(msgcontent.begin(), msgcontent.end());

			auto is = concurrency::streams::container_stream<std::vector<uint8_t>>::open_istream(std::move(msgbuf));

			websocket_outgoing_message msg;
			msg.set_binary_message(is);

			if (pClient->connected_to_theater)
				pClient->theater_ws.send(msg);
		}
		catch (const websocket_exception& ex)
		{
			BOOST_LOG_TRIVIAL(error) << "handle_read() - Failed to write to websocket! (" << ex.what() << ")";
			return handle_stop();
		}

		start_receive();
	}
}


void ProxyUDP::handle_send(const system::error_code& /*error*/, size_t /*bytes_transferred*/)
{
	// Done sending, do we need to do something more here? (normally we don't specifically wait for incoming udp packets)
}

void ProxyUDP::handle_stop()
{
	socket_.close();
}
