#pragma once

#include "network/Packet.h"

// Packet105UpdateProgressbar - matches Java Packet105UpdateProgressbar.java exactly
// Update progress bar packet - server to client (0x69)
class Packet105UpdateProgressbar : public Packet {
public:
	int_t windowId;  // Note: Java uses int but reads/writes as byte
	int_t progressBar;  // Note: Java uses int but reads/writes as short
	int_t progressBarValue;  // Note: Java uses int but reads/writes as short
	
	Packet105UpdateProgressbar();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
