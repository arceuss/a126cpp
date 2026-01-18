#pragma once

#include "network/Packet.h"
#include "world/entity/WatchableObject.h"
#include <vector>
#include <memory>

// Packet24MobSpawn - matches Java Packet24MobSpawn.java exactly
// Mob Spawn packet - server to client
class Packet24MobSpawn : public Packet {
public:
	int_t entityId;
	byte_t type;
	int_t xPosition;
	int_t yPosition;
	int_t zPosition;
	byte_t yaw;
	byte_t pitch;
	std::vector<std::shared_ptr<WatchableObject>> receivedMetadata;  // Stores metadata read from packet
	
	Packet24MobSpawn();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
	
private:
	// Helper to read metadata from SocketInputStream (matches DataWatcher logic)
	std::vector<std::shared_ptr<WatchableObject>> readWatchableObjectsFromSocket(SocketInputStream& in);
};
