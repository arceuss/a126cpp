#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Packet62Sound - matches Java Packet62Sound.java exactly
// Sound effect packet - server to client (0x3D)
class Packet62Sound : public Packet {
public:
	jstring sound;  // Note: Java uses readUTF (Modified UTF-8), not readString (UTF-16)
	double locX;
	double locY;
	double locZ;
	float f;   // volume
	float f1;  // pitch
	
	Packet62Sound();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
