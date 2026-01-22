#pragma once

#include "network/Packet.h"

// Packet7 - matches Java Packet7.java exactly
// Use Entity packet - client to server
class Packet7 : public Packet {
public:
	int_t field_9277_a;  // User entity ID
	int_t field_9276_b;  // Target entity ID
	byte_t field_9278_c;  // Left-click flag (bool as byte)
	
	Packet7();
	Packet7(int_t user, int_t target, byte_t leftClick);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
