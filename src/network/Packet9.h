#pragma once

#include "network/Packet.h"

// Packet9 - matches Java Packet9.java exactly
class Packet9 : public Packet {
public:
	byte_t field_28048_a;  // Dimension
	long_t seed;
	
	Packet9();
	Packet9(byte_t dimension, long_t seed);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
