#include "pch.hpp"
#include "Packet.hpp"

Packet::Packet(unsigned char* data, unsigned int data_length)
{
	if (data_length < HEADER_LENGTH || data_length > PACKET_MAX_LENGTH)
	{
		BOOST_LOG_TRIVIAL(error) << "Invalid packet length! (Expected data length in range " << HEADER_LENGTH << "-" << PACKET_MAX_LENGTH << " (bytes), Got: " << data_length << ")";
		return;
	}

	// Service that should handle this packet (fsys, acct, etc...)
	char service_raw[HEADER_VALUE_LENGTH + 1];
	memcpy(service_raw, data, HEADER_VALUE_LENGTH);
	service_raw[HEADER_VALUE_LENGTH] = 0;
	service = service_raw;

	// Transaction kind (Simple, Chunked, etc...)
	kind = Utils::DecodeInt(data + TYPE_OFFSET, HEADER_VALUE_LENGTH);
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
			realLength = received_data_length + HEADER_LENGTH;
			BOOST_LOG_TRIVIAL(error) << "Invalid data length! (Expected: " << expected_data_length << ", Got: " << received_data_length << ")";
			return;
		}
	}

	this->data = Utils::GetPacketData(data_raw);
	isValid = true;
}
