#include "pch.hpp"
#include "ConnectionFESL.hpp"

#include "Config.hpp"
#include "Hook.hpp"
#include "ProxyClient.hpp"

using namespace boost::asio;
using namespace ip;

ConnectionFESL::ConnectionFESL(io_service& io_service, ssl::context& context) : game_socket_(io_service, context), retail_socket_(io_service, context)
{
	BOOST_LOG_FUNCTION()
}

socketSSL::lowest_layer_type& ConnectionFESL::gameSocket()
{
	return game_socket_.lowest_layer();
}

socketSSL::lowest_layer_type& ConnectionFESL::retailSocket()
{
	return retail_socket_.lowest_layer();
}

void ConnectionFESL::start()
{
	// Before reading stuff we have to do a handshake
	game_socket_.async_handshake(ssl::stream_base::server, bind(&ConnectionFESL::handle_handshake, shared_from_this(), placeholders::error));
}

void ConnectionFESL::handle_handshake(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("handle_handshake")

	Config* config = &Config::getInstance();
	Hook* hook = &Hook::getInstance();

	if (!error)
	{
		BOOST_LOG_TRIVIAL(debug) << "New connection from: " << getRemoteIp() << ":" << getRemotePort();

		if (config->hook->connectRetail)
		{
			ProxyClient* proxyClient = &ProxyClient::getInstance();

			std::string host = hook->exeType == CLIENT ? "bfbc2-pc.fesl.ea.com" : "bfbc2-pc-server.fesl.ea.com";
			std::string port = std::to_string(hook->exeType == CLIENT ? CLIENT_FESL_PORT : SERVER_FESL_PORT);

			BOOST_LOG_TRIVIAL(warning) << "Connecting to retail server (" << host << ":" << port << ")...";

			tcp::resolver::query query(host, port);
			tcp::resolver::iterator iterator = proxyClient->feslResolver->resolve(query);

			async_connect(retailSocket(), iterator, bind(&ConnectionFESL::retail_handle_connect, shared_from_this(), placeholders::error));
		}

		game_socket_.async_read_some(buffer(received_data, PACKET_MAX_LENGTH), boost::bind(&ConnectionFESL::handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Disconnected, error message: " << error.message() << ", error code: " << error.value();
	}
}

void ConnectionFESL::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_NAMED_SCOPE("handle_read")

	Config* config = &Config::getInstance();

	if (!error)
	{
		received_length = bytes_transferred;

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

		BOOST_LOG_TRIVIAL(debug) << boost::format("-> %s %08x%08x {%s}") % packet_category % packet_type % packet_length % packet_data;

		if (config->hook->connectRetail && connected_to_retail)
		{
			async_write(retail_socket_, buffer(received_data, received_length),
				boost::bind(&ConnectionFESL::retail_handle_write, shared_from_this(), placeholders::error));
		}

		game_socket_.async_read_some(buffer(received_data, PACKET_MAX_LENGTH), boost::bind(&ConnectionFESL::handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));

	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		handle_stop(false);
	}
}

void ConnectionFESL::handle_write(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("handle_write")

	if (!error)
	{
		// Packet was sent

		// Only for logging
		// Packet category (fsys, acct, etc...)
		char packet_category[HEADER_LENGTH + 1];
		memcpy(packet_category, send_data, HEADER_LENGTH);
		packet_category[HEADER_LENGTH] = '\0';

		// Packet type (NORMAL, SPLITTED, etc...)
		const unsigned int packet_type = Utils::DecodeInt(send_data + 4, 4);
		const unsigned int packet_length = Utils::DecodeInt(send_data + 8, 4);

		const auto packet_data_raw = new char[packet_length - 12];
		memcpy(packet_data_raw, send_data + 12, packet_length - 12);
		std::string packet_data = Utils::GetPacketData(packet_data_raw);

		BOOST_LOG_TRIVIAL(debug) << boost::format("<- %s %08x%08x {%s}") % packet_category % packet_type % packet_length % packet_data;
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Disconnected, error message: " << error.message() << ", error code: " << error.value();
		handle_stop(false);
	}
}

std::string ConnectionFESL::getRemoteIp()
{
	return gameSocket().remote_endpoint().address().to_string();
}

USHORT ConnectionFESL::getRemotePort()
{
	return gameSocket().remote_endpoint().port();
}

void ConnectionFESL::retail_handle_connect(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("retail_handle_connect")

	if (!error)
	{
		BOOST_LOG_TRIVIAL(info) << "Connected to retail server!";

		retail_socket_.async_handshake(ssl::stream_base::client, boost::bind(&ConnectionFESL::retail_handle_handshake, shared_from_this(), placeholders::error));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Connection to retail server failed! (" << error.message() << ")";
		handle_stop(false);
	}
}

void ConnectionFESL::retail_handle_handshake(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("retail_handle_handshake")

	if (!error)
	{
		BOOST_LOG_TRIVIAL(debug) << "Handshake with retail server was successful!";

		connected_to_retail = true;
		async_write(retail_socket_, buffer(received_data, received_length),
			boost::bind(&ConnectionFESL::retail_handle_write, shared_from_this(), placeholders::error));

		retail_socket_.async_read_some(buffer(send_data, PACKET_MAX_LENGTH), bind(&ConnectionFESL::retail_handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Handshake with retail server failed! (" << error.message() << ")";
		handle_stop(false);
	}
}

void ConnectionFESL::retail_handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_NAMED_SCOPE("retail_handle_read")

	if (!error)
	{
		send_length = bytes_transferred;

		if (send_data[(send_length + receivedDataOffset) - 1] != NULL)
		{
			receivedDataOffset += send_length;
			retail_socket_.async_read_some(buffer(send_data + receivedDataOffset, PACKET_MAX_LENGTH - receivedDataOffset), bind(&ConnectionFESL::retail_handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
			return;
		}

		async_write(game_socket_,
			buffer(send_data, send_length + receivedDataOffset),
			boost::bind(&ConnectionFESL::handle_write, shared_from_this(), placeholders::error));

		receivedDataOffset = NULL;
		retail_socket_.async_read_some(buffer(send_data, PACKET_MAX_LENGTH), bind(&ConnectionFESL::retail_handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		handle_stop(false);
	}
}

void ConnectionFESL::retail_handle_write(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("retail_handle_write")

	if (error)
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		handle_stop(false);
	}
}


void ConnectionFESL::handle_stop(bool graceful_disconnect)
{
	Config* config = &Config::getInstance();

	if (config->hook->connectRetail)
	{
		connected_to_retail = false;
		retail_socket_.shutdown();
	}

	if (!graceful_disconnect)
	{
		
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "[" << getRemoteIp() << ":" << getRemotePort() << " Disconnected!";
	}
}
