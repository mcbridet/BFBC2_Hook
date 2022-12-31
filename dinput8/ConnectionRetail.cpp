#include "pch.hpp"
#include "ConnectionRetail.hpp"

#include "Hook.hpp"
#include "ProxyClient.hpp"

using namespace boost::asio;
using namespace ip;
using namespace web;
using namespace websockets::client;

ConnectionRetail::ConnectionRetail(ProxyType type, std::function<void(unsigned char*, unsigned int)> sendToGame,
                                   std::function<void()> closeCallback, io_service& io_service, ssl::context& context) :
	retail_socket_ssl_(io_service, context), retail_socket_(io_service)
{
	BOOST_LOG_FUNCTION();

	this->type = type;

	this->sendToGame = sendToGame;
	this->closeCallback = closeCallback;

	Hook* hook = &Hook::getInstance();

	if (type == PLASMA)
	{
		host = hook->exeType == CLIENT ? "bfbc2-pc.fesl.ea.com" : "bfbc2-pc-server.fesl.ea.com";
		port = std::to_string(hook->exeType == CLIENT ? CLIENT_PLASMA_PORT : SERVER_PLASMA_PORT);
	}
	else
	{
		host = hook->exeType == CLIENT ? "bfbc2-pc.theater.ea.com" : "bfbc2-pc-server.theater.ea.com";
		port = std::to_string(hook->exeType == CLIENT ? CLIENT_THEATER_PORT : SERVER_THEATER_PORT);
	}
}

ConnectionRetail::~ConnectionRetail()
{
	close();
}


socketSSL::lowest_layer_type& ConnectionRetail::retailSocketSSL()
{
	return retail_socket_ssl_.lowest_layer();
}


tcp::socket& ConnectionRetail::retailSocket()
{
	return retail_socket_;
}

void ConnectionRetail::connectToRetail()
{
	BOOST_LOG_NAMED_SCOPE("Retail (Connect)")
	BOOST_LOG_TRIVIAL(warning) << "Connecting to retail server (" << host << ":" << port << ")...";

	ProxyClient* proxyClient = &ProxyClient::getInstance();

	tcp::resolver::query query(host, port);
	tcp::resolver::iterator iterator = proxyClient->tcpResolver->resolve(query);

	if (type == PLASMA)
	{
		receive_buffer = new unsigned char[PACKET_MAX_LENGTH];

		connect(retailSocketSSL(), iterator);
		retail_socket_ssl_.handshake(ssl::stream_base::client);
	}
	else
	{
		receive_buffer = new unsigned char[USHRT_MAX];

		connect(retailSocket(), iterator);
	}

	BOOST_LOG_TRIVIAL(warning) << "Connected to retail server (" << host << ":" << port << ")!";
	connected = true;

	std::fill_n(receive_buffer, PACKET_MAX_LENGTH, 0);

	if (type == PLASMA)
		retail_socket_ssl_.async_read_some(buffer(receive_buffer, PACKET_MAX_LENGTH),
		                                   bind(&ConnectionRetail::handle_read, this, placeholders::error,
		                                        placeholders::bytes_transferred));
	else
		retail_socket_.async_read_some(buffer(receive_buffer, USHRT_MAX),
		                               bind(&ConnectionRetail::handle_read, this, placeholders::error,
		                                    placeholders::bytes_transferred));
}


void ConnectionRetail::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	// Retail server sent something, send it to the game (client)
	BOOST_LOG_NAMED_SCOPE("Retail (Read)")

	if (!error && connected)
	{
		receive_length += bytes_transferred;

		if (type == PLASMA)
		{
			// Plasma packets are pretty much guaranteed to be "valid" (a.k.a we only receive one message per packet)

			// For some reason, retail server sends only one (first) byte in first packet, and the rest of the packet in second packet
			// No idea why, but before processing check if first packet is "long" enough

			if (receive_length < HEADER_LENGTH)
			{
				retail_socket_ssl_.async_read_some(
					buffer(receive_buffer + receive_length, PACKET_MAX_LENGTH - receive_length),
					bind(&ConnectionRetail::handle_read, this, placeholders::error, placeholders::bytes_transferred));
				return;
			}

			const Packet packet(receive_buffer, receive_length);

			if (!packet.isValid)
			{
				return;
			}

			BOOST_LOG_TRIVIAL(debug) << boost::format("[PROXY] <- [%s] %s 0x%08x (%i bytes) {%s}") % (host + ":" + port) % packet.service % packet.kind % packet.length % packet.data;
			sendToGame(receive_buffer, receive_length);
		}
		else
		{
			// Theater packets are not guaranteed to be "valid" (we can receive more than one message per packet)
			// If message(s) are longer than PACKET_MAX_LENGTH, the rest of messages will be sent in another packet

			unsigned int current_offset = 0;

			while (true)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
				unsigned int length = length = Utils::DecodeInt(receive_buffer + current_offset + LENGTH_OFFSET,
				                                                HEADER_VALUE_LENGTH);

				if (length == 0)
					break;


				auto packet = new Packet(receive_buffer + current_offset, length);

				if (!packet->isValid)
				{
					// Invalid packet, check if packet is *just* incomplete (so we should wait for more data)

					if (packet->isIncomplete)
					{
						BOOST_LOG_TRIVIAL(debug) << "Incomplete packet, receiving next chunk...";
						retail_socket_.read_some(buffer(receive_buffer + current_offset + packet->realLength, USHRT_MAX - current_offset - packet->realLength));
						std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // It just... makes this thing work?

						continue;
					}

					// Invalid packet
					break;
				}

				current_offset += length;

				BOOST_LOG_TRIVIAL(debug) << boost::format("[PROXY] <- [%s] %s 0x%08x (%i bytes) {%s}") % (host + ":" + port) % packet->service % packet->kind % packet->length % packet->data;
				sendToGame(receive_buffer + current_offset - length, length);
			}
		}

		receive_length = 0;

		if (type == PLASMA)
		{
			std::fill_n(receive_buffer, PACKET_MAX_LENGTH, 0);

			retail_socket_ssl_.async_read_some(buffer(receive_buffer, PACKET_MAX_LENGTH),
				bind(&ConnectionRetail::handle_read, this, placeholders::error,
					placeholders::bytes_transferred));
		}
		else
		{
			std::fill_n(receive_buffer, PACKET_MAX_LENGTH, 0);

			retail_socket_.async_read_some(buffer(receive_buffer, USHRT_MAX),
				bind(&ConnectionRetail::handle_read, this, placeholders::error,
					placeholders::bytes_transferred));
		}
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		close();
	}
}

void ConnectionRetail::sendToRetail(unsigned char* data, unsigned int length)
{
	BOOST_LOG_NAMED_SCOPE("Retail (Write)")

	Packet packet(data, length);

	if (!packet.isValid)
	{
		BOOST_LOG_TRIVIAL(error) << "Tried to send invalid package! Skipping...";
		return;
	}

	if (type == PLASMA)
		write(retail_socket_ssl_, buffer(data, length));
	else
		write(retail_socket_, buffer(data, length));

	BOOST_LOG_TRIVIAL(debug) << boost::format("[PROXY] -> [%s] %s 0x%08x (%i bytes) {%s}") % (host + ":" + port) % packet.service % packet.kind % packet.length % packet.data;
}

void ConnectionRetail::close()
{
	if (connected)
	{
		if (type == PLASMA)
			retailSocketSSL().close();
		else
			retailSocket().close();

		connected = false;
		closeCallback();
	}
}
