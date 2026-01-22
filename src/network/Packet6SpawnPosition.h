#pragma once

#include "network/Packet.h"

// Packet6SpawnPosition - matches Java Packet6SpawnPosition.java exactly
class Packet6SpawnPosition : public Packet {
public:
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	
	Packet6SpawnPosition();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
