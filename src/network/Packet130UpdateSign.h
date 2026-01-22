#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet130UpdateSign - matches Java Packet130UpdateSign.java exactly
// Update Sign packet - bidirectional (0x82)
class Packet130UpdateSign : public Packet {
public:
	int_t xPosition;
	int_t yPosition;  // Note: Java uses int but reads/writes as short
	int_t zPosition;
	jstring signLines[4];  // Array of 4 strings
	
	Packet130UpdateSign();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
