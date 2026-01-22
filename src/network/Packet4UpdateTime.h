#pragma once

#include "network/Packet.h"

// Packet4UpdateTime - matches Java Packet4UpdateTime.java exactly
class Packet4UpdateTime : public Packet {
public:
	long_t time;
	
	Packet4UpdateTime();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
