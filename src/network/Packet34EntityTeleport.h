#pragma once

#include "network/Packet.h"

// Packet34EntityTeleport - matches Java Packet34EntityTeleport.java exactly
class Packet34EntityTeleport : public Packet {
public:
	int_t entityId;
	int_t xPosition;  // Absolute Integer (fixed-point)
	int_t yPosition;  // Absolute Integer (fixed-point)
	int_t zPosition;  // Absolute Integer (fixed-point)
	byte_t yaw;       // Packed rotation
	byte_t pitch;     // Packed pitch
	
	Packet34EntityTeleport();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
