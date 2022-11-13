#pragma once

class ProxyUDP
{
public:
	ProxyUDP(boost::asio::io_service& io_service, USHORT port);
	void handle_send(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/);

	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint remote_endpoint_;

private:
	bool connected_to_ws = false;
	int port;

	enum { max_length = 1024 };
	unsigned char received_data[max_length];

	void start_receive();
	void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_stop();
};

