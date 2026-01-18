#include "network/Packet25EntityPainting.h"
#include "network/NetHandler.h"

Packet25EntityPainting::Packet25EntityPainting()
	: entityId(0)
	, title(u"")
	, xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, direction(0)
{
}

void Packet25EntityPainting::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityId = var1.readInt();
	this->entityId = in.readInt();
	
	// this.title = readString(var1, EnumArt.SkullAndRoses.field_1624_y.length());
	// Max length is 13 (length of "SkullAndRoses")
	this->title = Packet::readString(in, 13);
	
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readInt();
	this->yPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.direction = var1.readInt();
	this->direction = in.readInt();
}

void Packet25EntityPainting::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityId);
	out.writeInt(this->entityId);
	
	// writeString(this.title, var1);
	Packet::writeString(this->title, out);
	
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.yPosition);
	out.writeInt(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.writeInt(this.direction);
	out.writeInt(this->direction);
}

void Packet25EntityPainting::processPacket(NetHandler* handler)
{
	// Java: var1.handlePainting(this);
	handler->handlePainting(this);
}

int Packet25EntityPainting::getPacketSize()
{
	// Java: variable size - 24 bytes + string length
	// int (4) + string (2 + title.length()*2) + int (4) + int (4) + int (4) + int (4)
	// = 22 + title.length()*2
	// Approximate - actual depends on string length
	return 24 + static_cast<int>(title.length() * 2);
}

int Packet25EntityPainting::getPacketId() const
{
	return 25;
}
