#pragma once

#include "network/Packet.h"

// Packet17Sleep - matches Java Packet17Sleep.java exactly
// Use Bed packet - server to client
class Packet17Sleep : public Packet {
public:
	int_t field_22045_a;  // Entity ID
	int_t field_22044_b;  // X coordinate
	int_t field_22048_c;  // Y coordinate (byte)
	int_t field_22047_d;  // Z coordinate
	int_t field_22046_e;  // In bed flag (byte)
	
	Packet17Sleep();
	Packet17Sleep(int_t entityId, int_t x, byte_t y, int_t z, byte_t inBed);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
