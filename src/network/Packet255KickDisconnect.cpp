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
	// Alpha Packet255KickDisconnect.java:24-25.
	this->reason = Packet::readString(in, 100);
}

void Packet255KickDisconnect::writePacketData(SocketOutputStream& out)
{
	// Alpha Packet255KickDisconnect.java:29-30. writeString only applies
	// Packet's Short.MAX_VALUE limit; the 100-character limit is read-side.
	Packet::writeString(this->reason, out);
}

void Packet255KickDisconnect::processPacket(NetHandler* handler)
{
	// Alpha Packet255KickDisconnect.java:34-35.
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
