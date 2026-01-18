#pragma once

#include "network/Packet.h"
#include <vector>

// Packet51MapChunk - matches Java Packet51MapChunk.java exactly
// CRITICAL for chunk loading - byte sequence must be 1:1
class Packet51MapChunk : public Packet {
public:
	int_t xPosition;
	short_t yPosition;  // Note: Java uses short for yPosition
	int_t zPosition;
	int_t xSize;  // Java: int, not byte (read as byte then +1, stored as int)
	int_t ySize;  // Java: int, not byte (read as byte then +1, stored as int)
	int_t zSize;  // Java: int, not byte (read as byte then +1, stored as int)
	std::vector<byte_t> chunk;  // Decompressed chunk data
	int_t chunkSize;  // Compressed chunk size
	
	Packet51MapChunk();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
