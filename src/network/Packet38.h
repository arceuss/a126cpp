#pragma once

#include "network/Packet.h"

// Packet38 - matches Java Packet38.java exactly
// Entity Status packet - server to client (0x26)
class Packet38 : public Packet {
public:
	int_t field_9274_a;  // Entity ID
	byte_t field_9273_b;  // Entity status
	
	Packet38();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
