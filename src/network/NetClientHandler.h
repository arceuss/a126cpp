#pragma once

#include "network/NetHandler.h"
#include "network/NetworkManager.h"
#include "java/String.h"
#include "java/Random.h"
#include <memory>

// Forward declarations
class Minecraft;
class MultiPlayerLevel;
class Entity;
class Packet;

// NetClientHandler - matches Java NetClientHandler.java exactly
class NetClientHandler : public NetHandler {
private:
	bool disconnected;
	std::unique_ptr<NetworkManager> netManager;
public:
	jstring field_1209_a;  // serverName (public to match Java access pattern)
private:
	Minecraft* mc;
	MultiPlayerLevel* worldClient;
	bool field_1210_g;
	Random rand;

public:
	NetClientHandler(Minecraft* mc, const jstring& host, int port);
	
	void processReadPackets();
	
	// Override isServerHandler
	bool isServerHandler() override;
	
	// Packet handlers - override all from NetHandler, matching Java exactly
	void handleLogin(Packet1Login* var1) override;
	void handlePickupSpawn(Packet21PickupSpawn* var1) override;
	void handleVehicleSpawn(Packet23VehicleSpawn* var1) override;
	void handlePainting(Packet25EntityPainting* var1) override;
	void func_6498_a(Packet28* var1) override;  // handleEntityVelocity
	void handleEntityMetadata(Packet40EntityMetadata* var1) override;
	void handleNamedEntitySpawn(Packet20NamedEntitySpawn* var1) override;
	void handleEntityTeleport(Packet34EntityTeleport* var1) override;
	void handleEntity(Packet30Entity* var1) override;
	void handleDestroyEntity(Packet29DestroyEntity* var1) override;
	void handleFlying(Packet10Flying* var1) override;
	void handlePreChunk(Packet50PreChunk* var1) override;
	void handleMultiBlockChange(Packet52MultiBlockChange* var1) override;
	void handleMapChunk(Packet51MapChunk* var1) override;
	void handleBlockChange(Packet53BlockChange* var1) override;
	void handleKickDisconnect(Packet255KickDisconnect* var1) override;
	void handleErrorMessage(const jstring& var1, const std::vector<jstring>& var2) override;
	void func_4114_b(Packet* var1) override;  // Override default handler to respond to keep-alive
	void handleCollect(Packet22Collect* var1) override;
	void handleChat(Packet3Chat* var1) override;
	void handleArmAnimation(Packet18ArmAnimation* var1) override;
	void handleHandshake(Packet2Handshake* var1) override;
	void disconnect();
	void handleMobSpawn(Packet24MobSpawn* var1) override;
	void handleUpdateTime(Packet4UpdateTime* var1) override;
	void handlePlayerInventory(Packet5PlayerInventory* var1) override;
	void handleSpawnPosition(Packet6SpawnPosition* var1) override;
	void func_6497_a(Packet39* var1) override;  // handleEntityAttach (SetRidingPacket)
	void func_6499_a(Packet7* var1) override;  // handleUseEntity (InteractPacket) - client-to-server only, no-op on client
	void func_9447_a(Packet38* var1) override;  // handleEntityAttach2
	void handleHealth(Packet8* var1) override;
	void func_9448_a(Packet9* var1) override;  // handleRespawn
	void func_12245_a(Packet60* var1) override;  // handleExplosion
	void func_20087_a(Packet100OpenWindow* var1) override;
	void func_20088_a(Packet103SetSlot* var1) override;
	void func_20089_a(Packet106Transaction* var1) override;
	void func_20091_a(Packet102WindowClick* var1) override;  // Window click - client-to-server only (no-op on client)
	void func_20094_a(Packet104WindowItems* var1) override;
	void handleSignUpdate(Packet130UpdateSign* var1) override;
	void func_20090_a(Packet105UpdateProgressbar* var1) override;
	void func_20092_a(Packet101CloseWindow* var1) override;
	void func_28115_a(Packet61DoorChange* var1) override;
	void handle62Sound(Packet62Sound* var1) override;
	void handle63Digging(Packet63Digging* var1) override;
	
	// Additional methods
	void func_28117_a(Packet* var1);  // send packet and wake writer thread
	void addToSendQueue(Packet* var1);
	
	// Getter for Minecraft instance
	Minecraft* getMinecraft() { return mc; }
	
	// Check if disconnected
	bool isDisconnected() const { return disconnected; }
	
	// Set disconnected flag (without triggering networkShutdown)
	void setDisconnected(bool value) { disconnected = value; }
	
	// Flush output stream to ensure packets are sent
	void flushOutput();

private:
	// Helper method matching Java func_12246_a
	// Returns entity by ID, or local player if ID matches
	Entity* func_12246_a(int entityId);
};
