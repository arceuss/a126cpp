#pragma once

#include "network/Packet.h"

// Packet29DestroyEntity - matches Java Packet29DestroyEntity.java exactly
class Packet29DestroyEntity : public Packet {
public:
	int_t entityId;
	
	Packet29DestroyEntity();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
