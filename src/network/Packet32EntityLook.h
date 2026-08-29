#pragma once

#include "network/Packet30Entity.h"

class Packet32EntityLook : public Packet30Entity {
public:

	Packet32EntityLook();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
