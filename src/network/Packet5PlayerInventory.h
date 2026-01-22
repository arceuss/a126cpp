#pragma once

#include "network/Packet.h"

// Packet5PlayerInventory - matches Java Packet5PlayerInventory.java exactly
class Packet5PlayerInventory : public Packet {
public:
	int_t entityID;
	int_t slot;
	int_t itemID;
	int_t itemDamage;
	
	Packet5PlayerInventory();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
