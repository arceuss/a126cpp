#pragma once

#include "network/Packet.h"

// Packet54PlayNoteBlock - matches Java Packet54PlayNoteBlock.java exactly
// Block Action packet - server to client (0x36)
class Packet54PlayNoteBlock : public Packet {
public:
	int_t xLocation;
	int_t yLocation;  // Note: Java uses int but reads/writes as short
	int_t zLocation;
	int_t instrumentType;  // Note: Java uses int but reads/writes as byte
	int_t pitch;  // Note: Java uses int but reads/writes as byte
	
	Packet54PlayNoteBlock();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
