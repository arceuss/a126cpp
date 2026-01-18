#pragma once

#include "network/Packet.h"

// Packet32EntityLook - matches Java Packet32EntityLook.java exactly
// Entity Look packet - server to client
// Note: In Java, this extends Packet30Entity, so it calls super.readPacketData (reads entityId)
class Packet32EntityLook : public Packet {
public:
	int_t entityId;  // From Packet30Entity super
	byte_t yaw;
	byte_t pitch;
	
	Packet32EntityLook();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
