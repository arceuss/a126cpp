#pragma once

#include "network/Packet.h"

// Packet39 - matches newb12 SetRidingPacket.java exactly
// Set Riding packet - server to client (0x27 / packet ID 39)
// Beta 1.2: SetRidingPacket - handles entity riding relationships
class Packet39 : public Packet {
public:
	int_t riderId;   // Rider entity ID (matches newb12 SetRidingPacket.riderId)
	int_t riddenId;  // Ridden entity ID, -1 for dismount (matches newb12 SetRidingPacket.riddenId)
	
	Packet39();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
