#pragma once
#include "HttpHandler.hpp"

class ProxyHTTP : boost::asio::noncopyable
{
public:
	ProxyHTTP(boost::asio::io_service& io_service, USHORT port);

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	int port_;

	HttpHandler::pointer new_http_connection;

	void start_accept();
	void handle_accept(const boost::system::error_code& error);
};
