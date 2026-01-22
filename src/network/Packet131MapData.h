#pragma once

#include "network/Packet.h"
#include <vector>

// Packet131MapData - matches Java Packet131MapData.java exactly
// Item Data packet - server to client (0x83)
class Packet131MapData : public Packet {
public:
	short_t field_28055_a;  // Item Type
	short_t field_28054_b;  // Item ID
	std::vector<byte_t> field_28056_c;  // Text/Data byte array
	
	Packet131MapData();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
