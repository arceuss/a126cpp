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
	// int (4) + short (2) + short (2) + short (2) = 10 bytes?
	// Wait, Java says 8. Let me check: entityID is int (4), slot is short (2), itemID is short (2), itemDamage is short (2)
	// That's 4+2+2+2 = 10. But Java says 8. Maybe slot/itemID/itemDamage are stored as int but written as short?
	// Actually, Java short is 2 bytes, so: 4 + 2 + 2 + 2 = 10
	// But the Java code says return 8. This might be a mistake in the original, or maybe the calculation is wrong.
	// Let's match Java exactly as written:
	return 8;
}

int Packet5PlayerInventory::getPacketId() const
{
	return 5;
}
