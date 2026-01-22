#include "network/Packet255KickDisconnect.h"
#include "network/NetHandler.h"

Packet255KickDisconnect::Packet255KickDisconnect()
	: reason(u"")
{
}

Packet255KickDisconnect::Packet255KickDisconnect(const jstring& reason)
	: reason(reason)
{
}

void Packet255KickDisconnect::readPacketData(SocketInputStream& in)
{
	// Java: this.reason = readString(var1, 100);
	this->reason = Packet::readString(in, 100);
}

void Packet255KickDisconnect::writePacketData(SocketOutputStream& out)
{
	// Java: writeString(this.reason, var1);
	// Java side reads this with readString(in, 100). Enforce the same limit here
	// so we never send a string that would be rejected by the server.
	Packet::writeString(this->reason, out, 100);
}

void Packet255KickDisconnect::processPacket(NetHandler* handler)
{
	// Java: var1.handleKickDisconnect(this);
	handler->handleKickDisconnect(this);
}

int Packet255KickDisconnect::getPacketSize()
{
	// Java: return this.reason.length();
	return static_cast<int>(reason.length());
}

int Packet255KickDisconnect::getPacketId() const
{
	return 255;
}
