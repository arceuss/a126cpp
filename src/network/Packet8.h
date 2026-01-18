#pragma once

#include "network/Packet.h"

// Packet8 - matches Java Packet8.java exactly
class Packet8 : public Packet {
public:
	int_t healthMP;  // Note: Java uses int but reads/writes as short
	
	Packet8();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
