#pragma once

#include "network/Packet.h"

// Packet30Entity - matches Java Packet30Entity.java exactly
// Minimal packet - only contains entityId in read/write
class Packet30Entity : public Packet {
public:
	int_t entityId;
	byte_t xPosition;
	byte_t yPosition;
	byte_t zPosition;
	byte_t yaw;
	byte_t pitch;
	bool rotating;
	
	Packet30Entity();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
