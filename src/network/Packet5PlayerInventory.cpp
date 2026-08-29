#include "network/Packet5PlayerInventory.h"
#include "network/NetHandler.h"

Packet5PlayerInventory::Packet5PlayerInventory()
	: entityID(0)
	, slot(0)
	, itemID(0)
	, itemDamage(0)
{
}

void Packet5PlayerInventory::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.entityID = var1.readInt();
	this->entityID = in.readInt();
	
	// this.slot = var1.readShort();
	this->slot = in.readShort();
	
	// this.itemID = var1.readShort();
	this->itemID = in.readShort();
	
	// this.itemDamage = var1.readShort();
	this->itemDamage = in.readShort();
}

void Packet5PlayerInventory::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.entityID);
	out.writeInt(this->entityID);
	
	// var1.writeShort(this.slot);
	out.writeShort(static_cast<short_t>(this->slot));
	
	// var1.writeShort(this.itemID);
	out.writeShort(static_cast<short_t>(this->itemID));
	
	// var1.writeShort(this.itemDamage);
	out.writeShort(static_cast<short_t>(this->itemDamage));
}

void Packet5PlayerInventory::processPacket(NetHandler* handler)
{
	// Java: var1.handlePlayerInventory(this);
	handler->handlePlayerInventory(this);
}

int Packet5PlayerInventory::getPacketSize()
{
	// Java: return 8;
	return 8;
}

int Packet5PlayerInventory::getPacketId() const
{
	return 5;
}
