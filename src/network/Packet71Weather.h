#pragma once

#include "network/Packet.h"

// Packet71Weather - matches Java Packet71Weather.java exactly
// Thunderbolt packet - server to client (0x47)
class Packet71Weather : public Packet {
public:
	int_t field_27054_a;  // Entity ID
	int_t field_27053_b;  // X coordinate
	int_t field_27057_c;  // Y coordinate
	int_t field_27056_d;  // Z coordinate
	int_t field_27055_e;  // Unknown boolean (byte in Java)
	
	Packet71Weather();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
