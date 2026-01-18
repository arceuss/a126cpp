#pragma once

#include "network/Packet.h"

// Packet28 - matches Java Packet28.java exactly
// Entity Velocity packet
class Packet28 : public Packet {
public:
	int_t entityId;  // field_6367_a
	short_t motionX;  // field_6366_b (velocity * 8000.0 as int)
	short_t motionY;  // field_6369_c (velocity * 8000.0 as int)
	short_t motionZ;  // field_6368_d (velocity * 8000.0 as int)
	
	Packet28();
	Packet28(int_t entityId, double motionX, double motionY, double motionZ);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
