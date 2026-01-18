#pragma once

#include "network/Packet.h"
#include <vector>

// Packet52MultiBlockChange - matches Java Packet52MultiBlockChange.java exactly
class Packet52MultiBlockChange : public Packet {
public:
	int_t xPosition;
	int_t zPosition;
	std::vector<short_t> coordinateArray;
	std::vector<byte_t> typeArray;
	std::vector<byte_t> metadataArray;
	int_t size;
	
	Packet52MultiBlockChange();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
