#include "network/Packet18ArmAnimation.h"
#include "network/NetHandler.h"

Packet18ArmAnimation::Packet18ArmAnimation()
	: entityId(0)
	, animate(0)
{
}

Packet18ArmAnimation::Packet18ArmAnimation(int_t entityId, int_t animate)
	: entityId(entityId)
	, animate(animate)
{
}

void Packet18ArmAnimation::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.animate = var1.readByte();
	this->animate = static_cast<int_t>(in.read() & 0xFF);
}

void Packet18ArmAnimation::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// var1.writeByte(this.animate);
	out.writeByte(static_cast<byte_t>(this->animate));
}

void Packet18ArmAnimation::processPacket(NetHandler* handler)
{
	// Java: var1.handleArmAnimation(this);
	handler->handleArmAnimation(this);
}

int Packet18ArmAnimation::getPacketSize()
{
	// Java: return 5;
	// int (4) + byte (1) = 5
	return 5;
}

int Packet18ArmAnimation::getPacketId() const
{
	return 18;
}
