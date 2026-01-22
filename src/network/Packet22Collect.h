#pragma once

#include "network/Packet.h"

// Packet22Collect - matches Java Packet22Collect.java exactly
// Collect Item packet - server to client
class Packet22Collect : public Packet {
public:
	int_t collectedEntityId;
	int_t collectorEntityId;
	
	Packet22Collect();
	Packet22Collect(int_t collectedEntityId, int_t collectorEntityId);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
