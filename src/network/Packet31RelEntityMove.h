#pragma once

#include "network/Packet.h"

// Packet31RelEntityMove - matches Java Packet31RelEntityMove.java exactly
// Entity Relative Move packet - server to client
// Note: In Java, this extends Packet30Entity, so it calls super.readPacketData (reads entityId)
class Packet31RelEntityMove : public Packet {
public:
	int_t entityId;  // From Packet30Entity super
	byte_t xPosition;  // Relative move in bytes (offset from Absolute Int)
	byte_t yPosition;
	byte_t zPosition;
	
	Packet31RelEntityMove();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
