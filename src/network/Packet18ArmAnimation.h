#pragma once

#include "network/Packet.h"

// Packet18ArmAnimation - matches Java Packet18ArmAnimation.java exactly
// Animation packet - bidirectional
class Packet18ArmAnimation : public Packet {
public:
	int_t entityId;
	int_t animate;  // Note: Java uses int but reads/writes as byte
	
	Packet18ArmAnimation();
	Packet18ArmAnimation(int_t entityId, int_t animate);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
