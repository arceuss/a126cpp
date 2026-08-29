#pragma once

#include "network/Packet.h"
#include <vector>

// Packet51MapChunk - matches Java Packet51MapChunk.java exactly
// CRITICAL for chunk loading - byte sequence must be 1:1
class Packet51MapChunk : public Packet {
public:
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	int_t xSize;
	int_t ySize;
	int_t zSize;
	std::vector<byte_t> chunk;

private:
	int_t chunkSize;

public:
	Packet51MapChunk();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
};
