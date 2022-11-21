#pragma once

class Packet
{
public:
	Packet(unsigned char* data, unsigned int data_length);

	std::string category;
	unsigned int type, length;
	std::string data;

	bool isValid = false;
	bool isIncomplete = false;
};
