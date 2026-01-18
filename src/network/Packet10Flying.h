#pragma once

#include "network/Packet.h"

// Packet10Flying - matches Java Packet10Flying.java exactly
class Packet10Flying : public Packet {
public:
	double xPosition;
	double yPosition;
	double zPosition;
	double stance;
	float yaw;
	float pitch;
	bool onGround;
	bool moving;
	bool rotating;
	
	Packet10Flying();
	Packet10Flying(bool onGround);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
