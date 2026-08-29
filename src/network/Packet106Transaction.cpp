#include "network/Packet106Transaction.h"
#include "network/NetHandler.h"

Packet106Transaction::Packet106Transaction()
	: windowId(0)
	, field_20028_b(0)
	, field_20030_c(false)
{
}

Packet106Transaction::Packet106Transaction(int_t windowId, short_t action, bool accepted)
	: windowId(windowId)
	, field_20028_b(action)
	, field_20030_c(accepted)
{
}

void Packet106Transaction::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.windowId = var1.readByte();
	this->windowId = static_cast<int_t>(in.readByte());
	
	// this.field_20028_b = var1.readShort();
	this->field_20028_b = in.readShort();
	
	// this.field_20030_c = var1.readByte() != 0;
	this->field_20030_c = in.readByte() != 0;
}

void Packet106Transaction::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeByte(this.windowId);
	out.writeByte(static_cast<byte_t>(this->windowId));
	
	// var1.writeShort(this.field_20028_b);
	out.writeShort(this->field_20028_b);
	
	// var1.writeByte(this.field_20030_c ? 1 : 0);
	out.writeByte(static_cast<byte_t>(this->field_20030_c ? 1 : 0));
}

void Packet106Transaction::processPacket(NetHandler* handler)
{
	// Java: var1.func_20089_a(this);
	handler->func_20089_a(this);
}

int Packet106Transaction::getPacketSize()
{
	// Java: return 4;
	// byte (1) + short (2) + byte (1) = 4
	return 4;
}

int Packet106Transaction::getPacketId() const
{
	return 106;
}
