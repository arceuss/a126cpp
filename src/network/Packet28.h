#pragma once

#include "network/Packet.h"

class Entity;

// Packet28 - matches Java Packet28.java exactly
// Entity Velocity packet
class Packet28 : public Packet {
public:
	int_t entityId;  // field_6367_a
	int_t motionX;  // field_6366_b
	int_t motionY;  // field_6369_c
	int_t motionZ;  // field_6368_d
	
	Packet28();
	Packet28(int_t entityId, double motionX, double motionY, double motionZ);
	
	Packet28(Entity &entity);
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
