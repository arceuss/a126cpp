#include "network/Packet103SetSlot.h"
#include "network/NetHandler.h"

Packet103SetSlot::Packet103SetSlot()
	: windowId(0)
	, itemSlot(0)
	, myItemStack(nullptr)
{
}

void Packet103SetSlot::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.windowId = var1.readByte();
	this->windowId = static_cast<int_t>(in.read() & 0xFF);
	
	// this.itemSlot = var1.readShort();
	this->itemSlot = in.readShort();
	
	// short var2 = var1.readShort();
	short_t itemIdShort = in.readShort();
	
	// if(var2 >= 0) {
	if (itemIdShort >= 0)
	{
		// byte var3 = var1.readByte();
		byte_t count = in.readByte();
		
		// short var4 = var1.readShort();
		short_t damage = in.readShort();
		
		// this.myItemStack = new ItemStack(var2, var3, var4);
		this->myItemStack = std::make_shared<ItemStack>(itemIdShort, count, damage);
	}
	else
	{
		// this.myItemStack = null;
		this->myItemStack = nullptr;
	}
}

void Packet103SetSlot::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.windowId);
	out.writeByte(static_cast<byte_t>(this->windowId));
	
	// var1.writeShort(this.itemSlot);
	out.writeShort(static_cast<short_t>(this->itemSlot));
	
	// if(this.myItemStack == null) {
	if (this->myItemStack == nullptr)
	{
		// var1.writeShort(-1);
		out.writeShort(-1);
	}
	else
	{
		// var1.writeShort(this.myItemStack.itemID);
		out.writeShort(static_cast<short_t>(this->myItemStack->itemID));
		
		// var1.writeByte(this.myItemStack.stackSize);
		out.writeByte(static_cast<byte_t>(this->myItemStack->stackSize));
		
		// var1.writeShort(this.myItemStack.itemDamage);
		out.writeShort(static_cast<short_t>(this->myItemStack->itemDamage));
	}
}

void Packet103SetSlot::processPacket(NetHandler* handler)
{
	// Java: var1.func_20088_a(this);
	handler->func_20088_a(this);
}

int Packet103SetSlot::getPacketSize()
{
	// Java: variable size - 6 bytes + 3 for item if present
	// byte (1) + short (2) + short (2) [+ optional: byte(1) + short(2)]
	return (myItemStack != nullptr) ? 9 : 6;
}

int Packet103SetSlot::getPacketId() const
{
	return 103;
}
