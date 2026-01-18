#include "network/Packet60.h"
#include "network/NetHandler.h"

Packet60::Packet60()
	: field_12236_a(0.0)
	, field_12235_b(0.0)
	, field_12239_c(0.0)
	, field_12238_d(0.0f)
{
}

void Packet60::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.field_12236_a = var1.readDouble();
	this->field_12236_a = in.readDouble();
	
	// this.field_12235_b = var1.readDouble();
	this->field_12235_b = in.readDouble();
	
	// this.field_12239_c = var1.readDouble();
	this->field_12239_c = in.readDouble();
	
	// this.field_12238_d = var1.readFloat();
	this->field_12238_d = in.readFloat();
	
	// int var2 = var1.readInt();
	int_t recordCount = in.readInt();
	
	// this.field_12237_e = new HashSet();
	this->field_12237_e.clear();
	
	// int var3 = (int)this.field_12236_a;
	// int var4 = (int)this.field_12235_b;
	// int var5 = (int)this.field_12239_c;
	int_t baseX = static_cast<int_t>(this->field_12236_a);
	int_t baseY = static_cast<int_t>(this->field_12235_b);
	int_t baseZ = static_cast<int_t>(this->field_12239_c);
	
	// for(int var6 = 0; var6 < var2; ++var6) {
	//     int var7 = var1.readByte() + var3;
	//     int var8 = var1.readByte() + var4;
	//     int var9 = var1.readByte() + var5;
	//     this.field_12237_e.add(new ChunkPosition(var7, var8, var9));
	// }
	for (int_t i = 0; i < recordCount; ++i)
	{
		// Java: int var7 = var1.readByte() + var3;
		// Java byte is signed (-128 to 127), so we need to convert properly
		byte_t byteX = in.readByte();
		byte_t byteY = in.readByte();
		byte_t byteZ = in.readByte();
		
		// Convert signed byte to signed int (Java automatically promotes)
		int_t offsetX = static_cast<int_t>(static_cast<int8_t>(byteX));
		int_t offsetY = static_cast<int_t>(static_cast<int8_t>(byteY));
		int_t offsetZ = static_cast<int_t>(static_cast<int8_t>(byteZ));
		
		int_t x = baseX + offsetX;
		int_t y = baseY + offsetY;
		int_t z = baseZ + offsetZ;
		
		this->field_12237_e.insert(ChunkCoordinates(x, y, z));
	}
}

void Packet60::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeDouble(this.field_12236_a);
	out.writeDouble(this->field_12236_a);
	
	// var1.writeDouble(this.field_12235_b);
	out.writeDouble(this->field_12235_b);
	
	// var1.writeDouble(this.field_12239_c);
	out.writeDouble(this->field_12239_c);
	
	// var1.writeFloat(this.field_12238_d);
	out.writeFloat(this->field_12238_d);
	
	// var1.writeInt(this.field_12237_e.size());
	out.writeInt(static_cast<int_t>(this->field_12237_e.size()));
	
	// int var2 = (int)this.field_12236_a;
	// int var3 = (int)this.field_12235_b;
	// int var4 = (int)this.field_12239_c;
	int_t baseX = static_cast<int_t>(this->field_12236_a);
	int_t baseY = static_cast<int_t>(this->field_12235_b);
	int_t baseZ = static_cast<int_t>(this->field_12239_c);
	
	// Iterator var5 = this.field_12237_e.iterator();
	// while(var5.hasNext()) {
	//     ChunkPosition var6 = (ChunkPosition)var5.next();
	//     int var7 = var6.x - var2;
	//     int var8 = var6.y - var3;
	//     int var9 = var6.z - var4;
	//     var1.writeByte(var7);
	//     var1.writeByte(var8);
	//     var1.writeByte(var9);
	// }
	for (const auto& pos : this->field_12237_e)
	{
		int_t offsetX = pos.x - baseX;
		int_t offsetY = pos.y - baseY;
		int_t offsetZ = pos.z - baseZ;
		
		out.writeByte(static_cast<byte_t>(offsetX));
		out.writeByte(static_cast<byte_t>(offsetY));
		out.writeByte(static_cast<byte_t>(offsetZ));
	}
}

void Packet60::processPacket(NetHandler* handler)
{
	// Java: var1.func_12245_a(this);
	handler->func_12245_a(this);
}

int Packet60::getPacketSize()
{
	// Java: variable size
	// double (8) + double (8) + double (8) + float (4) + int (4) + records (3 * count)
	return 32 + static_cast<int>(field_12237_e.size() * 3);
}

int Packet60::getPacketId() const
{
	return 60;
}
