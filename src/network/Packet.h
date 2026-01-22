#pragma once

#include "network/SocketStreams.h"
#include "network/NetHandler.h"
#include "java/Type.h"
#include "java/String.h"
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <typeindex>

class NetHandler;

// Forward declarations for all packet types
class Packet0KeepAlive;
class Packet1Login;
class Packet2Handshake;
class Packet3Chat;
class Packet4UpdateTime;
class Packet5PlayerInventory;
class Packet6SpawnPosition;
class Packet7;
class Packet8;
class Packet9;
class Packet10Flying;
class Packet11PlayerPosition;
class Packet12PlayerLook;
class Packet13PlayerLookMove;
class Packet14BlockDig;
class Packet15Place;
class Packet16BlockItemSwitch;
class Packet17Sleep;
class Packet18ArmAnimation;
class Packet19EntityAction;
class Packet20NamedEntitySpawn;
class Packet21PickupSpawn;
class Packet22Collect;
class Packet23VehicleSpawn;
class Packet24MobSpawn;
class Packet25EntityPainting;
class Packet27Position;
class Packet28;
class Packet29DestroyEntity;
class Packet30Entity;
class Packet31RelEntityMove;
class Packet32EntityLook;
class Packet33RelEntityMoveLook;
class Packet34EntityTeleport;
class Packet38;
class Packet39;
class Packet40EntityMetadata;
class Packet50PreChunk;
class Packet51MapChunk;
class Packet52MultiBlockChange;
class Packet53BlockChange;
class Packet54PlayNoteBlock;
class Packet60;
class Packet61DoorChange;
class Packet62Sound;
class Packet63Digging;
class Packet70Bed;
class Packet71Weather;
class Packet100OpenWindow;
class Packet101CloseWindow;
class Packet102WindowClick;
class Packet103SetSlot;
class Packet104WindowItems;
class Packet105UpdateProgressbar;
class Packet106Transaction;
class Packet130UpdateSign;
class Packet131MapData;
class Packet200Statistic;
class Packet255KickDisconnect;

// Base packet class - matches Java Packet.java exactly
class Packet {
public:
	// Static packet registry
	static std::map<int, std::function<Packet*()>> packetIdToFactory;
	static std::map<std::type_index, int> packetClassToId;
	static std::set<int> clientPacketIdList;
	static std::set<int> serverPacketIdList;
	
	// Instance members matching Java
	const long_t creationTimeMillis;
	bool isChunkDataPacket;
	
	// Static methods matching Java
	static void addIdClassMapping(int packetId, bool isClientPacket, bool isServerPacket, 
	                              std::function<Packet*()> factory);
	
	static Packet* getNewPacket(int packetId);
	static int getPacketIdForType(const std::type_info& type);
	
	static std::unique_ptr<Packet> readPacket(SocketInputStream& in, bool isServerHandler);
	static void writePacket(Packet* packet, SocketOutputStream& out);
	
	// String helpers matching Java Packet.readString/writeString
	static jstring readString(SocketInputStream& in, int maxLength);
	static void writeString(const jstring& str, SocketOutputStream& out, int maxLength = 32767);
	
	// Overloads for std::ostream/std::istream (used by DataWatcher)
	// Note: These use UTF-16 like Packet::writeString, not Modified UTF-8
	static jstring readString(std::istream& in, int maxLength);
	static void writeString(const jstring& str, std::ostream& out, int maxLength = 32767);
	
	// Abstract methods - must be implemented by each packet
	virtual void readPacketData(SocketInputStream& in) = 0;
	virtual void writePacketData(SocketOutputStream& out) = 0;
	virtual void processPacket(NetHandler* handler) = 0;
	virtual int getPacketSize() = 0;
	
	// Virtual destructor
	virtual ~Packet() = default;
	
protected:
	// Helper to get packet ID from type
	template<typename T>
	static int getPacketId() {
		return getPacketIdForType(typeid(T));
	}

public:
	// Constructor
	Packet();
	
	// Get packet ID for this instance
	virtual int getPacketId() const = 0;
	
	// Initialize packet registry - must be called before using packets
	static void initializePacketRegistry();
};
