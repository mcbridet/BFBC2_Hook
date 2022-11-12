#pragma once

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

    boost::asio::ip::tcp::resolver* plasmaResolver;
};
