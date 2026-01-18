#pragma once

#include "network/Packet.h"

// Packet0KeepAlive - matches Java Packet0KeepAlive.java exactly
class Packet0KeepAlive : public Packet {
public:
	Packet0KeepAlive();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
