#pragma once
#include "ConnectionPlasma.hpp"

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
	ConnectionPlasma::pointer new_plasma_connection;

	void handle_accept_plasma(const boost::system::error_code& error);
};
