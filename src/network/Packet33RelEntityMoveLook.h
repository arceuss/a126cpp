#pragma once

#include "network/Packet30Entity.h"

class Packet33RelEntityMoveLook : public Packet30Entity {
public:

	Packet33RelEntityMoveLook();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
