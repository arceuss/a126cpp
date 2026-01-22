#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet1Login - matches Java Packet1Login.java exactly
class Packet1Login : public Packet {
public:
	int_t protocolVersion;
	jstring username;
	long_t field_4074_d;  // Map seed
	byte_t field_4073_e;  // Dimension
	
	Packet1Login();
	Packet1Login(const jstring& username, int_t protocolVersion);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
