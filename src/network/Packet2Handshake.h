#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet2Handshake - matches Java Packet2Handshake.java exactly
class Packet2Handshake : public Packet {
public:
	jstring username;
	
	Packet2Handshake();
	Packet2Handshake(const jstring& username);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
