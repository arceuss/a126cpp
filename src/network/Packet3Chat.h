#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet3Chat - matches Java Packet3Chat.java exactly
class Packet3Chat : public Packet {
public:
	jstring message;
	
	Packet3Chat();
	Packet3Chat(const jstring& message);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
