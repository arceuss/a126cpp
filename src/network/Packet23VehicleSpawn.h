#pragma once

#include "network/Packet.h"

// Packet23VehicleSpawn - matches Java Packet23VehicleSpawn.java exactly
// Add Object/Vehicle packet - server to client
class Packet23VehicleSpawn : public Packet {
public:
	int_t entityId;
	int_t type;  // Note: Java uses int but reads/writes as byte
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	int_t field_28044_i;  // Unknown flag - if > 0, following fields are sent
	int_t field_28047_e;  // Unknown (short)
	int_t field_28046_f;  // Unknown (short)
	int_t field_28045_g;  // Unknown (short)
	
	Packet23VehicleSpawn();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
