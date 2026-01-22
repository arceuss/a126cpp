#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet20NamedEntitySpawn - matches Java Packet20NamedEntitySpawn.java exactly
class Packet20NamedEntitySpawn : public Packet {
public:
	int_t entityId;
	jstring name;
	int_t xPosition;  // Absolute Integer (fixed-point)
	int_t yPosition;  // Absolute Integer (fixed-point)
	int_t zPosition;  // Absolute Integer (fixed-point)
	byte_t rotation;  // Packed rotation
	byte_t pitch;     // Packed pitch
	short_t currentItem;
	
	Packet20NamedEntitySpawn();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
