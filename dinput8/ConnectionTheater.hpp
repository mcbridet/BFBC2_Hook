#pragma once

class ConnectionTheater : public boost::enable_shared_from_this<ConnectionTheater>, private boost::noncopyable
{
public:
	ConnectionTheater(boost::asio::io_service& io_service);

	typedef boost::shared_ptr<ConnectionTheater> pointer;

	boost::asio::ip::tcp::socket& gameSocket();
	boost::asio::ip::tcp::socket& retailSocket();

	void start();

private:
	web::websockets::client::websocket_callback_client ws;

	boost::asio::ip::tcp::socket game_socket_;
	boost::asio::ip::tcp::socket retail_socket_;

	unsigned char received_data[PACKET_MAX_LENGTH];
	unsigned int received_length;

	unsigned char send_data[PACKET_MAX_LENGTH];
	unsigned int send_length;

	bool connected_to_game = false;
	bool connected_to_retail = false;

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);	// normal read (also deals with multiple packets)
	void handle_write(const boost::system::error_code& error);							// normal send (also deals with multiple packets)
	void handle_stop();																	// cleanup after a disconnect
};

