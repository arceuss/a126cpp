#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet255KickDisconnect - matches Java Packet255KickDisconnect.java exactly
class Packet255KickDisconnect : public Packet {
public:
	jstring reason;
	
	Packet255KickDisconnect();
	Packet255KickDisconnect(const jstring& reason);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
