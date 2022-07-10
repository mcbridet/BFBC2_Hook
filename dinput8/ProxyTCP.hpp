#pragma once
#include "ConnectionFESL.hpp"

class ProxyTCP : boost::asio::noncopyable
{
public:
	ProxyTCP(boost::asio::io_service& io_service, USHORT port, bool secure, std::wstring wsPath);

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ssl::context context_;

	int port_;
	bool secure_;
	std::wstring wsPath_;

	void start_accept();
	ConnectionFESL::pointer new_fesl_connection;

	void handle_accept_fesl(const boost::system::error_code& error);
};
