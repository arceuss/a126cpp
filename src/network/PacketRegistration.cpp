#include "network/Packet.h"
#include "network/Packet0KeepAlive.h"
#include "network/Packet1Login.h"
#include "network/Packet2Handshake.h"
#include "network/Packet3Chat.h"
#include "network/Packet4UpdateTime.h"
#include "network/Packet5PlayerInventory.h"
#include "network/Packet6SpawnPosition.h"
#include "network/Packet8.h"
#include "network/Packet9.h"
#include "network/Packet10Flying.h"
#include "network/Packet11PlayerPosition.h"
#include "network/Packet12PlayerLook.h"
#include "network/Packet13PlayerLookMove.h"
#include "network/Packet14BlockDig.h"
#include "network/Packet15Place.h"
#include "network/Packet20NamedEntitySpawn.h"
#include "network/Packet21PickupSpawn.h"
#include "network/Packet28.h"
#include "network/Packet29DestroyEntity.h"
#include "network/Packet30Entity.h"
#include "network/Packet34EntityTeleport.h"
#include "network/Packet50PreChunk.h"
#include "network/Packet51MapChunk.h"
#include "network/Packet52MultiBlockChange.h"
#include "network/Packet53BlockChange.h"
#include "network/Packet7.h"
#include "network/Packet16BlockItemSwitch.h"
#include "network/Packet17Sleep.h"
#include "network/Packet18ArmAnimation.h"
#include "network/Packet19EntityAction.h"
#include "network/Packet22Collect.h"
#include "network/Packet23VehicleSpawn.h"
#include "network/Packet24MobSpawn.h"
#include "network/Packet25EntityPainting.h"
#include "network/Packet27Position.h"
#include "network/Packet31RelEntityMove.h"
#include "network/Packet32EntityLook.h"
#include "network/Packet33RelEntityMoveLook.h"
#include "network/Packet38.h"
#include "network/Packet39.h"
#include "network/Packet40EntityMetadata.h"
#include "network/Packet54PlayNoteBlock.h"
#include "network/Packet60.h"
#include "network/Packet61DoorChange.h"
#include "network/Packet62Sound.h"
#include "network/Packet63Digging.h"
#include "network/Packet70Bed.h"
#include "network/Packet71Weather.h"
#include "network/Packet100OpenWindow.h"
#include "network/Packet101CloseWindow.h"
#include "network/Packet102WindowClick.h"
#include "network/Packet103SetSlot.h"
#include "network/Packet104WindowItems.h"
#include "network/Packet105UpdateProgressbar.h"
#include "network/Packet106Transaction.h"
#include "network/Packet130UpdateSign.h"
#include "network/Packet131MapData.h"
#include "network/Packet200Statistic.h"
#include "network/Packet255KickDisconnect.h"

// Static initialization function - call this to register all packets
// Matches Java static initializer block
void Packet::initializePacketRegistry()
{
	// Java: addIdClassMapping(id, isClientPacket, isServerPacket, class)
	// Match Java Packet.java static block exactly
	// This function is called once at program startup to register all packet types
	
	// Packet0KeepAlive: client=true, server=true
	Packet::addIdClassMapping(0, true, true, []() -> Packet* { return new Packet0KeepAlive(); });
	Packet::packetClassToId[std::type_index(typeid(Packet0KeepAlive))] = 0;
	
	// Packet1Login: client=true, server=true
	Packet::addIdClassMapping(1, true, true, []() -> Packet* { return new Packet1Login(); });
	Packet::packetClassToId[std::type_index(typeid(Packet1Login))] = 1;
	
	// Packet2Handshake: client=true, server=true
	Packet::addIdClassMapping(2, true, true, []() -> Packet* { return new Packet2Handshake(); });
	Packet::packetClassToId[std::type_index(typeid(Packet2Handshake))] = 2;
	
	// Packet3Chat: client=true, server=true
	Packet::addIdClassMapping(3, true, true, []() -> Packet* { return new Packet3Chat(); });
	Packet::packetClassToId[std::type_index(typeid(Packet3Chat))] = 3;
	
	// Packet4UpdateTime: client=true, server=false
	Packet::addIdClassMapping(4, true, false, []() -> Packet* { return new Packet4UpdateTime(); });
	Packet::packetClassToId[std::type_index(typeid(Packet4UpdateTime))] = 4;
	
	// Packet5PlayerInventory: client=true, server=false
	Packet::addIdClassMapping(5, true, false, []() -> Packet* { return new Packet5PlayerInventory(); });
	Packet::packetClassToId[std::type_index(typeid(Packet5PlayerInventory))] = 5;
	
	// Packet6SpawnPosition: client=true, server=false
	Packet::addIdClassMapping(6, true, false, []() -> Packet* { return new Packet6SpawnPosition(); });
	Packet::packetClassToId[std::type_index(typeid(Packet6SpawnPosition))] = 6;
	
	// Packet7: client=false, server=true
	Packet::addIdClassMapping(7, false, true, []() -> Packet* { return new Packet7(); });
	Packet::packetClassToId[std::type_index(typeid(Packet7))] = 7;
	
	// Packet8: client=true, server=false
	Packet::addIdClassMapping(8, true, false, []() -> Packet* { return new Packet8(); });
	Packet::packetClassToId[std::type_index(typeid(Packet8))] = 8;
	
	// Packet9: client=true, server=true
	Packet::addIdClassMapping(9, true, true, []() -> Packet* { return new Packet9(); });
	Packet::packetClassToId[std::type_index(typeid(Packet9))] = 9;
	
	// Packet10Flying: client=true, server=true
	Packet::addIdClassMapping(10, true, true, []() -> Packet* { return new Packet10Flying(); });
	Packet::packetClassToId[std::type_index(typeid(Packet10Flying))] = 10;
	
	// Packet11PlayerPosition: client=true, server=true
	Packet::addIdClassMapping(11, true, true, []() -> Packet* { return new Packet11PlayerPosition(); });
	Packet::packetClassToId[std::type_index(typeid(Packet11PlayerPosition))] = 11;
	
	// Packet12PlayerLook: client=true, server=true
	Packet::addIdClassMapping(12, true, true, []() -> Packet* { return new Packet12PlayerLook(); });
	Packet::packetClassToId[std::type_index(typeid(Packet12PlayerLook))] = 12;
	
	// Packet13PlayerLookMove: client=true, server=true
	Packet::addIdClassMapping(13, true, true, []() -> Packet* { return new Packet13PlayerLookMove(); });
	Packet::packetClassToId[std::type_index(typeid(Packet13PlayerLookMove))] = 13;
	
	// Packet14BlockDig: client=false, server=true
	Packet::addIdClassMapping(14, false, true, []() -> Packet* { return new Packet14BlockDig(); });
	Packet::packetClassToId[std::type_index(typeid(Packet14BlockDig))] = 14;
	
	// Packet15Place: client=false, server=true
	Packet::addIdClassMapping(15, false, true, []() -> Packet* { return new Packet15Place(); });
	Packet::packetClassToId[std::type_index(typeid(Packet15Place))] = 15;
	
	// Packet16BlockItemSwitch: client=false, server=true
	Packet::addIdClassMapping(16, false, true, []() -> Packet* { return new Packet16BlockItemSwitch(); });
	Packet::packetClassToId[std::type_index(typeid(Packet16BlockItemSwitch))] = 16;
	
	// Packet17Sleep: client=true, server=false
	Packet::addIdClassMapping(17, true, false, []() -> Packet* { return new Packet17Sleep(); });
	Packet::packetClassToId[std::type_index(typeid(Packet17Sleep))] = 17;
	
	// Packet18ArmAnimation: client=true, server=true
	Packet::addIdClassMapping(18, true, true, []() -> Packet* { return new Packet18ArmAnimation(); });
	Packet::packetClassToId[std::type_index(typeid(Packet18ArmAnimation))] = 18;
	
	// Packet19EntityAction: client=false, server=true
	Packet::addIdClassMapping(19, false, true, []() -> Packet* { return new Packet19EntityAction(); });
	Packet::packetClassToId[std::type_index(typeid(Packet19EntityAction))] = 19;
	
	// Packet20NamedEntitySpawn: client=true, server=false
	Packet::addIdClassMapping(20, true, false, []() -> Packet* { return new Packet20NamedEntitySpawn(); });
	Packet::packetClassToId[std::type_index(typeid(Packet20NamedEntitySpawn))] = 20;
	
	// Packet21PickupSpawn: client=true, server=false
	Packet::addIdClassMapping(21, true, false, []() -> Packet* { return new Packet21PickupSpawn(); });
	Packet::packetClassToId[std::type_index(typeid(Packet21PickupSpawn))] = 21;
	
	// Packet22Collect: client=true, server=false
	Packet::addIdClassMapping(22, true, false, []() -> Packet* { return new Packet22Collect(); });
	Packet::packetClassToId[std::type_index(typeid(Packet22Collect))] = 22;
	
	// Packet23VehicleSpawn: client=true, server=false
	Packet::addIdClassMapping(23, true, false, []() -> Packet* { return new Packet23VehicleSpawn(); });
	Packet::packetClassToId[std::type_index(typeid(Packet23VehicleSpawn))] = 23;
	
	// Packet24MobSpawn: client=true, server=false
	Packet::addIdClassMapping(24, true, false, []() -> Packet* { return new Packet24MobSpawn(); });
	Packet::packetClassToId[std::type_index(typeid(Packet24MobSpawn))] = 24;
	
	// Packet25EntityPainting: client=true, server=false
	Packet::addIdClassMapping(25, true, false, []() -> Packet* { return new Packet25EntityPainting(); });
	Packet::packetClassToId[std::type_index(typeid(Packet25EntityPainting))] = 25;
	
	// Packet27Position: client=true, server=true
	Packet::addIdClassMapping(27, true, true, []() -> Packet* { return new Packet27Position(); });
	Packet::packetClassToId[std::type_index(typeid(Packet27Position))] = 27;
	
	// Packet28: client=true, server=false
	Packet::addIdClassMapping(28, true, false, []() -> Packet* { return new Packet28(); });
	Packet::packetClassToId[std::type_index(typeid(Packet28))] = 28;
	
	// Packet29DestroyEntity: client=true, server=false
	Packet::addIdClassMapping(29, true, false, []() -> Packet* { return new Packet29DestroyEntity(); });
	Packet::packetClassToId[std::type_index(typeid(Packet29DestroyEntity))] = 29;
	
	// Packet30Entity: client=true, server=false
	Packet::addIdClassMapping(30, true, false, []() -> Packet* { return new Packet30Entity(); });
	Packet::packetClassToId[std::type_index(typeid(Packet30Entity))] = 30;
	
	// Packet31RelEntityMove: client=true, server=false
	Packet::addIdClassMapping(31, true, false, []() -> Packet* { return new Packet31RelEntityMove(); });
	Packet::packetClassToId[std::type_index(typeid(Packet31RelEntityMove))] = 31;
	
	// Packet32EntityLook: client=true, server=false
	Packet::addIdClassMapping(32, true, false, []() -> Packet* { return new Packet32EntityLook(); });
	Packet::packetClassToId[std::type_index(typeid(Packet32EntityLook))] = 32;
	
	// Packet33RelEntityMoveLook: client=true, server=false
	Packet::addIdClassMapping(33, true, false, []() -> Packet* { return new Packet33RelEntityMoveLook(); });
	Packet::packetClassToId[std::type_index(typeid(Packet33RelEntityMoveLook))] = 33;
	
	// Packet34EntityTeleport: client=true, server=false
	Packet::addIdClassMapping(34, true, false, []() -> Packet* { return new Packet34EntityTeleport(); });
	Packet::packetClassToId[std::type_index(typeid(Packet34EntityTeleport))] = 34;
	
	// Packet38: client=true, server=false
	Packet::addIdClassMapping(38, true, false, []() -> Packet* { return new Packet38(); });
	Packet::packetClassToId[std::type_index(typeid(Packet38))] = 38;
	
	// Packet39: client=true, server=false
	Packet::addIdClassMapping(39, true, false, []() -> Packet* { return new Packet39(); });
	Packet::packetClassToId[std::type_index(typeid(Packet39))] = 39;
	
	// Packet40EntityMetadata: client=true, server=false
	Packet::addIdClassMapping(40, true, false, []() -> Packet* { return new Packet40EntityMetadata(); });
	Packet::packetClassToId[std::type_index(typeid(Packet40EntityMetadata))] = 40;
	
	// Packet50PreChunk: client=true, server=false
	Packet::addIdClassMapping(50, true, false, []() -> Packet* { return new Packet50PreChunk(); });
	Packet::packetClassToId[std::type_index(typeid(Packet50PreChunk))] = 50;
	
	// Packet51MapChunk: client=true, server=false
	Packet::addIdClassMapping(51, true, false, []() -> Packet* { return new Packet51MapChunk(); });
	Packet::packetClassToId[std::type_index(typeid(Packet51MapChunk))] = 51;
	
	// Packet52MultiBlockChange: client=true, server=false
	Packet::addIdClassMapping(52, true, false, []() -> Packet* { return new Packet52MultiBlockChange(); });
	Packet::packetClassToId[std::type_index(typeid(Packet52MultiBlockChange))] = 52;
	
	// Packet53BlockChange: client=true, server=false
	Packet::addIdClassMapping(53, true, false, []() -> Packet* { return new Packet53BlockChange(); });
	Packet::packetClassToId[std::type_index(typeid(Packet53BlockChange))] = 53;
	
	// Packet54PlayNoteBlock: client=true, server=false
	Packet::addIdClassMapping(54, true, false, []() -> Packet* { return new Packet54PlayNoteBlock(); });
	Packet::packetClassToId[std::type_index(typeid(Packet54PlayNoteBlock))] = 54;
	
	// Packet60: client=true, server=false (Explosion)
	Packet::addIdClassMapping(60, true, false, []() -> Packet* { return new Packet60(); });
	Packet::packetClassToId[std::type_index(typeid(Packet60))] = 60;
	
	// Packet61DoorChange: client=true, server=false (Sound effect)
	Packet::addIdClassMapping(61, true, false, []() -> Packet* { return new Packet61DoorChange(); });
	Packet::packetClassToId[std::type_index(typeid(Packet61DoorChange))] = 61;
	
	// Packet62Sound: client=true, server=false
	Packet::addIdClassMapping(62, true, false, []() -> Packet* { return new Packet62Sound(); });
	Packet::packetClassToId[std::type_index(typeid(Packet62Sound))] = 62;
	
	// Packet63Digging: client=true, server=false
	Packet::addIdClassMapping(63, true, false, []() -> Packet* { return new Packet63Digging(); });
	Packet::packetClassToId[std::type_index(typeid(Packet63Digging))] = 63;
	
	// Packet70Bed: client=true, server=false
	Packet::addIdClassMapping(70, true, false, []() -> Packet* { return new Packet70Bed(); });
	Packet::packetClassToId[std::type_index(typeid(Packet70Bed))] = 70;
	
	// Packet71Weather: client=true, server=false
	Packet::addIdClassMapping(71, true, false, []() -> Packet* { return new Packet71Weather(); });
	Packet::packetClassToId[std::type_index(typeid(Packet71Weather))] = 71;
	
	// Packet100OpenWindow: client=true, server=false
	Packet::addIdClassMapping(100, true, false, []() -> Packet* { return new Packet100OpenWindow(); });
	Packet::packetClassToId[std::type_index(typeid(Packet100OpenWindow))] = 100;
	
	// Packet101CloseWindow: client=true, server=true
	Packet::addIdClassMapping(101, true, true, []() -> Packet* { return new Packet101CloseWindow(); });
	Packet::packetClassToId[std::type_index(typeid(Packet101CloseWindow))] = 101;
	
	// Packet102WindowClick: client=false, server=true
	Packet::addIdClassMapping(102, false, true, []() -> Packet* { return new Packet102WindowClick(); });
	Packet::packetClassToId[std::type_index(typeid(Packet102WindowClick))] = 102;
	
	// Packet103SetSlot: client=true, server=false
	Packet::addIdClassMapping(103, true, false, []() -> Packet* { return new Packet103SetSlot(); });
	Packet::packetClassToId[std::type_index(typeid(Packet103SetSlot))] = 103;
	
	// Packet104WindowItems: client=true, server=false
	Packet::addIdClassMapping(104, true, false, []() -> Packet* { return new Packet104WindowItems(); });
	Packet::packetClassToId[std::type_index(typeid(Packet104WindowItems))] = 104;
	
	// Packet105UpdateProgressbar: client=true, server=false
	Packet::addIdClassMapping(105, true, false, []() -> Packet* { return new Packet105UpdateProgressbar(); });
	Packet::packetClassToId[std::type_index(typeid(Packet105UpdateProgressbar))] = 105;
	
	// Packet106Transaction: client=true, server=true
	Packet::addIdClassMapping(106, true, true, []() -> Packet* { return new Packet106Transaction(); });
	Packet::packetClassToId[std::type_index(typeid(Packet106Transaction))] = 106;
	
	// Packet130UpdateSign: client=true, server=true
	Packet::addIdClassMapping(130, true, true, []() -> Packet* { return new Packet130UpdateSign(); });
	Packet::packetClassToId[std::type_index(typeid(Packet130UpdateSign))] = 130;
	
	// Packet131MapData: client=true, server=false
	Packet::addIdClassMapping(131, true, false, []() -> Packet* { return new Packet131MapData(); });
	Packet::packetClassToId[std::type_index(typeid(Packet131MapData))] = 131;
	
	// Packet200Statistic: client=true, server=false
	Packet::addIdClassMapping(200, true, false, []() -> Packet* { return new Packet200Statistic(); });
	Packet::packetClassToId[std::type_index(typeid(Packet200Statistic))] = 200;
	
	// Packet255KickDisconnect: client=true, server=true
	Packet::addIdClassMapping(255, true, true, []() -> Packet* { return new Packet255KickDisconnect(); });
	Packet::packetClassToId[std::type_index(typeid(Packet255KickDisconnect))] = 255;
	
	// TODO: Add remaining packets as they are created
	// Note: This function should be called once at program startup, before any networking code
}

// Auto-initialize on first use (called from Packet::getNewPacket if registry is empty)
// This matches Java's static initializer behavior
static bool packetRegistryInitialized = false;

void ensurePacketRegistryInitialized()
{
	if (!packetRegistryInitialized)
	{
		Packet::initializePacketRegistry();
		packetRegistryInitialized = true;
	}
}
