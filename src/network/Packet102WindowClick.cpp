#include "network/Packet102WindowClick.h"
#include "network/NetHandler.h"
#include <memory>

Packet102WindowClick::Packet102WindowClick()
	: window_Id(0)
	, inventorySlot(0)
	, mouseClick(0)
	, action(0)
	, itemStack(nullptr)
	, field_27050_f(false)
{
}

Packet102WindowClick::Packet102WindowClick(int_t windowId, int_t inventorySlot, int_t mouseClick, bool shift, std::unique_ptr<ItemStack> itemStack, short_t action)
	: window_Id(windowId)
	, inventorySlot(inventorySlot)
	, mouseClick(mouseClick)
	, action(action)
	, itemStack(itemStack ? std::make_shared<ItemStack>(*itemStack) : nullptr)
	, field_27050_f(shift)
{
}

void Packet102WindowClick::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.window_Id = var1.readByte();
	this->window_Id = static_cast<int_t>(in.read() & 0xFF);
	
	// this.inventorySlot = var1.readShort();
	this->inventorySlot = in.readShort();
	
	// this.mouseClick = var1.readByte();
	this->mouseClick = static_cast<int_t>(in.read() & 0xFF);
	
	// this.action = var1.readShort();
	this->action = in.readShort();
	
	// this.field_27050_f = var1.readBoolean();
	this->field_27050_f = in.readBoolean();
	
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

void Packet102WindowClick::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.window_Id);
	out.writeByte(static_cast<byte_t>(this->window_Id));
	
	// var1.writeShort(this.inventorySlot);
	out.writeShort(static_cast<short_t>(this->inventorySlot));
	
	// var1.writeByte(this.mouseClick);
	out.writeByte(static_cast<byte_t>(this->mouseClick));
	
	// var1.writeShort(this.action);
	out.writeShort(this->action);
	
	// var1.writeBoolean(this.field_27050_f);
	out.writeBoolean(this->field_27050_f);
	
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
		
		// var1.writeShort(this.itemStack.itemDamage);
		out.writeShort(static_cast<short_t>(this->itemStack->itemDamage));
	}
}

void Packet102WindowClick::processPacket(NetHandler* handler)
{
	// Java: var1.func_20091_a(this);
	handler->func_20091_a(this);
}

int Packet102WindowClick::getPacketSize()
{
	// Java: variable size - 10 bytes + 3 for item if present
	// byte (1) + short (2) + byte (1) + short (2) + boolean (1) + short (2) [+ optional: byte(1) + short(2)]
	return (itemStack != nullptr) ? 13 : 10;
}

int Packet102WindowClick::getPacketId() const
{
	return 102;
}
