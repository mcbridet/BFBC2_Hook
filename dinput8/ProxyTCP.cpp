#include "pch.hpp"
#include "ProxyTCP.hpp"

using namespace boost;
using namespace asio;
using ip::tcp;

ProxyTCP::ProxyTCP(io_service& io_service, USHORT port, bool secure) :
	acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), context_(ssl::context::sslv23)
{
	BOOST_LOG_FUNCTION();

	port_ = port;
	secure_ = secure;

	if (secure)
	{
		SSL_CTX_set_cipher_list(context_.native_handle(), "ALL:!DH");
		SSL_CTX_set_options(context_.native_handle(), SSL_OP_ALL);

		SSL_CTX_use_certificate_ASN1(context_.native_handle(), sizeof(CERTIFICATE_X509), CERTIFICATE_X509);
		SSL_CTX_use_PrivateKey_ASN1(EVP_PKEY_RSA, context_.native_handle(), PRIVATE_KEY_RSA, sizeof(PRIVATE_KEY_RSA));
		SSL_CTX_set_verify_depth(context_.native_handle(), 1);
	}

	start_accept();
	BOOST_LOG_TRIVIAL(info) << "Created TCP Proxy (listening on port " << port << ")...";
}

void ProxyTCP::start_accept()
{
	if (secure_)
	{
		new_plasma_connection.reset(
			new ConnectionPlasma(static_cast<io_context&>(acceptor_.get_executor().context()), context_));
		acceptor_.async_accept(new_plasma_connection->gameSocket(),
		                       bind(&ProxyTCP::handle_accept_plasma, this, asio::placeholders::error));
	}
	else
	{
		new_theater_connection.reset(
			new ConnectionTheater(static_cast<io_context&>(acceptor_.get_executor().context()), context_));
		acceptor_.async_accept(new_theater_connection->gameSocket(),
		                       boost::bind(&ProxyTCP::handle_accept_theater, this, asio::placeholders::error));
	}
}

void ProxyTCP::handle_accept_plasma(const system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Plasma")

	if (!acceptor_.is_open())
	{
		BOOST_LOG_TRIVIAL(error) << "TCP Socket, port " << port_ << " - acceptor is not open";
		return;
	}

	if (!error)
		new_plasma_connection->start();
	else
		BOOST_LOG_TRIVIAL(error) << "TCP Socket, port " << port_ << " - error: " << error.message() << ", error code: " <<
 error.value();

	start_accept();
}

void ProxyTCP::handle_accept_theater(const system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Theater")

	if (!acceptor_.is_open())
	{
		BOOST_LOG_TRIVIAL(error) << "TCP Socket, port " << port_ << " - acceptor is not open";
		return;
	}

	if (!error)
		new_theater_connection->start();
	else
		BOOST_LOG_TRIVIAL(error) << "TCP Socket, port " << port_ << " - error: " << error.message() << ", error code: " <<
 error.value();

	start_accept();
}
