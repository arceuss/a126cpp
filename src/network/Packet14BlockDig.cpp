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
	: status(static_cast<byte_t>(status))
	, xPosition(x)
	, yPosition(static_cast<byte_t>(y))
	, zPosition(z)
	, face(static_cast<byte_t>(face))
{
}

void Packet14BlockDig::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.status = var1.read();
	this->status = static_cast<byte_t>(in.read() & 0xFF);
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.read();
	this->yPosition = static_cast<byte_t>(in.read() & 0xFF);
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.face = var1.read();
	this->face = static_cast<byte_t>(in.read() & 0xFF);
}

void Packet14BlockDig::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.write(this.status);
	out.writeByte(this->status);
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.write(this.yPosition);
	out.writeByte(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.write(this.face);
	out.writeByte(this->face);
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
