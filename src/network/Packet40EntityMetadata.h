#pragma once

#include "network/Packet.h"
#include "world/entity/WatchableObject.h"
#include <vector>
#include <memory>

// Packet40EntityMetadata - matches Java Packet40EntityMetadata.java exactly
// Entity Metadata packet - server to client (0x28)
class Packet40EntityMetadata : public Packet {
public:
	int_t entityId;
	std::vector<std::shared_ptr<WatchableObject>> field_21048_b;  // Metadata list
	
	Packet40EntityMetadata();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
	
private:
	// Helper to read metadata from SocketInputStream (matches DataWatcher logic)
	std::vector<std::shared_ptr<WatchableObject>> readWatchableObjectsFromSocket(SocketInputStream& in);
};
