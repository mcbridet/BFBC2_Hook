#pragma once
#include "ConnectionRetail.hpp"
#include "ConnectionWebSocket.hpp"

class ConnectionTheater : public boost::enable_shared_from_this<ConnectionTheater>, private boost::noncopyable
{
public:
	ConnectionTheater(boost::asio::io_service& io_service, boost::asio::ssl::context& context);

	typedef boost::shared_ptr<ConnectionTheater> pointer;

	boost::asio::ip::tcp::socket& gameSocket();

	void start();

private:
	ConnectionRetail* retailCtx;
	ConnectionWebSocket* wsCtx;

	boost::asio::ip::tcp::socket game_socket_;

	unsigned char receive_buffer[PACKET_MAX_LENGTH];
	unsigned int receive_length;

	bool connected = false;

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);	// normal read (also deals with multiple packets)
	void handle_stop(bool crash = true);												// cleanup after a disconnect

	void sendToGame(unsigned char* data, int length);
};

