#pragma once
#include "ProxyUDP.hpp"

class ProxyClient
{
public:
    ProxyClient() {}
    ProxyClient(const ProxyClient&) {}

    static ProxyClient& getInstance() {
        static ProxyClient* _instance = nullptr;

        if (_instance == nullptr)
            _instance = new ProxyClient();

        return *_instance;
    }

    boost::asio::ip::tcp::resolver* tcpResolver;

    ProxyUDP* theaterCtx;
    web::websockets::client::websocket_callback_client theater_ws;

    bool connected_to_theater = false;
    unsigned char udp_send_data[PACKET_MAX_LENGTH];
};
