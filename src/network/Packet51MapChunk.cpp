#include "network/Packet51MapChunk.h"
#include "network/NetHandler.h"
#include <zlib.h>
#include <stdexcept>
#include <cstddef>

Packet51MapChunk::Packet51MapChunk()
	: xPosition(0)
	, yPosition(0)
	, zPosition(0)
	, xSize(0)
	, ySize(0)
	, zSize(0)
	, chunkSize(0)
{
	// Java: this.isChunkDataPacket = true;
	this->isChunkDataPacket = true;
}

void Packet51MapChunk::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER - CRITICAL for byte sequence
	// this.xPosition = var1.readInt();
	this->xPosition = in.readInt();
	
	// this.yPosition = var1.readShort();
	this->yPosition = in.readShort();
	
	// this.zPosition = var1.readInt();
	this->zPosition = in.readInt();
	
	// this.xSize = var1.read() + 1;
	// Java: read() returns unsigned byte (0-255) as int, then we add 1
	this->xSize = (in.read() & 0xFF) + 1;
	
	// this.ySize = var1.read() + 1;
	this->ySize = (in.read() & 0xFF) + 1;
	
	// this.zSize = var1.read() + 1;
	this->zSize = (in.read() & 0xFF) + 1;
	
	// int var2 = var1.readInt();
	int_t compressedSize = in.readInt();
	
	// Validate compressed size to prevent excessive memory allocation
	if (compressedSize < 0 || compressedSize > 2097152)  // Max 2MB compressed size
	{
		throw std::runtime_error("Invalid compressed chunk size: " + std::to_string(compressedSize));
	}
	
	// byte[] var3 = new byte[var2];
	std::vector<byte_t> compressedData(compressedSize);
	
	// var1.readFully(var3);
	in.readFully(compressedData.data(), compressedSize);
	
	// this.chunk = new byte[this.xSize * this.ySize * this.zSize * 5 / 2];
	int_t decompressedSize = this->xSize * this->ySize * this->zSize * 5 / 2;
	
	// Validate decompressed size to prevent excessive memory allocation
	if (decompressedSize < 0 || decompressedSize > 8388608)  // Max 8MB decompressed size
	{
		throw std::runtime_error("Invalid decompressed chunk size: " + std::to_string(decompressedSize) + 
		                         " (xSize=" + std::to_string(this->xSize) + 
		                         ", ySize=" + std::to_string(this->ySize) + 
		                         ", zSize=" + std::to_string(this->zSize) + ")");
	}
	
	this->chunk.resize(decompressedSize);
	
	// Inflater var4 = new Inflater();
	z_stream zs;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;
	zs.avail_in = 0;
	zs.next_in = Z_NULL;
	
	// var4.setInput(var3);
	int ret = inflateInit(&zs);
	if (ret != Z_OK)
	{
		throw std::runtime_error("Failed to initialize zlib inflater");
	}
	
	zs.avail_in = static_cast<uInt>(compressedSize);
	zs.next_in = reinterpret_cast<Bytef*>(compressedData.data());
	zs.avail_out = static_cast<uInt>(decompressedSize);
	zs.next_out = reinterpret_cast<Bytef*>(this->chunk.data());
	
	// try {
	//     var4.inflate(this.chunk);
	// } catch (DataFormatException var9) {
	//     throw new IOException("Bad compressed data format");
	// } finally {
	//     var4.end();
	// }
	// Java's Inflater.inflate(byte[]) will keep inflating until either the output buffer
	// is filled or the stream ends. Do the same here without requiring a single Z_FINISH.
	ret = Z_OK;
	while (zs.total_out < static_cast<uLong>(decompressedSize))
	{
		ret = inflate(&zs, Z_NO_FLUSH);
		if (ret == Z_STREAM_END)
		{
			break;
		}
		if (ret != Z_OK && ret != Z_BUF_ERROR)
		{
			inflateEnd(&zs);
			throw std::runtime_error("Bad compressed data format");
		}
		// If zlib reports it needs more input/output progress but we have no more input,
		// stop and validate below.
		if (ret == Z_BUF_ERROR && zs.avail_in == 0)
		{
			break;
		}
	}

	// Validate decompressed length matches what the packet declared.
	if (zs.total_out != static_cast<uLong>(decompressedSize))
	{
		inflateEnd(&zs);
		throw std::runtime_error("Bad compressed data length");
	}
	
	inflateEnd(&zs);
	
	// Store compressed size for writing (this is the size of compressed data we received)
	this->chunkSize = compressedSize;
}

void Packet51MapChunk::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER - CRITICAL for byte sequence
	// var1.writeInt(this.xPosition);
	out.writeInt(this->xPosition);
	
	// var1.writeShort(this.yPosition);
	out.writeShort(this->yPosition);
	
	// var1.writeInt(this.zPosition);
	out.writeInt(this->zPosition);
	
	// var1.write(this.xSize - 1);
	// Java: write(byte) - write lower 8 bits
	out.writeByte(static_cast<byte_t>(this->xSize - 1));
	
	// var1.write(this.ySize - 1);
	out.writeByte(static_cast<byte_t>(this->ySize - 1));
	
	// var1.write(this.zSize - 1);
	out.writeByte(static_cast<byte_t>(this->zSize - 1));
	
	// Java: var1.writeInt(this.chunkSize);
	// Note: chunkSize must be set before calling writePacketData
	// If chunk contains decompressed data, we need to compress it first
	// For now, assume chunkSize is already set correctly
	out.writeInt(this->chunkSize);
	
	// Java: var1.write(this.chunk, 0, this.chunkSize);
	// Write compressed chunk data
	if (this->chunkSize > 0 && this->chunkSize <= static_cast<int_t>(this->chunk.size()))
	{
		out.write(this->chunk.data(), this->chunkSize);
	}
}

void Packet51MapChunk::processPacket(NetHandler* handler)
{
	// Java: var1.handleMapChunk(this);
	handler->handleMapChunk(this);
}

int Packet51MapChunk::getPacketSize()
{
	// Java: return 17 + this.chunkSize;
	// int (4) + short (2) + int (4) + byte (1) + byte (1) + byte (1) + int (4) = 17
	return 17 + static_cast<int>(this->chunkSize);
}

int Packet51MapChunk::getPacketId() const
{
	return 51;
}
