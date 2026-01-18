#pragma once

class Packet;
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

#include "java/String.h"
#include <vector>

// Base class for packet handlers - matches Java NetHandler exactly
class NetHandler {
public:
	virtual ~NetHandler() = default;
	
	virtual bool isServerHandler() = 0;
	
	// Default handlers (matching Java NetHandler.java)
	virtual void handleMapChunk(Packet51MapChunk* var1);
	virtual void func_4114_b(Packet* var1);  // Default handler for unrecognized packets
	virtual void handleErrorMessage(const jstring& var1, const std::vector<jstring>& var2);
	virtual void handleKickDisconnect(Packet255KickDisconnect* var1);
	
	virtual void handleLogin(Packet1Login* var1);
	virtual void handleFlying(Packet10Flying* var1);
	virtual void handleMultiBlockChange(Packet52MultiBlockChange* var1);
	virtual void handleBlockDig(Packet14BlockDig* var1);
	virtual void handleBlockChange(Packet53BlockChange* var1);
	virtual void handlePreChunk(Packet50PreChunk* var1);
	virtual void handleNamedEntitySpawn(Packet20NamedEntitySpawn* var1);
	virtual void handleEntity(Packet30Entity* var1);
	virtual void handleEntityTeleport(Packet34EntityTeleport* var1);
	virtual void handlePlace(Packet15Place* var1);
	virtual void handleBlockItemSwitch(Packet16BlockItemSwitch* var1);
	virtual void handleDestroyEntity(Packet29DestroyEntity* var1);
	virtual void handlePickupSpawn(Packet21PickupSpawn* var1);
	virtual void handleCollect(Packet22Collect* var1);
	virtual void handleChat(Packet3Chat* var1);
	virtual void handleAddToInventory(Packet17Sleep* var1);
	virtual void handleVehicleSpawn(Packet23VehicleSpawn* var1);
	virtual void handleArmAnimation(Packet18ArmAnimation* var1);
	virtual void handleEntityAction(Packet19EntityAction* var1);
	virtual void handleHandshake(Packet2Handshake* var1);
	virtual void handleMobSpawn(Packet24MobSpawn* var1);
	virtual void handleUpdateTime(Packet4UpdateTime* var1);
	virtual void handlePlayerInventory(Packet5PlayerInventory* var1);
	virtual void handleSpawnPosition(Packet6SpawnPosition* var1);
	virtual void func_6498_a(Packet28* var1);  // handleEntityVelocity
	virtual void handleEntityMetadata(Packet40EntityMetadata* var1);
	virtual void func_6497_a(Packet39* var1);  // handleEntityAttach
	virtual void func_6499_a(Packet7* var1);
	virtual void func_9447_a(Packet38* var1);  // handleEntityAttach2
	virtual void handleHealth(Packet8* var1);
	virtual void func_9448_a(Packet9* var1);  // handleRespawn
	virtual void func_12245_a(Packet60* var1);  // handleExplosion
	virtual void func_20087_a(Packet100OpenWindow* var1);
	virtual void func_20092_a(Packet101CloseWindow* var1);
	virtual void func_20091_a(Packet102WindowClick* var1);
	virtual void func_20088_a(Packet103SetSlot* var1);
	virtual void func_20094_a(Packet104WindowItems* var1);
	virtual void handleSignUpdate(Packet130UpdateSign* var1);
	virtual void func_20090_a(Packet105UpdateProgressbar* var1);
	virtual void func_20089_a(Packet106Transaction* var1);
	virtual void handlePainting(Packet25EntityPainting* var1);
	virtual void func_22185_a(Packet27Position* var1);
	virtual void func_28115_a(Packet61DoorChange* var1);
	virtual void handle62Sound(Packet62Sound* var1);
	virtual void handle63Digging(Packet63Digging* var1);
	virtual void handleBed(Packet70Bed* var1);
	virtual void handleWeather(Packet71Weather* var1);
	virtual void handleMap(Packet131MapData* var1);
	virtual void handleStatistic(Packet200Statistic* var1);
	virtual void handleNoteBlock(Packet54PlayNoteBlock* var1);
};
