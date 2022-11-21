#include "pch.hpp"
#include "Proxy.hpp"

#include "Hook.hpp"
#include "ProxyClient.hpp"
#include "ProxyTCP.hpp"
#include "ProxyUDP.hpp"
#include "ProxyHTTP.hpp"

DWORD WINAPI ProxyInit(LPVOID /*lpParameter*/)
{
	Proxy::getInstance();
	return TRUE;
}

Proxy::Proxy()
{
	BOOST_LOG_FUNCTION();
	using namespace boost::asio;

	Config* config = &Config::getInstance();
	Hook* hook = &Hook::getInstance();

	while (true)
	{
		ProxyClient* proxyClient = &ProxyClient::getInstance();

		try
		{
			io_service io_service;

			USHORT plasmaPort;
			USHORT theaterPort;

			if (hook->exeType == CLIENT)
			{
				plasmaPort = CLIENT_PLASMA_PORT;
				theaterPort = CLIENT_THEATER_PORT;
			}
			else if (hook->exeType == SERVER)
			{
				plasmaPort = SERVER_PLASMA_PORT;
				theaterPort = SERVER_THEATER_PORT;
			}
			else
				return;

			BOOST_LOG_TRIVIAL(info) << "Initializing...";
			auto plasmaProxy = new ProxyTCP(io_service, plasmaPort, true);

			auto theaterProxyTCP = new ProxyTCP(io_service, theaterPort, false);
			auto theaterProxyUDP = new ProxyUDP(io_service, theaterPort);

			auto httpProxy = new ProxyHTTP(io_service, HTTP_PORT);

			if (config->hook->connectRetail)
			{
				proxyClient->tcpResolver = new ip::tcp::resolver(io_service);
			}

			BOOST_LOG_TRIVIAL(info) << "Finished initialization, ready for receiving incoming connections!";

			io_service.run();
		}
		catch (ProxyStopException&)
		{
			// Either retail or websocket were disconnected, clean up memory by restarting proxy
			BOOST_LOG_TRIVIAL(info) << "Restarting proxy...";
			proxyClient->theater_ws.close();
			continue;
		}
		catch (std::exception& e)
		{
			BOOST_LOG_TRIVIAL(fatal) << "An unhandled exception has occurred: " << e.what();
			BOOST_LOG_TRIVIAL(info) << "Restarting proxy...";

			continue;
		}

		break;
	}
}
