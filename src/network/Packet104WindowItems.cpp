#include "network/Packet104WindowItems.h"
#include "network/NetHandler.h"

Packet104WindowItems::Packet104WindowItems()
	: windowId(0)
{
}

void Packet104WindowItems::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.windowId = var1.readByte();
	this->windowId = static_cast<int_t>(in.read() & 0xFF);
	
	// short var2 = var1.readShort();
	short_t count = in.readShort();
	
	// this.itemStack = new ItemStack[var2];
	this->itemStack.clear();
	this->itemStack.reserve(count);
	
	// for(int var3 = 0; var3 < var2; ++var3) {
	for (int_t i = 0; i < count; ++i)
	{
		// short var4 = var1.readShort();
		short_t itemIdShort = in.readShort();
		
		// if(var4 >= 0) {
		if (itemIdShort >= 0)
		{
			// byte var5 = var1.readByte();
			byte_t count = in.readByte();
			
			// short var6 = var1.readShort();
			short_t damage = in.readShort();
			
			// this.itemStack[var3] = new ItemStack(var4, var5, var6);
			this->itemStack.push_back(std::make_shared<ItemStack>(itemIdShort, count, damage));
		}
		else
		{
			// this.itemStack[var3] = null;
			this->itemStack.push_back(nullptr);
		}
	}
}

void Packet104WindowItems::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.windowId);
	out.writeByte(static_cast<byte_t>(this->windowId));
	
	// var1.writeShort(this.itemStack.length);
	out.writeShort(static_cast<short_t>(this->itemStack.size()));
	
	// for(int var2 = 0; var2 < this.itemStack.length; ++var2) {
	for (const auto& stack : this->itemStack)
	{
		// if(this.itemStack[var2] == null) {
		if (stack == nullptr)
		{
			// var1.writeShort(-1);
			out.writeShort(-1);
		}
		else
		{
			// var1.writeShort((short)this.itemStack[var2].itemID);
			out.writeShort(static_cast<short_t>(stack->itemID));
			
			// var1.writeByte((byte)this.itemStack[var2].stackSize);
			out.writeByte(static_cast<byte_t>(stack->stackSize));
			
			// var1.writeShort((short)this.itemStack[var2].itemDamage);
			out.writeShort(static_cast<short_t>(stack->itemDamage));
		}
	}
}

void Packet104WindowItems::processPacket(NetHandler* handler)
{
	// Java: var1.func_20094_a(this);
	handler->func_20094_a(this);
}

int Packet104WindowItems::getPacketSize()
{
	// Java: variable size
	// byte (1) + short (2) + items (2 per item + 3 if has item)
	int size = 3;
	for (const auto& stack : itemStack)
	{
		size += 2;  // short for item ID
		if (stack != nullptr)
		{
			size += 3;  // byte + short for count + damage
		}
	}
	return size;
}

int Packet104WindowItems::getPacketId() const
{
	return 104;
}
