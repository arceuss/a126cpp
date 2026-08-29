#pragma once

#include "network/Packet.h"
#include <vector>

// Map item data packet (0x83).
class Packet131MapData : public Packet {
public:
	short_t field_28055_a;  // Item id.
	short_t field_28054_b;  // Item damage (map id).
	std::vector<byte_t> field_28056_c;
	
	Packet131MapData();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
