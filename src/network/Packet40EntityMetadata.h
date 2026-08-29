#pragma once

#include "network/Packet.h"
#include "world/entity/WatchableObject.h"
#include <vector>
#include <memory>

class Packet40EntityMetadata : public Packet {
public:
	int_t entityId;
	Packet40EntityMetadata();
	
	void readPacketData(SocketInputStream& in) override;
	void writePacketData(SocketOutputStream& out) override;
	void processPacket(NetHandler* handler) override;
	int getPacketSize() override;
	int getPacketId() const override;
	std::vector<std::shared_ptr<WatchableObject>>& func_21047_b();
	
private:
	std::vector<std::shared_ptr<WatchableObject>> field_21048_b;
	std::vector<std::shared_ptr<WatchableObject>> readWatchableObjectsFromSocket(SocketInputStream& in);
};
