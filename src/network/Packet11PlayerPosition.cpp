#include "network/Packet11PlayerPosition.h"

Packet11PlayerPosition::Packet11PlayerPosition()
{
	// Java: this.moving = true;
	this->moving = true;
}

Packet11PlayerPosition::Packet11PlayerPosition(double x, double y, double stance, double z, bool onGround)
{
	// Java: this.xPosition = var1;
	this->xPosition = x;
	
	// this.yPosition = var3;
	this->yPosition = y;
	
	// this.stance = var5;
	this->stance = stance;
	
	// this.zPosition = var7;
	this->zPosition = z;
	
	// this.onGround = var9;
	this->onGround = onGround;
	
	// this.moving = true;
	this->moving = true;
}

void Packet11PlayerPosition::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER - must match byte sequence
	// this.xPosition = var1.readDouble();
	this->xPosition = in.readDouble();
	
	// this.yPosition = var1.readDouble();
	this->yPosition = in.readDouble();
	
	// this.stance = var1.readDouble();
	this->stance = in.readDouble();
	
	// this.zPosition = var1.readDouble();
	this->zPosition = in.readDouble();
	
	// super.readPacketData(var1);
	Packet10Flying::readPacketData(in);
}

void Packet11PlayerPosition::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER - must match byte sequence
	// var1.writeDouble(this.xPosition);
	out.writeDouble(this->xPosition);
	
	// var1.writeDouble(this.yPosition);
	out.writeDouble(this->yPosition);
	
	// var1.writeDouble(this.stance);
	out.writeDouble(this->stance);
	
	// var1.writeDouble(this.zPosition);
	out.writeDouble(this->zPosition);
	
	// super.writePacketData(var1);
	Packet10Flying::writePacketData(out);
}

int Packet11PlayerPosition::getPacketSize()
{
	// Java: return 33;
	// double (8) * 4 = 32, plus 1 byte from super (onGround) = 33
	return 33;
}

int Packet11PlayerPosition::getPacketId() const
{
	return 11;
}
