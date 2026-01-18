#pragma once

#include "network/Packet10Flying.h"

// Packet13PlayerLookMove - matches Java Packet13PlayerLookMove.java exactly
// Extends Packet10Flying
class Packet13PlayerLookMove : public Packet10Flying {
public:
	Packet13PlayerLookMove();
	Packet13PlayerLookMove(double x, double y, double stance, double z, float yaw, float pitch, bool onGround);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
