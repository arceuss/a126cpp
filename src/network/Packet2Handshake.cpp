#include "network/Packet2Handshake.h"
#include "network/NetHandler.h"

Packet2Handshake::Packet2Handshake()
	: username(u"")
{
}

Packet2Handshake::Packet2Handshake(const jstring& username)
	: username(username)
{
}

void Packet2Handshake::readPacketData(SocketInputStream& in)
{
	// Java: this.username = readString(var1, 32);
	this->username = Packet::readString(in, 32);
}

void Packet2Handshake::writePacketData(SocketOutputStream& out)
{
	// Java: writeString(this.username, var1);
	Packet::writeString(this->username, out);
}

void Packet2Handshake::processPacket(NetHandler* handler)
{
	// Java: var1.handleHandshake(this);
	handler->handleHandshake(this);
}

int Packet2Handshake::getPacketSize()
{
	// Java: return 4 + this.username.length() + 4;
	// 2 (short for string length) + username.length()*2 (UTF-16 bytes) = 2 + username.length()*2
	// But Java formula suggests: 4 + username.length() + 4
	// Let's match Java exactly:
	return 4 + static_cast<int>(username.length()) + 4;
}

int Packet2Handshake::getPacketId() const
{
	return 2;
}
