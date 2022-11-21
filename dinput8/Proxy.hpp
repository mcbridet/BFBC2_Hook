#pragma once

DWORD WINAPI ProxyInit(LPVOID lpParameter);

class Proxy
{
public:
	Proxy();

	Proxy(const Proxy&)
	{
	}

	static Proxy& getInstance()
	{
		static Proxy* _instance = nullptr;

		if (_instance == nullptr)
			_instance = new Proxy();

		return *_instance;
	}
};
