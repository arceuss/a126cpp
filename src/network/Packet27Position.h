#pragma once

#include "network/Packet.h"

// Packet27Position - matches Java Packet27Position.java exactly
// Stance update packet - bidirectional
class Packet27Position : public Packet {
public:
	float field_22039_a;
	float field_22038_b;
	float field_22041_e;
	float field_22040_f;
	bool field_22043_c;
	bool field_22042_d;
	
	Packet27Position();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
