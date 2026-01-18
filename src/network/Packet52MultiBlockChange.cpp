#include "network/Packet52MultiBlockChange.h"
#include "network/NetHandler.h"

Packet52MultiBlockChange::Packet52MultiBlockChange()
	: xPosition(0)
	, zPosition(0)
	, size(0)
{
	// Java: this.isChunkDataPacket = true;
	this->isChunkDataPacket = true;
}

void Packet52MultiBlockChange::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.size = var1.readShort() & '\uffff';
	// Java: '\uffff' is 0xFFFF, which masks to get unsigned short value
	short_t sizeShort = in.readShort();
	this->size = static_cast<int_t>(sizeShort) & 0xFFFF;
	
	// this.coordinateArray = new short[this.size];
	this->coordinateArray.resize(this->size);
	
	// this.typeArray = new byte[this.size];
	this->typeArray.resize(this->size);
	
	// this.metadataArray = new byte[this.size];
	this->metadataArray.resize(this->size);
	
	// for(int var2 = 0; var2 < this.size; ++var2) {
	//     this.coordinateArray[var2] = var1.readShort();
	// }
	for (int_t i = 0; i < this->size; ++i)
	{
		this->coordinateArray[i] = in.readShort();
	}
	
	// var1.readFully(this.typeArray);
	in.readFully(this->typeArray.data(), this->size);
	
	// var1.readFully(this.metadataArray);
	in.readFully(this->metadataArray.data(), this->size);
}

void Packet52MultiBlockChange::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.writeShort((short)this.size);
	out.writeShort(static_cast<short_t>(this->size));
	
	// for(int var2 = 0; var2 < this.size; ++var2) {
	//     var1.writeShort(this.coordinateArray[var2]);
	// }
	for (int_t i = 0; i < this->size; ++i)
	{
		out.writeShort(this->coordinateArray[i]);
	}
	
	// var1.write(this.typeArray);
	out.write(this->typeArray.data(), this->size);
	
	// var1.write(this.metadataArray);
	out.write(this->metadataArray.data(), this->size);
}

void Packet52MultiBlockChange::processPacket(NetHandler* handler)
{
	// Java: var1.handleMultiBlockChange(this);
	handler->handleMultiBlockChange(this);
}

int Packet52MultiBlockChange::getPacketSize()
{
	// Java: return 10 + this.size * 4;
	// int (4) + int (4) + short (2) = 10, plus size * (short(2) + byte(1) + byte(1)) = size * 4
	return 10 + static_cast<int>(this->size * 4);
}

int Packet52MultiBlockChange::getPacketId() const
{
	return 52;
}
