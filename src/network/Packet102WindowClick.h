#pragma once

#include "network/Packet.h"
#include "world/item/ItemStack.h"
#include <memory>

// Packet102WindowClick - matches Java Packet102WindowClick.java exactly
// Window click packet - client to server (0x66)
class Packet102WindowClick : public Packet {
public:
	int_t window_Id;  // Note: Java uses int but reads/writes as byte
	int_t inventorySlot;  // Note: Java uses int but reads/writes as short
	int_t mouseClick;  // Note: Java uses int but reads/writes as byte
	short_t action;
	std::shared_ptr<ItemStack> itemStack;
	bool field_27050_f;  // Shift
	
	Packet102WindowClick();
	Packet102WindowClick(int_t windowId, int_t inventorySlot, int_t mouseClick, bool shift, std::unique_ptr<ItemStack> itemStack, short_t action);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
