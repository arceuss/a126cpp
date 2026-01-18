#pragma once

#include "network/Packet.h"
#include "world/item/ItemStack.h"
#include <memory>

// Packet103SetSlot - matches Java Packet103SetSlot.java exactly
// Set slot packet - server to client (0x67)
class Packet103SetSlot : public Packet {
public:
	int_t windowId;  // Note: Java uses int but reads/writes as byte
	int_t itemSlot;  // Note: Java uses int but reads/writes as short
	std::shared_ptr<ItemStack> myItemStack;
	
	Packet103SetSlot();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
