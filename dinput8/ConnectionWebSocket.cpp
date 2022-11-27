#include "pch.hpp"
#include "ConnectionWebSocket.hpp"

#include "Config.hpp"
#include "Hook.hpp"
#include "ProxyClient.hpp"

using namespace web;
using namespace websockets::client;

ConnectionWebSocket::ConnectionWebSocket(ProxyType type, std::function<void(unsigned char*, unsigned)> sendToGame,
                                         std::function<void()> closeCallback)
{
	BOOST_LOG_FUNCTION()
	this->type = type;

	this->sendToGame = sendToGame;
	this->closeCallback = closeCallback;

	connected = false;

	ws.set_message_handler([this](websocket_incoming_message msg) { handle_receive(msg); });
	ws.set_close_handler(
		[this](websocket_close_status close_status, const utility::string_t& reason, const std::error_code& error)
		{
			handle_disconnect(close_status, reason, error);
		});

	Config* config = &Config::getInstance();

	wsPath = config->hook->serverSecure ? L"wss://" : L"ws://";
	wsPath += std::wstring(config->hook->serverAddress.begin(), config->hook->serverAddress.end());
	wsPath += L":";
	wsPath += std::to_wstring(config->hook->serverPort);
	wsPath += type == PLASMA ? L"/plasma" : L"/theater";
}

ConnectionWebSocket::~ConnectionWebSocket()
{
	close();
}

void ConnectionWebSocket::connect()
{
	BOOST_LOG_NAMED_SCOPE("WebSocket (Connect)")

	try
	{
		BOOST_LOG_TRIVIAL(warning) << "Connecting to WebSocket server (" << wsPath << ")...";
		ws.connect(wsPath).wait();
		BOOST_LOG_TRIVIAL(warning) << "Connected to WebSocket server (" << wsPath << ")!";

		ProxyClient* pClient = &ProxyClient::getInstance();

		pClient->theater_ws = ws;
		connected = true;
	}
	catch (const websocket_exception& ex)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to connect to server! (" << ex.what() << ")";
		close();
	}
}

void ConnectionWebSocket::send(unsigned char* data, unsigned int length)
{
	BOOST_LOG_NAMED_SCOPE("WebSocket (Write)")

	try
	{
		Packet packet(data, length);

		if (!packet.isValid)
		{
			BOOST_LOG_TRIVIAL(error) << "Tried to send invalid package! Skipping...";
			return;
		}

		auto packet_data = new char[length];
		memcpy(packet_data, data, length);

		auto msgcontent = std::string(packet_data, length);
		std::vector<uint8_t> msgbuf(msgcontent.begin(), msgcontent.end());

		auto is = concurrency::streams::container_stream<std::vector<uint8_t>>::open_istream(std::move(msgbuf));

		websocket_outgoing_message msg;
		msg.set_binary_message(is);

		ws.send(msg).wait();
		BOOST_LOG_TRIVIAL(debug) << boost::format("[PROXY] -> [%s] %s 0x%08x (%i bytes) {%s}") % std::string(wsPath.begin(), wsPath.end()) % packet.service % packet.kind % packet.length % packet.data;
	}
	catch (const websocket_exception& ex)
	{
		BOOST_LOG_TRIVIAL(error) << "send() - Failed to write to websocket! (" << ex.what() << ")";
		close();
	}
}

void ConnectionWebSocket::handle_receive(websocket_incoming_message msg)
{
	BOOST_LOG_NAMED_SCOPE("WebSocket (Read)")

	try
	{
		switch (msg.message_type())
		{
		case websocket_message_type::text_message:
			return handle_text_message(msg.extract_string().get());
		case websocket_message_type::binary_message:
			return handle_binary_message(msg.body());
		case websocket_message_type::ping:
			return handle_ping(msg.body());
		default: break;
		}
	}
	catch (const websocket_exception& ex)
	{
		BOOST_LOG_TRIVIAL(error) << "handle_receive() - Failed to read from websocket! (" << ex.what() << ")";
		close();
	}
}

void ConnectionWebSocket::handle_text_message(std::string textMessage)
{
	BOOST_LOG_TRIVIAL(info) << textMessage;
}

void ConnectionWebSocket::handle_binary_message(Concurrency::streams::istream binaryMessage)
{
	concurrency::streams::container_buffer<std::vector<uint8_t>> incoming_data;
	binaryMessage.read_to_end(incoming_data).wait();

	std::vector<uint8_t> read_data = incoming_data.collection();
	auto send_buffer = new unsigned char[PACKET_MAX_LENGTH];
	std::fill_n(send_buffer, PACKET_MAX_LENGTH, 0);
	unsigned int length = read_data.size();
	memcpy(send_buffer, read_data.data(), length);
	incoming_data.close();

	const Packet packet(send_buffer, length);

	if (!packet.isValid)
	{
		BOOST_LOG_TRIVIAL(debug) << "Received invalid packet, skipping...";
		return;
	}

	if (packet.service == "ECHO")
	{
		ProxyClient* pClient = &ProxyClient::getInstance();

		std::fill_n(pClient->udp_send_data, PACKET_MAX_LENGTH, 0);
		memcpy(pClient->udp_send_data, send_buffer, length);
		pClient->udp_send_length = length;

		pClient->theaterCtx->socket_.async_send_to(
			boost::asio::buffer(send_buffer, length),
			pClient->theaterCtx->remote_endpoint_,
			boost::bind(&ProxyUDP::handle_send, pClient->theaterCtx, boost::asio::placeholders::error,
			            boost::asio::placeholders::bytes_transferred)
		);

		BOOST_LOG_TRIVIAL(debug) << boost::format("[UDP] [PROXY] <- [%s] %s 0x%08x (%i bytes) {%s}") % std::string(wsPath.begin(), wsPath.end()) % packet.service % packet.kind % packet.length % packet.data;
	}
	else
	{
		sendToGame(send_buffer, length);
		BOOST_LOG_TRIVIAL(debug) << boost::format("[PROXY] <- [%s] %s 0x%08x (%i bytes) {%s}") % std::string(wsPath.begin(), wsPath.end()) % packet.service % packet.kind % packet.length % packet.data;
	}
}

void ConnectionWebSocket::handle_ping(Concurrency::streams::istream ping)
{
	try
	{
		concurrency::streams::container_buffer<std::vector<uint8_t>> incoming_ping;
		ping.read_to_end(incoming_ping).wait();

		std::vector<uint8_t> ping_buf = incoming_ping.collection();
		auto ping_data = new char[ping_buf.size()];
		memcpy(ping_data, ping_buf.data(), ping_buf.size());

		websocket_outgoing_message pong;
		pong.set_pong_message(ping_data);

		ws.send(pong).wait();
	}
	catch (const websocket_exception& ex)
	{
		BOOST_LOG_TRIVIAL(error) << "handle_ping() - Failed to write to websocket! (" << ex.what() << ")";
		close();
	}
}

void ConnectionWebSocket::handle_disconnect(websocket_close_status close_status, const utility::string_t& reason,
                                            const std::error_code& error)
{
	BOOST_LOG_NAMED_SCOPE("WebSocket (Disconnect)")
	BOOST_LOG_TRIVIAL(info) << "Disconnected from server! (Close Status: " << static_cast<int>(close_status) << ", Reason: " << reason.c_str() << " - (Error Code: " << error.value() << ", Error Message: " << error.message().c_str() << "))";
	connected = false;
	closeCallback();
}

void ConnectionWebSocket::close()
{
	if (connected)
	{
		ws.close();
	}
}
