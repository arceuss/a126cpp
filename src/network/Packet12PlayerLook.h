#pragma once

#include "network/Packet10Flying.h"

// Packet12PlayerLook - matches Java Packet12PlayerLook.java exactly
// Extends Packet10Flying
class Packet12PlayerLook : public Packet10Flying {
public:
	Packet12PlayerLook();
	Packet12PlayerLook(float yaw, float pitch, bool onGround);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
