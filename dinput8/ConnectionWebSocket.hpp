#pragma once

class ConnectionWebSocket
{
public:
	ConnectionWebSocket(ProxyType type, std::function<void(unsigned char*, unsigned int)> sendToGame,
	                    std::function<void()> closeCallback);
	~ConnectionWebSocket();

	void connect();
	void send(unsigned char* data, unsigned int length);
	void close();

	bool connected = false;

private:
	web::websockets::client::websocket_callback_client ws;

	ProxyType type;

	std::function<void(unsigned char*, unsigned int)> sendToGame;
	std::function<void()> closeCallback;

	std::wstring wsPath;

	void handle_receive(web::websockets::client::websocket_incoming_message msg);

	void handle_text_message(std::string textMessage);
	void handle_binary_message(Concurrency::streams::istream binaryMessage);
	void handle_ping(Concurrency::streams::istream ping);

	void handle_disconnect(web::websockets::client::websocket_close_status close_status,
	                       const utility::string_t& reason, const std::error_code& error);
};
