#pragma once

#include "network/Packet10Flying.h"

// Packet11PlayerPosition - matches Java Packet11PlayerPosition.java exactly
// Extends Packet10Flying
class Packet11PlayerPosition : public Packet10Flying {
public:
	Packet11PlayerPosition();
	Packet11PlayerPosition(double x, double y, double stance, double z, bool onGround);
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
