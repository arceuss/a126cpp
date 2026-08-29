#pragma once

#include "network/Packet.h"

// Packet14BlockDig - matches Java Packet14BlockDig.java exactly
class Packet14BlockDig : public Packet {
public:
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	int_t face;
	int_t status;
	
	Packet14BlockDig();
	Packet14BlockDig(int_t status, int_t x, int_t y, int_t z, int_t face);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
