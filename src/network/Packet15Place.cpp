#include "network/Packet15Place.h"
#include "network/NetHandler.h"
#include "world/item/ItemStack.h"
#include <memory>

Packet15Place::Packet15Place()
	: xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, direction(0)
	, itemStack(nullptr)
{
}

Packet15Place::Packet15Place(int_t x, int_t y, int_t z, int_t direction, std::shared_ptr<ItemStack> itemStack)
	: xPosition(x)
	, yPosition(static_cast<byte_t>(y))
	, zPosition(z)
	, direction(static_cast<byte_t>(direction))
	, itemStack(itemStack)
{
}

void Packet15Place::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.read();
	this->yPosition = static_cast<byte_t>(in.read() & 0xFF);
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.direction = var1.read();
	this->direction = static_cast<byte_t>(in.read() & 0xFF);
	
	// short var2 = var1.readShort();
	short_t itemIdShort = in.readShort();
	
	// if(var2 >= 0) {
	if (itemIdShort >= 0)
	{
		// byte var3 = var1.readByte();
		byte_t count = in.readByte();
		
		// short var4 = var1.readShort();
		short_t damage = in.readShort();
		
		// this.itemStack = new ItemStack(var2, var3, var4);
		this->itemStack = std::make_shared<ItemStack>(itemIdShort, count, damage);
	}
	else
	{
		// this.itemStack = null;
		this->itemStack = nullptr;
	}
}

void Packet15Place::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.write(this.yPosition);
	out.writeByte(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.write(this.direction);
	out.writeByte(this->direction);
	
	// if(this.itemStack == null) {
	if (this->itemStack == nullptr)
	{
		// var1.writeShort(-1);
		out.writeShort(-1);
	}
	else
	{
		// var1.writeShort(this.itemStack.itemID);
		out.writeShort(static_cast<short_t>(this->itemStack->itemID));
		
		// var1.writeByte(this.itemStack.stackSize);
		out.writeByte(static_cast<byte_t>(this->itemStack->stackSize));
		
		// var1.writeShort(this.itemStack.getItemDamage());
		out.writeShort(static_cast<short_t>(this->itemStack->itemDamage));
	}
}

void Packet15Place::processPacket(NetHandler* handler)
{
	// Java: var1.handlePlace(this);
	handler->handlePlace(this);
}

int Packet15Place::getPacketSize()
{
	// Java: return 15;
	// int (4) + byte (1) + int (4) + byte (1) + short (2) [+ optional: byte(1) + short(2)] = 12 or 15
	return 15;
}

int Packet15Place::getPacketId() const
{
	return 15;
}
