#include "pch.hpp"
#include "Packet.hpp"

Packet::Packet(unsigned char* data, unsigned int data_length)
{
	if (data_length < HEADER_LENGTH || data_length > PACKET_MAX_LENGTH)
		return;

	// Packet category (fsys, acct, etc...)
	char category_raw[HEADER_VALUE_LENGTH + 1];
	memcpy(category_raw, data, HEADER_VALUE_LENGTH);
	category_raw[HEADER_VALUE_LENGTH] = 0;
	category = category_raw;

	// Packet type (NORMAL, SPLITTED, etc...)
	type = Utils::DecodeInt(data + TYPE_OFFSET, HEADER_VALUE_LENGTH);
	length = Utils::DecodeInt(data + LENGTH_OFFSET, HEADER_VALUE_LENGTH);

	if (length != data_length)
	{
		BOOST_LOG_TRIVIAL(error) << "Invalid packet length! (Expected: " << length << ", Got: " << data_length << ")";
		return;
	}

	auto data_raw = new char[length - HEADER_LENGTH];
	memcpy(data_raw, data + HEADER_LENGTH, length - HEADER_LENGTH);

	int expected_data_length = length - (HEADER_LENGTH) - 1; // packet length - header - null terminator = data length
	int received_data_length = std::string(data_raw).length();

	if (expected_data_length != received_data_length)
	{
		isIncomplete = expected_data_length != -1;

		if (isIncomplete)
		{
			BOOST_LOG_TRIVIAL(error) << "Invalid data length! (Expected: " << expected_data_length << ", Got: " << received_data_length << ")";
			return;
		}
	} 

	this->data = Utils::GetPacketData(data_raw);
	isValid = true;
}
