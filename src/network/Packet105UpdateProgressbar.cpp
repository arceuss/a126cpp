#include "network/Packet105UpdateProgressbar.h"
#include "network/NetHandler.h"

Packet105UpdateProgressbar::Packet105UpdateProgressbar()
	: windowId(0)
	, progressBar(0)
	, progressBarValue(0)
{
}

void Packet105UpdateProgressbar::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.windowId = var1.readByte();
	this->windowId = static_cast<int_t>(in.read() & 0xFF);
	
	// this.progressBar = var1.readShort();
	this->progressBar = in.readShort();
	
	// this.progressBarValue = var1.readShort();
	this->progressBarValue = in.readShort();
}

void Packet105UpdateProgressbar::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.windowId);
	out.writeByte(static_cast<byte_t>(this->windowId));
	
	// var1.writeShort(this.progressBar);
	out.writeShort(static_cast<short_t>(this->progressBar));
	
	// var1.writeShort(this.progressBarValue);
	out.writeShort(static_cast<short_t>(this->progressBarValue));
}

void Packet105UpdateProgressbar::processPacket(NetHandler* handler)
{
	// Java: var1.func_20090_a(this);
	handler->func_20090_a(this);
}

int Packet105UpdateProgressbar::getPacketSize()
{
	// Java: return 5;
	// byte (1) + short (2) + short (2) = 5
	return 5;
}

int Packet105UpdateProgressbar::getPacketId() const
{
	return 105;
}
