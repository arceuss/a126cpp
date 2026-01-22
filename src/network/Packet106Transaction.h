#pragma once

#include "network/Packet.h"

// Packet106Transaction - matches Java Packet106Transaction.java exactly
// Transaction packet - bidirectional (0x6A)
class Packet106Transaction : public Packet {
public:
	int_t windowId;  // Note: Java uses int but reads/writes as byte
	short_t field_20028_b;  // Action number
	bool field_20030_c;  // Accepted
	
	Packet106Transaction();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
