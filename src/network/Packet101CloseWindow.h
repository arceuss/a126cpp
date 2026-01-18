#pragma once

#include "network/Packet.h"

// Packet101CloseWindow - matches Java Packet101CloseWindow.java exactly
// Close window packet - bidirectional (0x65)
class Packet101CloseWindow : public Packet {
public:
	int_t windowId;  // Note: Java uses int but reads/writes as byte
	
	Packet101CloseWindow();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
