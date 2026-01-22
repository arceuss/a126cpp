#pragma once

#include "network/Packet.h"

// Packet200Statistic - matches Java Packet200Statistic.java exactly
// Increment Statistic packet - server to client (0xC8)
class Packet200Statistic : public Packet {
public:
	int_t field_27052_a;  // Statistic ID
	int_t field_27051_b;  // Amount (byte in Java, int in class)
	
	Packet200Statistic();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
