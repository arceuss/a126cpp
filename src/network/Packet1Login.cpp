#include "network/Packet1Login.h"
#include "network/NetHandler.h"

Packet1Login::Packet1Login()
	: protocolVersion(0)
	, username(u"")
	, field_4074_d(0)
	, field_4073_e(0)
{
}

Packet1Login::Packet1Login(const jstring& username, int_t protocolVersion)
	: protocolVersion(protocolVersion)
	, username(username)
	, field_4074_d(0)
	, field_4073_e(0)
{
}

void Packet1Login::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER - must match byte sequence
	// this.protocolVersion = var1.readInt();
	this->protocolVersion = in.readInt();
	
	// this.username = readString(var1, 16);
	this->username = Packet::readString(in, 16);
	
	// this.field_4074_d = var1.readLong();
	this->field_4074_d = in.readLong();
	
	// this.field_4073_e = var1.readByte();
	this->field_4073_e = in.readByte();
}

void Packet1Login::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER - must match byte sequence
	// var1.writeInt(this.protocolVersion);
	out.writeInt(this->protocolVersion);
	
	// writeString(this.username, var1);
	Packet::writeString(this->username, out);
	
	// var1.writeLong(this.field_4074_d);
	out.writeLong(this->field_4074_d);
	
	// var1.writeByte(this.field_4073_e);
	out.writeByte(this->field_4073_e);
}

void Packet1Login::processPacket(NetHandler* handler)
{
	// Java: var1.handleLogin(this);
	handler->handleLogin(this);
}

int Packet1Login::getPacketSize()
{
	// Java: return 4 + this.username.length() + 4 + 5;
	return 4 + static_cast<int>(username.length()) + 4 + 5;
}

int Packet1Login::getPacketId() const
{
	return 1;
}
