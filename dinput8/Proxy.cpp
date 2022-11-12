#include "pch.hpp"
#include "Proxy.hpp"

#include "Hook.hpp"
#include "ProxyClient.hpp"
#include "ProxyTCP.hpp"

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
			ProxyTCP* plasmaProxy = new ProxyTCP(io_service, plasmaPort, true);

			if (config->hook->connectRetail)
			{
				ProxyClient* proxyClient = &ProxyClient::getInstance();
				proxyClient->plasmaResolver = new ip::tcp::resolver(io_service);
			}

			BOOST_LOG_TRIVIAL(info) << "Finished initialization, ready for receiving incoming connections!";

			io_service.run();
		}
		catch (std::exception& e)
		{
			BOOST_LOG_TRIVIAL(error) << "An unhandled exception has occurred: " << e.what();
			BOOST_LOG_TRIVIAL(info) << "Restarting proxy...";

			continue;
		}

		break;
	}
}
