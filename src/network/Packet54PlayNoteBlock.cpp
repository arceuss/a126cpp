#include "network/Packet54PlayNoteBlock.h"
#include "network/NetHandler.h"

Packet54PlayNoteBlock::Packet54PlayNoteBlock()
	: xLocation(0)
	, yLocation(0)
	, zLocation(0)
	, instrumentType(0)
	, pitch(0)
{
}

void Packet54PlayNoteBlock::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.xLocation = var1.readInt();
	this->xLocation = in.readInt();
	
	// this.yLocation = var1.readShort();
	this->yLocation = in.readShort();
	
	// this.zLocation = var1.readInt();
	this->zLocation = in.readInt();
	
	// this.instrumentType = var1.read();
	this->instrumentType = static_cast<int_t>(in.read() & 0xFF);
	
	// this.pitch = var1.read();
	this->pitch = static_cast<int_t>(in.read() & 0xFF);
}

void Packet54PlayNoteBlock::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.xLocation);
	out.writeInt(this->xLocation);
	
	// var1.writeShort(this.yLocation);
	out.writeShort(static_cast<short_t>(this->yLocation));
	
	// var1.writeInt(this.zLocation);
	out.writeInt(this->zLocation);
	
	// var1.write(this.instrumentType);
	out.writeByte(static_cast<byte_t>(this->instrumentType));
	
	// var1.write(this.pitch);
	out.writeByte(static_cast<byte_t>(this->pitch));
}

void Packet54PlayNoteBlock::processPacket(NetHandler* handler)
{
	// Java: var1.handleNoteBlock(this);
	handler->handleNoteBlock(this);
}

int Packet54PlayNoteBlock::getPacketSize()
{
	// Java: return 12;
	// int (4) + short (2) + int (4) + byte (1) + byte (1) = 12
	return 12;
}

int Packet54PlayNoteBlock::getPacketId() const
{
	return 54;
}
