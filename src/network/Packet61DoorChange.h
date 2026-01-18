#pragma once

#include "network/Packet.h"

// Packet61DoorChange - matches Java Packet61DoorChange.java exactly
// Sound effect packet - server to client (0x3D)
class Packet61DoorChange : public Packet {
public:
	int_t field_28050_a;  // Effect ID
	int_t field_28049_b;  // Sound data
	int_t field_28053_c;  // X coordinate
	int_t field_28052_d;  // Y coordinate (byte in protocol, int in class)
	int_t field_28051_e;  // Z coordinate
	
	Packet61DoorChange();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
