#pragma once

#include "network/Packet.h"

class EntityItem;

// Packet21PickupSpawn - matches Java Packet21PickupSpawn.java exactly
class Packet21PickupSpawn : public Packet {
public:
	int_t entityId;
	int_t xPosition;  // Absolute Integer (fixed-point)
	int_t yPosition;  // Absolute Integer (fixed-point)
	int_t zPosition;  // Absolute Integer (fixed-point)
	byte_t rotation;  // Packed rotation (motionX * 128)
	byte_t pitch;     // Packed pitch (motionY * 128)
	byte_t roll;      // Packed roll (motionZ * 128)
	int_t itemId;
	int_t count;
	int_t itemDamage;
	
	Packet21PickupSpawn();
	Packet21PickupSpawn(EntityItem &entityItem);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
