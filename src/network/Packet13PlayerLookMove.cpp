#include "network/Packet13PlayerLookMove.h"

Packet13PlayerLookMove::Packet13PlayerLookMove()
{
	// Java: this.rotating = true;
	//       this.moving = true;
	this->rotating = true;
	this->moving = true;
}

Packet13PlayerLookMove::Packet13PlayerLookMove(double x, double y, double stance, double z, float yaw, float pitch, bool onGround)
{
	// Java: this.xPosition = var1;
	this->xPosition = x;
	
	// this.yPosition = var3;
	this->yPosition = y;
	
	// this.stance = var5;
	this->stance = stance;
	
	// this.zPosition = var7;
	this->zPosition = z;
	
	// this.yaw = var9;
	this->yaw = yaw;
	
	// this.pitch = var10;
	this->pitch = pitch;
	
	// this.onGround = var11;
	this->onGround = onGround;
	
	// this.rotating = true;
	this->rotating = true;
	
	// this.moving = true;
	this->moving = true;
}

void Packet13PlayerLookMove::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER - must match byte sequence exactly
	// this.xPosition = var1.readDouble();
	this->xPosition = in.readDouble();
	
	// this.yPosition = var1.readDouble();
	this->yPosition = in.readDouble();
	
	// this.stance = var1.readDouble();
	this->stance = in.readDouble();
	
	// this.zPosition = var1.readDouble();
	this->zPosition = in.readDouble();
	
	// this.yaw = var1.readFloat();
	this->yaw = in.readFloat();
	
	// this.pitch = var1.readFloat();
	this->pitch = in.readFloat();
	
	// super.readPacketData(var1);
	Packet10Flying::readPacketData(in);
}

void Packet13PlayerLookMove::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER - must match byte sequence exactly
	// var1.writeDouble(this.xPosition);
	out.writeDouble(this->xPosition);
	
	// var1.writeDouble(this.yPosition);
	out.writeDouble(this->yPosition);
	
	// var1.writeDouble(this.stance);
	out.writeDouble(this->stance);
	
	// var1.writeDouble(this.zPosition);
	out.writeDouble(this->zPosition);
	
	// var1.writeFloat(this.yaw);
	out.writeFloat(this->yaw);
	
	// var1.writeFloat(this.pitch);
	out.writeFloat(this->pitch);
	
	// super.writePacketData(var1);
	Packet10Flying::writePacketData(out);
}

int Packet13PlayerLookMove::getPacketSize()
{
	// Java: return 41;
	// double (8) * 4 = 32, float (4) * 2 = 8, plus 1 byte from super (onGround) = 41
	return 41;
}

int Packet13PlayerLookMove::getPacketId() const
{
	return 13;
}
