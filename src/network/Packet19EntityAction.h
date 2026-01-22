#pragma once

#include "network/Packet.h"

// Packet19EntityAction - matches Java Packet19EntityAction.java exactly
// Entity Action packet - client to server
class Packet19EntityAction : public Packet {
public:
	int_t entityId;
	int_t state;  // Note: Java uses int but reads/writes as byte
	
	Packet19EntityAction();
	Packet19EntityAction(int_t entityId, int_t state);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
