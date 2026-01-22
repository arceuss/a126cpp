#pragma once

#include "network/Packet.h"
#include "world/phys/ChunkCoordinates.h"
#include <set>

// Packet60 - matches Java Packet60.java exactly
// Explosion packet - server to client (0x3C)
class Packet60 : public Packet {
public:
	double field_12236_a;  // X
	double field_12235_b;  // Y
	double field_12239_c;  // Z
	float field_12238_d;   // Radius/Unknown
	std::set<ChunkCoordinates> field_12237_e;  // Affected block positions
	
	Packet60();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
