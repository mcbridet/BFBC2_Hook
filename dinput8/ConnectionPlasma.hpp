#pragma once

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socketSSL;

class ConnectionPlasma : public boost::enable_shared_from_this<ConnectionPlasma>, boost::asio::noncopyable
{
public:
	ConnectionPlasma(boost::asio::io_service& io_service, boost::asio::ssl::context& context);
	typedef boost::shared_ptr<ConnectionPlasma> pointer;

	socketSSL::lowest_layer_type& gameSocket();
	socketSSL::lowest_layer_type& retailSocket();

	void start();

private:
	web::websockets::client::websocket_callback_client ws;

	socketSSL game_socket_;
	socketSSL retail_socket_;

	void handle_handshake(const boost::system::error_code& error);						// first sequence of packets for ssl connection
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred); // normal read
	void handle_write(const boost::system::error_code& error);							// normal send (also deals with multiple packets)
	void handle_stop();																	// cleanup after a disconnect

	void retail_handle_connect(const boost::system::error_code& error);
	void retail_handle_handshake(const boost::system::error_code& error);
	void retail_handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void retail_handle_write(const boost::system::error_code& error);

	bool connected_to_ws = false;
	bool connected_to_game = false;
	bool connected_to_retail = false;

	unsigned char received_data[PACKET_MAX_LENGTH];
	unsigned int received_length;

	int receivedDataOffset = 0;

	unsigned char send_data[PACKET_MAX_LENGTH];
	unsigned int send_length;
};
