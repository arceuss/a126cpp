#pragma once

#include "network/Packet.h"

// Packet63Digging - matches Java Packet63Digging.java exactly
// Digging progress packet - server to client (protocol extension)
class Packet63Digging : public Packet {
public:
	int_t x;
	int_t y;
	int_t z;
	byte_t face;
	float progress;
	long_t timestamp;  // Set on read, not written
	
	Packet63Digging();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
