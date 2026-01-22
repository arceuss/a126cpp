#pragma once

#include "network/Packet.h"

// Packet70Bed - matches Java Packet70Bed.java exactly
// New/Invalid State packet - server to client (0x46)
class Packet70Bed : public Packet {
public:
	int_t field_25019_b;  // Reason code (byte in Java, but stored as int)
	
	Packet70Bed();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
