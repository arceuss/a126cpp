#pragma once

#include "network/Packet.h"
#include "java/String.h"

// Sign update packet (0x82).
class Packet130UpdateSign : public Packet {
public:
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	jstring signLines[4];

	Packet130UpdateSign();
	Packet130UpdateSign(int_t xPosition, int_t yPosition, int_t zPosition,
		const jstring (&signLines)[4]);
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
