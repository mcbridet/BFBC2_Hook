#pragma once
#include <iostream>

class ProxyStopException : public std::exception {
public:
	const char* what() {
		return "Restart Proxy";
	}
};
