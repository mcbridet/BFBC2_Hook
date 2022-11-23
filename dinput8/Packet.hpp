#pragma once

class Packet
{
public:
	Packet(unsigned char* data, unsigned int data_length);

	std::string service;
	unsigned int kind, length;
	std::string data;

	bool isValid = false;
	bool isIncomplete = false;
};
