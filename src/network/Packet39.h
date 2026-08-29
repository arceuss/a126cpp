#pragma once

#include "network/Packet.h"

class Packet39 : public Packet {
public:
	int_t field_6365_a;
	int_t field_6364_b;
	
	Packet39();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
