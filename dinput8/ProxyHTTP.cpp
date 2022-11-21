#include "pch.hpp"
#include "ProxyHTTP.hpp"

using namespace boost;
using namespace asio;
using ip::tcp;

ProxyHTTP::ProxyHTTP(io_service& io_service, USHORT port) : acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
{
	BOOST_LOG_FUNCTION();

	port_ = port;

	start_accept();
	BOOST_LOG_TRIVIAL(info) << "Created HTTP Proxy (listening on port " << port << ")...";
}

void ProxyHTTP::start_accept()
{
	new_http_connection.reset(new HttpHandler(static_cast<io_context&>(acceptor_.get_executor().context())));
	acceptor_.async_accept(new_http_connection->gameSocket(),
	                       boost::bind(&ProxyHTTP::handle_accept, this, asio::placeholders::error));
}

void ProxyHTTP::handle_accept(const system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("HTTP")

	if (!acceptor_.is_open())
	{
		BOOST_LOG_TRIVIAL(error) << "TCP Socket, port " << port_ << " - acceptor is not open";
		return;
	}

	if (!error)
		new_http_connection->start();
	else
		BOOST_LOG_TRIVIAL(error) << "TCP Socket, port " << port_ << " - error: " << error.message() << ", error code: " <<
 error.value();

	start_accept();
}
