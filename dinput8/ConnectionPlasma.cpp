#include "pch.hpp"
#include "ConnectionPlasma.hpp"

#include "Config.hpp"
#include "Hook.hpp"
#include "ProxyClient.hpp"

using namespace boost::asio;
using namespace ip;
using namespace web;
using namespace web::websockets::client;

ConnectionPlasma::ConnectionPlasma(io_service& io_service, ssl::context& context) : game_socket_(io_service, context), retail_socket_(io_service, context)
{
	BOOST_LOG_FUNCTION()
}

socketSSL::lowest_layer_type& ConnectionPlasma::gameSocket()
{
	return game_socket_.lowest_layer();
}

socketSSL::lowest_layer_type& ConnectionPlasma::retailSocket()
{
	return retail_socket_.lowest_layer();
}

void ConnectionPlasma::start()
{
	// Before reading stuff we have to do a handshake
	game_socket_.async_handshake(ssl::stream_base::server, bind(&ConnectionPlasma::handle_handshake, shared_from_this(), placeholders::error));
}

void ConnectionPlasma::handle_handshake(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->handle_handshake")

	Config* config = &Config::getInstance();
	Hook* hook = &Hook::getInstance();

	if (!error)
	{
		BOOST_LOG_TRIVIAL(debug) << "New connection, initializing...";

		if (config->hook->connectRetail)
		{
			ProxyClient* proxyClient = &ProxyClient::getInstance();

			std::string host = hook->exeType == CLIENT ? "bfbc2-pc.fesl.ea.com" : "bfbc2-pc-server.fesl.ea.com";
			std::string port = std::to_string(hook->exeType == CLIENT ? CLIENT_PLASMA_PORT : SERVER_PLASMA_PORT);

			BOOST_LOG_TRIVIAL(warning) << "Connecting to retail server (" << host << ":" << port << ")...";

			tcp::resolver::query query(host, port);
			tcp::resolver::iterator iterator = proxyClient->plasmaResolver->resolve(query);

			async_connect(retailSocket(), iterator, bind(&ConnectionPlasma::retail_handle_connect, shared_from_this(), placeholders::error));
		}
		else
		{
			std::wstring wsFinalPath = config->hook->serverSecure ? L"wss://" : L"ws://";
			wsFinalPath += std::wstring(config->hook->serverAddress.begin(), config->hook->serverAddress.end());
			wsFinalPath += L":";
			wsFinalPath += std::to_wstring(config->hook->serverPort);
			wsFinalPath += L"/plasma/";
			wsFinalPath += (hook->exeType == CLIENT) ? L"client" : L"server";

			BOOST_LOG_TRIVIAL(info) << "Connecting to " << wsFinalPath << "...";

			ws.set_message_handler([this](websocket_incoming_message msg) {
				try {
					switch (msg.message_type())
					{
						case websocket_message_type::text_message:
						{
							std::string server_message = msg.extract_string().get();
							BOOST_LOG_TRIVIAL(info) << server_message;
							break;
						}
						case websocket_message_type::binary_message:
						{
							auto incoming_data_raw = msg.body();
							concurrency::streams::container_buffer<std::vector<uint8_t>> incoming_data;
							incoming_data_raw.read_to_end(incoming_data).wait();

							std::vector<uint8_t> read_data = incoming_data.collection();
							std::fill_n(send_data, PACKET_MAX_LENGTH, 0);
							memcpy(send_data, read_data.data(), read_data.size());

							send_length = read_data.size();
							incoming_data.close();

							if (connected_to_game)
							{
								async_write(game_socket_, buffer(send_data, send_length),
									boost::bind(&ConnectionPlasma::handle_write, shared_from_this(), placeholders::error));
							}

							break;
						}
						case websocket_message_type::ping:
						{
							auto ping_body = msg.body();
							concurrency::streams::container_buffer<std::vector<uint8_t>> incoming_ping;
							ping_body.read_to_end(incoming_ping).wait();

							std::vector<uint8_t> ping_buf = incoming_ping.collection();
							auto ping_data = new char[ping_buf.size()];
							memcpy(ping_data, ping_buf.data(), ping_buf.size());

							websocket_outgoing_message pong;
							pong.set_pong_message(ping_data);

							ws.send(pong);
							break;
						}
					}
				}
				catch (const websocket_exception& ex)
				{
					BOOST_LOG_TRIVIAL(error) << "[WebSocket Message Handler]: Failed to read from websocket! (" << ex.what() << ")";
				}
			});

			ws.set_close_handler([this](websocket_close_status close_status, const utility::string_t& reason, const std::error_code& error) {
				BOOST_LOG_TRIVIAL(info) << "Disconnected from server! (Close Status: " << (int)close_status << ", Reason: " << reason.c_str() << " - (Error Code: " << error.value() << ", Error Message: " << error.message().c_str() << "))";
				handle_stop(false);
			});

			try
			{
				ws.connect(wsFinalPath).wait();
				connected_to_ws = true;
			}
			catch (const websocket_exception& ex)
			{
				BOOST_LOG_TRIVIAL(error) << "Failed to connect to server! (" << ex.what() << ")";
				return;
			}
		}

		connected_to_game = true;
		game_socket_.async_read_some(buffer(received_data, PACKET_MAX_LENGTH), boost::bind(&ConnectionPlasma::handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
		BOOST_LOG_TRIVIAL(info) << "Client connected";
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Disconnected, error message: " << error.message() << ", error code: " << error.value();
		handle_stop();
	}
}

void ConnectionPlasma::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->handle_read")

	Config* config = &Config::getInstance();

	if (!error)
	{
		received_length = bytes_transferred;

		// Only for logging
		// Packet category (fsys, acct, etc...)
		char packet_category[HEADER_LENGTH + 1];
		memcpy(packet_category, received_data, HEADER_LENGTH);
		packet_category[HEADER_LENGTH] = '\0';

		// Packet type (NORMAL, SPLITTED, etc...)
		const unsigned int packet_type = Utils::DecodeInt(received_data + 4, 4);
		const unsigned int packet_length = Utils::DecodeInt(received_data + 8, 4);

		const auto packet_data_raw = new char[received_length - 12];
		memcpy(packet_data_raw, received_data + 12, received_length - 12);
		std::string packet_data = Utils::GetPacketData(packet_data_raw);

		BOOST_LOG_TRIVIAL(debug) << boost::format("-> %s %08x%08x {%s}") % packet_category % packet_type % packet_length % packet_data;

		if (config->hook->connectRetail)
		{
			if (connected_to_retail)
			{
				async_write(retail_socket_, buffer(received_data, received_length),
					boost::bind(&ConnectionPlasma::retail_handle_write, shared_from_this(), placeholders::error));
			}
		}
		else
		{
			try
			{
				auto packet_data = new char[received_length];
				memcpy(packet_data, received_data, received_length);

				std::string msgcontent = std::string(packet_data, received_length);
				std::vector<uint8_t> msgbuf(msgcontent.begin(), msgcontent.end());

				auto is = concurrency::streams::container_stream<std::vector<uint8_t>>::open_istream(std::move(msgbuf));

				websocket_outgoing_message msg;
				msg.set_binary_message(is);

				ws.send(msg);
			}
			catch (const websocket_exception& ex)
			{
				BOOST_LOG_TRIVIAL(error) << "handle_read() - Failed to write to websocket! (" << ex.what() << ")";
				return handle_stop();
			}
		}

		game_socket_.async_read_some(buffer(received_data, PACKET_MAX_LENGTH), boost::bind(&ConnectionPlasma::handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		handle_stop();
	}
}

void ConnectionPlasma::handle_write(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->handle_write")

	if (!error)
	{
		// Packet was sent

		// Only for logging
		// Packet category (fsys, acct, etc...)
		char packet_category[HEADER_LENGTH + 1];
		memcpy(packet_category, send_data, HEADER_LENGTH);
		packet_category[HEADER_LENGTH] = '\0';

		// Packet type (NORMAL, SPLITTED, etc...)
		const unsigned int packet_type = Utils::DecodeInt(send_data + 4, 4);
		const unsigned int packet_length = Utils::DecodeInt(send_data + 8, 4);

		const auto packet_data_raw = new char[send_length - 12];
		memcpy(packet_data_raw, send_data + 12, send_length - 12);
		std::string packet_data = Utils::GetPacketData(packet_data_raw);

		BOOST_LOG_TRIVIAL(debug) << boost::format("<- %s %08x%08x {%s}") % packet_category % packet_type % packet_length % packet_data;
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Disconnected, error message: " << error.message() << ", error code: " << error.value();
		handle_stop();
	}
}

void ConnectionPlasma::retail_handle_connect(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->retail_handle_connect")

	if (!error)
	{
		BOOST_LOG_TRIVIAL(info) << "Connected to retail server!";

		retail_socket_.async_handshake(ssl::stream_base::client, boost::bind(&ConnectionPlasma::retail_handle_handshake, shared_from_this(), placeholders::error));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Connection to retail server failed! (" << error.message() << ")";
		handle_stop();
	}
}

void ConnectionPlasma::retail_handle_handshake(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->retail_handle_handshake")

	if (!error)
	{
		BOOST_LOG_TRIVIAL(debug) << "Handshake with retail server was successful!";

		connected_to_retail = true;
		async_write(retail_socket_, buffer(received_data, received_length),
			boost::bind(&ConnectionPlasma::retail_handle_write, shared_from_this(), placeholders::error));

		retail_socket_.async_read_some(buffer(send_data, PACKET_MAX_LENGTH), bind(&ConnectionPlasma::retail_handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Handshake with retail server failed! (" << error.message() << ")";
		handle_stop();
	}
}

void ConnectionPlasma::retail_handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->retail_handle_read")

	if (!error)
	{
		send_length = bytes_transferred;

		if (send_data[(send_length + receivedDataOffset) - 1] != NULL)
		{
			receivedDataOffset += send_length;
			retail_socket_.async_read_some(buffer(send_data + receivedDataOffset, PACKET_MAX_LENGTH - receivedDataOffset), bind(&ConnectionPlasma::retail_handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
			return;
		}

		async_write(game_socket_,
			buffer(send_data, send_length + receivedDataOffset),
			boost::bind(&ConnectionPlasma::handle_write, shared_from_this(), placeholders::error));

		receivedDataOffset = NULL;
		retail_socket_.async_read_some(buffer(send_data, PACKET_MAX_LENGTH), bind(&ConnectionPlasma::retail_handle_read, shared_from_this(), placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		handle_stop();
	}
}

void ConnectionPlasma::retail_handle_write(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->retail_handle_write")

	if (error)
	{
		BOOST_LOG_TRIVIAL(error) << "Error message: " << error.message() << ", error code: " << error.value();
		handle_stop();
	}
}


void ConnectionPlasma::handle_stop(bool crash)
{
	BOOST_LOG_NAMED_SCOPE("Plasma->handle_stop")

	if (connected_to_retail)
	{
		connected_to_retail = false;
		retail_socket_.lowest_layer().close();
	}

	if (connected_to_ws)
	{
		connected_to_ws = false;
		ws.close();
	}

	if (connected_to_game)
	{
		connected_to_game = false;
		game_socket_.lowest_layer().close();
	}

	BOOST_LOG_TRIVIAL(info) << "Client Disconnected!";

	if (crash)
	{
		// Throw an exception to restart proxy
		throw ProxyStopException();
	}
}
