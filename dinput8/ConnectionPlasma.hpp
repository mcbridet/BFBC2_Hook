#pragma once
#include "ConnectionRetail.hpp"
#include "ConnectionWebSocket.hpp"

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socketSSL;

class ConnectionPlasma : public boost::enable_shared_from_this<ConnectionPlasma>, boost::asio::noncopyable
{
public:
	ConnectionPlasma(boost::asio::io_service& io_service, boost::asio::ssl::context& context);
	typedef boost::shared_ptr<ConnectionPlasma> pointer;

	socketSSL::lowest_layer_type& gameSocket();

	void start();

private:
	ConnectionRetail* retailCtx;
	ConnectionWebSocket* wsCtx;

	socketSSL game_socket_;

	void handle_handshake(const boost::system::error_code& error);						// first sequence of packets for ssl connection
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred); // normal read
	void handle_stop(bool crash = true);												// cleanup after a disconnect

	void sendToGame(unsigned char* data, int length);

	bool connected = false;

	unsigned char receive_buffer[PACKET_MAX_LENGTH];
	unsigned int receive_length;
};
