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
	xPosition = in.readInt();
	yPosition = in.readShort();
	zPosition = in.readInt();
	xSize = in.read() + 1;
	ySize = in.read() + 1;
	zSize = in.read() + 1;

	const int_t compressedSize = in.readInt();
	std::vector<byte_t> compressedData(static_cast<size_t>(compressedSize));
	in.readFully(compressedData.data(), compressedData.size());

	const int_t decompressedSize = xSize * ySize * zSize * 5 / 2;
	chunk.assign(static_cast<size_t>(decompressedSize), 0);

	z_stream stream{};
	if (inflateInit(&stream) != Z_OK)
		throw std::runtime_error("Failed to initialize zlib inflater");

	stream.next_in = reinterpret_cast<Bytef*>(compressedData.data());
	stream.avail_in = static_cast<uInt>(compressedData.size());
	stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
	stream.avail_out = static_cast<uInt>(chunk.size());

	int result = Z_OK;
	while (stream.avail_out != 0 && stream.avail_in != 0 && result == Z_OK)
		result = inflate(&stream, Z_NO_FLUSH);

	inflateEnd(&stream);
	if (result != Z_OK && result != Z_STREAM_END && result != Z_BUF_ERROR
		&& result != Z_NEED_DICT)
		throw std::runtime_error("Bad compressed data format");
}

void Packet51MapChunk::writePacketData(SocketOutputStream& out)
{
	out.writeInt(xPosition);
	out.writeShort(static_cast<short_t>(yPosition));
	out.writeInt(zPosition);
	out.write(xSize - 1);
	out.write(ySize - 1);
	out.write(zSize - 1);
	out.writeInt(chunkSize);
	if (chunkSize < 0 || static_cast<size_t>(chunkSize) > chunk.size())
		throw std::out_of_range("Invalid chunk range");
	out.write(chunk.data(), static_cast<size_t>(chunkSize));
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
