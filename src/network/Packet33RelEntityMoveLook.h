#pragma once

#include "network/Packet.h"

// Packet33RelEntityMoveLook - matches Java Packet33RelEntityMoveLook.java exactly
// Entity Look and Relative Move packet - server to client
// Note: In Java, this extends Packet30Entity, so it calls super.readPacketData (reads entityId)
class Packet33RelEntityMoveLook : public Packet {
public:
	int_t entityId;  // From Packet30Entity super
	byte_t xPosition;  // Relative move in bytes (offset from Absolute Int)
	byte_t yPosition;
	byte_t zPosition;
	byte_t yaw;
	byte_t pitch;
	
	Packet33RelEntityMoveLook();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
