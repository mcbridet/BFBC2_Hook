#pragma once

class HttpHandler : public boost::enable_shared_from_this<HttpHandler>, private boost::noncopyable
{
public:
	HttpHandler(boost::asio::io_service& io_service);

	using pointer = boost::shared_ptr<HttpHandler>;

	boost::asio::ip::tcp::socket& gameSocket();
	boost::asio::ip::tcp::socket& retailSocket();

	void start();

private:
	boost::asio::ip::tcp::socket game_socket_;
	boost::asio::ip::tcp::socket retail_socket_;

	// The buffer for performing reads.
	boost::beast::flat_buffer buffer_{8192};

	// The request message.
	boost::beast::http::request<boost::beast::http::dynamic_body> request_;

	// The response message.
	boost::beast::http::response<boost::beast::http::dynamic_body> response_;

	std::string logTarget;

	void process_request(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
};
