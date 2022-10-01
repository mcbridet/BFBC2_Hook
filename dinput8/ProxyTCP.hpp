#pragma once
#include "ConnectionFESL.hpp"

class ProxyTCP : boost::asio::noncopyable
{
public:
	ProxyTCP(boost::asio::io_service& io_service, USHORT port, bool secure);

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ssl::context context_;

	int port_;
	bool secure_;

	void start_accept();
	ConnectionFESL::pointer new_fesl_connection;

	void handle_accept_fesl(const boost::system::error_code& error);
};
