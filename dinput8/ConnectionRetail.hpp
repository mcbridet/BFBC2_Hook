#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socketSSL;

class ConnectionRetail
{
public:
	ConnectionRetail(ProxyType type, std::function<void(unsigned char*, unsigned int)> sendToGame, std::function<void()> closeCallback, boost::asio::io_service& io_service, boost::asio::ssl::context& context);

	socketSSL::lowest_layer_type& retailSocketSSL();
	boost::asio::ip::tcp::socket& retailSocket();

	void connectToRetail();
	void sendToRetail(unsigned char* data, unsigned int length);
	void close();

	bool connected = false;

private:
	ProxyType type;
	std::string host, port;

	socketSSL retail_socket_ssl_;
	boost::asio::ip::tcp::socket retail_socket_;

	unsigned char receive_buffer[PACKET_MAX_LENGTH];
	unsigned int receive_length;

	std::function<void(unsigned char*, unsigned int)> sendToGame;
	std::function<void()> closeCallback;

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
};
