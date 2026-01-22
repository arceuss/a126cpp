#include "network/Packet12PlayerLook.h"

Packet12PlayerLook::Packet12PlayerLook()
{
	// Java: this.rotating = true;
	this->rotating = true;
}

Packet12PlayerLook::Packet12PlayerLook(float yaw, float pitch, bool onGround)
{
	// Java: this.yaw = var1;
	this->yaw = yaw;
	
	// this.pitch = var2;
	this->pitch = pitch;
	
	// this.onGround = var3;
	this->onGround = onGround;
	
	// this.rotating = true;
	this->rotating = true;
}

void Packet12PlayerLook::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.yaw = var1.readFloat();
	this->yaw = in.readFloat();
	
	// this.pitch = var1.readFloat();
	this->pitch = in.readFloat();
	
	// super.readPacketData(var1);
	Packet10Flying::readPacketData(in);
}

void Packet12PlayerLook::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeFloat(this.yaw);
	out.writeFloat(this->yaw);
	
	// var1.writeFloat(this.pitch);
	out.writeFloat(this->pitch);
	
	// super.writePacketData(var1);
	Packet10Flying::writePacketData(out);
}

int Packet12PlayerLook::getPacketSize()
{
	// Java: return 9;
	// float (4) * 2 = 8, plus 1 byte from super (onGround) = 9
	return 9;
}

int Packet12PlayerLook::getPacketId() const
{
	return 12;
}
