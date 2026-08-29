#pragma once

#include "network/Packet.h"

// Statistic update packet (0xC8).
class Packet200Statistic : public Packet {
public:
	int_t field_27052_a;  // Statistic id.
	int_t field_27051_b;  // Signed byte value stored in a Java int.
	
	Packet200Statistic();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
