#include "network/Packet10Flying.h"
#include "network/NetHandler.h"

Packet10Flying::Packet10Flying()
	: xPosition(0.0)
	, yPosition(0.0)
	, zPosition(0.0)
	, stance(0.0)
	, yaw(0.0f)
	, pitch(0.0f)
	, onGround(false)
	, moving(false)
	, rotating(false)
{
}

Packet10Flying::Packet10Flying(bool onGround)
	: xPosition(0.0)
	, yPosition(0.0)
	, zPosition(0.0)
	, stance(0.0)
	, yaw(0.0f)
	, pitch(0.0f)
	, onGround(onGround)
	, moving(false)
	, rotating(false)
{
}

void Packet10Flying::readPacketData(SocketInputStream& in)
{
	// Java: this.onGround = var1.read() != 0;
	int b = in.read();
	this->onGround = (b != 0);
}

void Packet10Flying::writePacketData(SocketOutputStream& out)
{
	// Java: var1.write(this.onGround ? 1 : 0);
	out.write(this->onGround ? 1 : 0);
}

void Packet10Flying::processPacket(NetHandler* handler)
{
	// Java: var1.handleFlying(this);
	handler->handleFlying(this);
}

int Packet10Flying::getPacketSize()
{
	// Java: return 1;
	return 1;
}

int Packet10Flying::getPacketId() const
{
	return 10;
}
