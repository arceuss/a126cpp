#include "network/Packet101CloseWindow.h"
#include "network/NetHandler.h"

Packet101CloseWindow::Packet101CloseWindow()
	: windowId(0)
{
}

void Packet101CloseWindow::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.windowId = var1.readByte();
	this->windowId = static_cast<int_t>(in.read() & 0xFF);
}

void Packet101CloseWindow::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.windowId);
	out.writeByte(static_cast<byte_t>(this->windowId));
}

void Packet101CloseWindow::processPacket(NetHandler* handler)
{
	// Java: var1.func_20092_a(this);
	handler->func_20092_a(this);
}

int Packet101CloseWindow::getPacketSize()
{
	// Java: return 1;
	// byte (1) = 1
	return 1;
}

int Packet101CloseWindow::getPacketId() const
{
	return 101;
}
