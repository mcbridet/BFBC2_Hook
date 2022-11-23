#include "pch.hpp"
#include "ConnectionPlasma.hpp"

#include "Config.hpp"
#include "Hook.hpp"

using namespace boost::asio;
using namespace ip;
using namespace web;
using namespace websockets::client;

ConnectionPlasma::ConnectionPlasma(io_service& io_service, ssl::context& context) : game_socket_(io_service, context)
{
	retailCtx = new ConnectionRetail(PLASMA,
	                                 [=](unsigned char* data, int length) { sendToGame(data, length); },
	                                 [=]() { handle_stop(); },
	                                 io_service, context);

	wsCtx = new ConnectionWebSocket(PLASMA,
	                                [=](unsigned char* data, int length) { sendToGame(data, length); },
	                                [=]() { handle_stop(false); });
}

socketSSL::lowest_layer_type& ConnectionPlasma::gameSocket()
{
	return game_socket_.lowest_layer();
}

void ConnectionPlasma::start()
{
	// Before reading stuff we have to do a handshake
	game_socket_.async_handshake(ssl::stream_base::server,
	                             bind(&ConnectionPlasma::handle_handshake, shared_from_this(), placeholders::error));
}

void ConnectionPlasma::handle_handshake(const boost::system::error_code& error)
{
	Config* config = &Config::getInstance();

	if (!error)
	{
		BOOST_LOG_TRIVIAL(debug) << "New connection, initializing...";

		if (config->hook->connectRetail)
			retailCtx->connectToRetail();
		else
			wsCtx->connect();

		connected = true;

		std::fill_n(receive_buffer, PACKET_MAX_LENGTH, 0);
		game_socket_.async_read_some(buffer(receive_buffer, PACKET_MAX_LENGTH),
		                             boost::bind(&ConnectionPlasma::handle_read, shared_from_this(),
		                                         placeholders::error, placeholders::bytes_transferred));

		BOOST_LOG_TRIVIAL(info) << "Client connected";
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Disconnected, error message: " << error.message() << ", error code: " << error.
value();
		handle_stop();
	}
}

void ConnectionPlasma::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	Config* config = &Config::getInstance();

	if (!error)
	{
		receive_length += bytes_transferred;

		const Packet packet(receive_buffer, receive_length);

		if (!packet.isValid)
		{
			game_socket_.async_read_some(buffer(receive_buffer + receive_length, PACKET_MAX_LENGTH - receive_length),
			                             bind(&ConnectionPlasma::handle_read, this, placeholders::error,
			                                  placeholders::bytes_transferred));
			return;
		}

		BOOST_LOG_TRIVIAL(debug) << boost::format("[PROXY] <- [GAME (Plasma)] %s 0x%08x (%i bytes) {%s}") % packet.service % packet.kind % packet.length % packet.data;

		if (config->hook->connectRetail)
			retailCtx->sendToRetail(receive_buffer, receive_length);
		else
			wsCtx->send(receive_buffer, receive_length);

		receive_length = 0;
		std::fill_n(receive_buffer, PACKET_MAX_LENGTH, 0);

		game_socket_.async_read_some(buffer(receive_buffer, PACKET_MAX_LENGTH),
		                             boost::bind(&ConnectionPlasma::handle_read, shared_from_this(),
		                                         placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		handle_stop();
	}
}

void ConnectionPlasma::sendToGame(unsigned char* data, int length)
{
	Packet packet(data, length);

	if (!packet.isValid)
	{
		BOOST_LOG_TRIVIAL(error) << "Tried to send invalid package! Skipping...";
		return;
	}

	write(game_socket_, buffer(data, length));
	BOOST_LOG_TRIVIAL(debug) << boost::format("[PROXY] -> [GAME (Plasma)] %s 0x%08x (%i bytes) {%s}") % packet.service % packet.kind % packet.length % packet.data;
}


void ConnectionPlasma::handle_stop(bool crash)
{
	if (wsCtx->connected)
		wsCtx->close();

	if (retailCtx->connected)
		retailCtx->close();

	if (connected)
	{
		game_socket_.lowest_layer().close();
		connected = false;
	}

	BOOST_LOG_TRIVIAL(info) << "Client Disconnected!";

	if (crash)
	{
		// Throw an exception to restart proxy
		throw ProxyStopException();
	}
}
