#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet100OpenWindow - matches Java Packet100OpenWindow.java exactly
// Open window packet - server to client (0x64)
class Packet100OpenWindow : public Packet {
public:
	int_t windowId;  // Note: Java uses int but reads/writes as byte
	int_t inventoryType;  // Note: Java uses int but reads/writes as byte
	jstring windowTitle;  // Note: Uses readUTF (Modified UTF-8), not readString (UTF-16)
	int_t slotsCount;  // Note: Java uses int but reads/writes as byte
	
	Packet100OpenWindow();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
	
private:
	// Helper to read UTF-8 string (Modified UTF-8 format)
	jstring readUTF(SocketInputStream& in);
};
