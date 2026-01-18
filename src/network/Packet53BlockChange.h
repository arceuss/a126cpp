#pragma once

#include "network/Packet.h"

// Packet53BlockChange - matches Java Packet53BlockChange.java exactly
class Packet53BlockChange : public Packet {
public:
	int_t xPosition;
	byte_t yPosition;  // Note: Java uses byte for yPosition
	int_t zPosition;
	byte_t type;
	byte_t metadata;
	
	Packet53BlockChange();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
