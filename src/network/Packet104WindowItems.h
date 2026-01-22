#pragma once

#include "network/Packet.h"
#include "world/item/ItemStack.h"
#include <vector>
#include <memory>

// Packet104WindowItems - matches Java Packet104WindowItems.java exactly
// Window items packet - server to client (0x68)
class Packet104WindowItems : public Packet {
public:
	int_t windowId;  // Note: Java uses int but reads/writes as byte
	std::vector<std::shared_ptr<ItemStack>> itemStack;  // Array of ItemStack
	
	Packet104WindowItems();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
