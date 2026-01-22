#pragma once

#include "network/Packet.h"

// Packet50PreChunk - matches Java Packet50PreChunk.java exactly
class Packet50PreChunk : public Packet {
public:
	int_t xPosition;
	int_t yPosition;  // Note: Java uses yPosition for chunk Z coordinate
	bool mode;
	
	Packet50PreChunk();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
