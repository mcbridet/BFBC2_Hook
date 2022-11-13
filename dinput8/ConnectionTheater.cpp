#include "pch.hpp"
#include "ConnectionTheater.hpp"
#include "Config.hpp"
#include "Hook.hpp"
#include "ProxyClient.hpp"
#include "ProxyUDP.hpp"

using namespace boost;
using namespace asio;
using asio::ip::tcp;
using namespace web;
using namespace web::websockets::client;

ConnectionTheater::ConnectionTheater(io_service& io_service) : game_socket_(io_service), retail_socket_(io_service)
{
	BOOST_LOG_FUNCTION()
}

tcp::socket& ConnectionTheater::gameSocket()
{
	return game_socket_;
}

tcp::socket& ConnectionTheater::retailSocket()
{
	return retail_socket_;
}

void ConnectionTheater::start()
{
	game_socket_.async_read_some(buffer(received_data, PACKET_MAX_LENGTH), bind(&ConnectionTheater::handle_read, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void ConnectionTheater::handle_read(const system::error_code& error, size_t bytes_transferred)
{
	BOOST_LOG_NAMED_SCOPE("Theater->handle_read")

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

		const auto packet_data_raw = new char[packet_length - 12];
		memcpy(packet_data_raw, received_data + 12, packet_length - 12);
		std::string packet_data = Utils::GetPacketData(packet_data_raw);

		BOOST_LOG_TRIVIAL(debug) << boost::format("-> %s %08x%08x {%s}") % packet_category % packet_type % packet_length % packet_data;
		ProxyClient* pClient = &ProxyClient::getInstance();

		if (!pClient->connected_to_theater)
		{
			Config* config = &Config::getInstance();
			Hook* hook = &Hook::getInstance();

			std::wstring wsFinalPath = config->hook->serverSecure ? L"wss://" : L"ws://";
			wsFinalPath += std::wstring(config->hook->serverAddress.begin(), config->hook->serverAddress.end());
			wsFinalPath += L":";
			wsFinalPath += std::to_wstring(config->hook->serverPort);
			wsFinalPath += L"/theater/";
			wsFinalPath += (hook->exeType == CLIENT) ? L"client" : L"server";

			BOOST_LOG_TRIVIAL(info) << "Connecting to " << wsFinalPath << "...";

			ws.set_message_handler([this, pClient](websocket_incoming_message msg) {
				try {
					switch (msg.message_type())
					{
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

							char packet_category[HEADER_LENGTH + 1];
							memcpy(packet_category, send_data, HEADER_LENGTH);
							packet_category[HEADER_LENGTH] = '\0';

							if (packet_category == "ECHO")
							{
								pClient->theaterCtx->socket_.async_send_to(
									buffer(send_data, send_length),
									pClient->theaterCtx->remote_endpoint_,
									boost::bind(&ProxyUDP::handle_send, pClient->theaterCtx, asio::placeholders::error, asio::placeholders::bytes_transferred)
								);
							}
							else
							{
								async_write(game_socket_, buffer(send_data, send_length), bind(&ConnectionTheater::handle_write, shared_from_this(), asio::placeholders::error));
							}

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
				handle_stop();
			});

			try
			{
				ws.connect(wsFinalPath).wait();

				pClient->connected_to_theater = true;
				pClient->theater_ws = ws;
			}
			catch (const websocket_exception& ex)
			{
				BOOST_LOG_TRIVIAL(error) << "Failed to connect to server! (" << ex.what() << ")";
				return;
			}
		}

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

		game_socket_.async_read_some(buffer(received_data, PACKET_MAX_LENGTH), bind(&ConnectionTheater::handle_read, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
	}
}

void ConnectionTheater::handle_write(const boost::system::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("Theater->handle_write")

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

		const auto packet_data_raw = new char[packet_length - 12];
		memcpy(packet_data_raw, send_data + 12, packet_length - 12);
		std::string packet_data = Utils::GetPacketData(packet_data_raw);

		BOOST_LOG_TRIVIAL(debug) << boost::format("<- %s %08x%08x {%s}") % packet_category % packet_type % packet_length % packet_data;
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Disconnected, error message: " << error.message() << ", error code: " << error.value();
		handle_stop();
	}
}

void ConnectionTheater::handle_stop(bool crash)
{
	BOOST_LOG_NAMED_SCOPE("Theater->handle_stop")

	if (connected_to_retail)
	{
		connected_to_retail = false;
		retail_socket_.lowest_layer().close();
	}

	ProxyClient* pClient = &ProxyClient::getInstance();

	if (pClient->connected_to_theater)
	{
		pClient->connected_to_theater = false;
		ws.close();
	}

	if (connected_to_game)
	{
		connected_to_game = false;
		game_socket_.lowest_layer().close();
	}

	BOOST_LOG_TRIVIAL(info) << "Client Disconnected!";
}
