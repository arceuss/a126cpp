#include "network/Packet14BlockDig.h"
#include "network/NetHandler.h"

Packet14BlockDig::Packet14BlockDig()
	: xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, face(0)
	, status(0)
{
}

Packet14BlockDig::Packet14BlockDig(int_t status, int_t x, int_t y, int_t z, int_t face)
	: status(status)
	, xPosition(x)
	, yPosition(y)
	, zPosition(z)
	, face(face)
{
}

void Packet14BlockDig::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.status = var1.read();
	this->status = in.read();
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.read();
	this->yPosition = in.read();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.face = var1.read();
	this->face = in.read();
}

void Packet14BlockDig::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.write(this.status);
	out.write(this->status);
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.write(this.yPosition);
	out.write(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.write(this.face);
	out.write(this->face);
}

void Packet14BlockDig::processPacket(NetHandler* handler)
{
	// Java: var1.handleBlockDig(this);
	handler->handleBlockDig(this);
}

int Packet14BlockDig::getPacketSize()
{
	// Java: return 11;
	// byte (1) + int (4) + byte (1) + int (4) + byte (1) = 11
	return 11;
}

int Packet14BlockDig::getPacketId() const
{
	return 14;
}
