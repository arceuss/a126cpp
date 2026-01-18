#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet25EntityPainting - matches Java Packet25EntityPainting.java exactly
// Entity Painting packet - server to client
class Packet25EntityPainting : public Packet {
public:
	int_t entityId;
	jstring title;
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	int_t direction;
	
	Packet25EntityPainting();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
