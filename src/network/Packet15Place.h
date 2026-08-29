#pragma once

#include "network/Packet.h"

// Forward declaration
class ItemStack;

// Packet15Place - matches Java Packet15Place.java exactly
class Packet15Place : public Packet {
public:
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	int_t direction;
	std::shared_ptr<ItemStack> itemStack;
	
	Packet15Place();
	Packet15Place(int_t x, int_t y, int_t z, int_t direction, std::shared_ptr<ItemStack> itemStack);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
