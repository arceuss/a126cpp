#pragma once

#include "network/Packet.h"

// Packet16BlockItemSwitch - matches Java Packet16BlockItemSwitch.java exactly
// Holding Change packet - client to server
class Packet16BlockItemSwitch : public Packet {
public:
	int_t id;  // Slot ID (0-8)
	
	Packet16BlockItemSwitch();
	Packet16BlockItemSwitch(int_t slotId);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
