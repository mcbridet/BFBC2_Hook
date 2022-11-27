#include "pch.hpp"
#include "HttpHandler.hpp"
#include "Config.hpp"

using namespace boost;
using namespace asio;
using ip::tcp;
namespace http = beast::http;
using namespace web::http;
using namespace client;

HttpHandler::HttpHandler(io_service& io_service) : game_socket_(io_service), retail_socket_(io_service)
{
	BOOST_LOG_FUNCTION()
}

tcp::socket& HttpHandler::gameSocket()
{
	return game_socket_;
}

tcp::socket& HttpHandler::retailSocket()
{
	return retail_socket_;
}

void HttpHandler::start()
{
	http::async_read(game_socket_, buffer_, request_, bind(&HttpHandler::process_request, shared_from_this(),
	                                                       asio::placeholders::error,
	                                                       asio::placeholders::bytes_transferred));
}

void HttpHandler::process_request(const system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_NAMED_SCOPE("HTTP->process_request")

	if (!error)
	{
		Config* config = &Config::getInstance();

		auto request_header = request_.base();

		auto host = request_header["Host"];

		auto boostMethod = request_header.method();
		method sdkMethod;

		switch (boostMethod)
		{
		case http::verb::get:
			sdkMethod = methods::GET;
			break;
		case http::verb::post:
			sdkMethod = methods::POST;
			break;
		default:
			BOOST_LOG_TRIVIAL(warning) << "Not implemented method! (" << request_header.method_string() << ") Defaulting to GET...";
			sdkMethod = methods::GET;
			break;
		}

		auto target = request_header.target();
		logTarget = target.to_string();

		std::wstring httpFinalPath;

		if (config->hook->connectRetail)
		{
			httpFinalPath = L"http://" + std::wstring(host.begin(), host.end());
			httpFinalPath += std::wstring(target.begin(), target.end());
		}
		else
		{
			httpFinalPath = config->hook->serverSecure ? L"https://" : L"http://";
			httpFinalPath += std::wstring(config->hook->serverAddress.begin(), config->hook->serverAddress.end());
			httpFinalPath += L":";
			httpFinalPath += std::to_wstring(config->hook->serverPort);
			httpFinalPath += std::wstring(target.begin(), target.end());
		}

		http_client client(httpFinalPath);

		response_.version(request_header.version());
		response_.body().clear();

		try
		{
			int result = -1;
			std::string content_type;

			Concurrency::streams::stringstreambuf response_buffer;

			client.request(sdkMethod)
			      .then([&content_type, &result, &response_buffer](const http_response& response)
			      {
				      utility::string_t ct = response.headers().content_type();
				      content_type = std::string(ct.begin(), ct.end());
				      result = response.status_code();
				      return response.body().read_to_end(response_buffer).get();
			      }).wait();

			response_.result(result);
			response_.set(http::field::content_type, content_type);
			ostream(response_.body()) << response_buffer.collection();
		}
		catch (std::exception& e)
		{
			BOOST_LOG_TRIVIAL(error) << "[HTTP Request Handler]: Failure while doing HTTP request to origin server! (" << e.what() << ")";
			response_.result(http::status::internal_server_error);
			response_.set(http::field::content_type, "text/html");
			ostream(response_.body()) << "<h1>Proxy Error</h1><br/>" << e.what() <<
				"<br/><br/><hr>Battlefield: Bad Company 2 Master Server Emulator by Marek Grzyb (@GrzybDev).\r\n";
		}

		response_.content_length(response_.body().size());

		http::async_write(game_socket_, response_, bind(&HttpHandler::handle_write, shared_from_this(),
		                                                asio::placeholders::error,
		                                                asio::placeholders::bytes_transferred));
		http::async_read(game_socket_, buffer_, request_, bind(&HttpHandler::process_request, shared_from_this(),
		                                                       asio::placeholders::error,
		                                                       asio::placeholders::bytes_transferred));
	}
}

void HttpHandler::handle_write(const system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_TRIVIAL(debug) << format("%s %s [%s]") % request_.method_string() % logTarget % response_.result_int();
}
