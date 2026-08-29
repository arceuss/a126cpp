#pragma once

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

#include "network/Packet.h"
#include "network/SocketStreams.h"
#include "tools/headless/TestFramework.h"

namespace headless
{

inline std::vector<byte_t> encodePacketData(Packet &packet)
{
	std::vector<byte_t> bytes;
	SocketOutputStream output(bytes);
	packet.writePacketData(output);
	output.flush();
	return bytes;
}

inline std::vector<byte_t> encodeFramedPacket(Packet &packet)
{
	std::vector<byte_t> bytes;
	SocketOutputStream output(bytes);
	Packet::writePacket(&packet, output);
	output.flush();
	return bytes;
}

template<typename PacketType>
std::unique_ptr<PacketType> decodePacketData(const std::vector<byte_t> &bytes)
{
	std::unique_ptr<PacketType> packet = std::make_unique<PacketType>();
	SocketInputStream input(bytes);
	packet->readPacketData(input);
	if (input.read() != -1)
		throw std::runtime_error("packet decoder left unread payload bytes");
	return packet;
}

inline std::string packetBytes(const std::vector<byte_t> &bytes)
{
	std::ostringstream text;
	text << std::hex << std::setfill('0');
	for (size_t i = 0; i < bytes.size(); ++i)
	{
		if (i != 0)
			text << ' ';
		text << std::setw(2) << static_cast<int_t>(static_cast<ubyte_t>(bytes[i]));
	}
	return text.str();
}

inline bool checkPacketBytes(TestContext &ctx, const std::vector<byte_t> &actual,
	const std::vector<byte_t> &expected, const std::string &message)
{
	if (actual == expected)
		return true;
	ctx.fail(message + ": expected [" + packetBytes(expected) + "], got [" + packetBytes(actual) + "]");
	return false;
}

}
