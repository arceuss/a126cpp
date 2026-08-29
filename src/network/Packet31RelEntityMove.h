#pragma once

#include "network/Packet30Entity.h"

class Packet31RelEntityMove : public Packet30Entity {
public:

	Packet31RelEntityMove();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
