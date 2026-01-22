#include "network/Packet130UpdateSign.h"
#include "network/NetHandler.h"

Packet130UpdateSign::Packet130UpdateSign()
	: xPosition(0)
	, yPosition(0)
	, zPosition(0)
{
	for (int i = 0; i < 4; ++i)
	{
		signLines[i] = u"";
	}
}

void Packet130UpdateSign::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readShort();
	this->yPosition = in.readShort();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.signLines = new String[4];
	// for(int var2 = 0; var2 < 4; ++var2) {
	//     this.signLines[var2] = readString(var1, 15);
	// }
	for (int i = 0; i < 4; ++i)
	{
		this->signLines[i] = Packet::readString(in, 15);
	}
}

void Packet130UpdateSign::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeShort(this.yPosition);
	out.writeShort(static_cast<short_t>(this->yPosition));
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// for(int var2 = 0; var2 < 4; ++var2) {
	//     writeString(this.signLines[var2], var1);
	// }
	for (int i = 0; i < 4; ++i)
	{
		Packet::writeString(this->signLines[i], out);
	}
}

void Packet130UpdateSign::processPacket(NetHandler* handler)
{
	// Java: var1.handleSignUpdate(this);
	handler->handleSignUpdate(this);
}

int Packet130UpdateSign::getPacketSize()
{
	// Java: variable size
	// int (4) + short (2) + int (4) + 4 strings (each: 2 + length*2)
	int size = 10;
	for (int i = 0; i < 4; ++i)
	{
		size += 2 + static_cast<int>(signLines[i].length() * 2);
	}
	return size;
}

int Packet130UpdateSign::getPacketId() const
{
	return 130;
}
