#include "network/NetClientHandler.h"
#include "client/Minecraft.h"
#include "client/player/EntityClientPlayerMP.h"
#include "network/Packet.h"
#include "network/Packet0KeepAlive.h"
#include "network/Packet1Login.h"
#include "network/Packet2Handshake.h"
#include "network/Packet3Chat.h"
#include "network/Packet4UpdateTime.h"
#include "network/Packet10Flying.h"
#include "network/Packet11PlayerPosition.h"
#include "network/Packet12PlayerLook.h"
#include "network/Packet13PlayerLookMove.h"
#include "network/Packet100OpenWindow.h"
#include "network/Packet103SetSlot.h"
#include "network/Packet104WindowItems.h"
#include "client/gui/FurnaceScreen.h"
#include "client/gui/ChestScreen.h"
#include "network/Packet105UpdateProgressbar.h"
#include "network/Packet106Transaction.h"
#include "network/Packet130UpdateSign.h"
#include "network/Packet255KickDisconnect.h"
#include "network/Packet61DoorChange.h"
#include "network/Packet62Sound.h"
#include "network/Packet63Digging.h"
#include "network/Packet5PlayerInventory.h"
#include "network/Packet6SpawnPosition.h"
#include "network/Packet8.h"
#include "network/Packet9.h"
#include "network/Packet14BlockDig.h"
#include "network/Packet15Place.h"
#include "network/Packet17Sleep.h"
#include "network/Packet18ArmAnimation.h"
#include "network/Packet19EntityAction.h"
#include "network/Packet20NamedEntitySpawn.h"
#include "network/Packet21PickupSpawn.h"
#include "network/Packet22Collect.h"
#include "client/particle/TakeAnimationParticle.h"
#include "client/particle/SmokeParticle.h"
#include "world/item/ItemRecord.h"
#include "world/item/Items.h"
#include "network/Packet23VehicleSpawn.h"
#include "network/Packet24MobSpawn.h"
#include "world/entity/Mob.h"
#include "network/Packet25EntityPainting.h"
#include "network/Packet27Position.h"
#include "network/Packet28.h"
#include "network/Packet29DestroyEntity.h"
#include "network/Packet30Entity.h"
#include "network/Packet34EntityTeleport.h"
#include "network/Packet38.h"
#include "network/Packet39.h"
#include "network/Packet40EntityMetadata.h"
#include "network/Packet50PreChunk.h"
#include "network/Packet51MapChunk.h"
#include "network/Packet52MultiBlockChange.h"
#include "network/Packet53BlockChange.h"
#include "network/Packet54PlayNoteBlock.h"
#include "network/Packet60.h"
#include "network/Packet131MapData.h"
#include "network/Packet200Statistic.h"
#include "network/Packet255KickDisconnect.h"
#include "client/gui/GuiConnectFailed.h"
#include "client/gui/GuiDownloadTerrain.h"
#include "client/gui/GuiNewChat.h"
#include "client/multiplayer/MultiPlayerLevel.h"
#include "client/gamemode/MultiplayerMode.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/tile/StoneSlabTile.h"
#include "world/entity/EntityIO.h"
#include "world/entity/player/Player.h"
#include "world/entity/player/EntityOtherPlayerMP.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/item/Boat.h"
#include "world/entity/item/Minecart.h"
#include "world/entity/projectile/Arrow.h"
#include "world/entity/projectile/Snowball.h"
#include "world/entity/projectile/Fireball.h"
#include "world/entity/projectile/FishingHook.h"
#include "world/entity/item/FallingTile.h"
#include "world/entity/PrimedTnt.h"
#include "world/entity/Painting.h"
#include "world/level/Explosion.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "client/renderer/tileentity/SignRenderer.h"
#include "world/phys/ChunkCoordinates.h"
#include "world/item/ItemStack.h"
#include "java/System.h"
#include "java/String.h"
#include "client/gui/ChestScreen.h"
#include "client/gui/FurnaceScreen.h"
#include "client/gui/WorkbenchScreen.h"
#include "client/gui/InventoryScreen.h"
#include "world/level/tile/entity/InventoryBasic.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/tile/entity/FurnaceTileEntity.h"
#include "client/player/LocalPlayer.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cmath>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

NetClientHandler::NetClientHandler(Minecraft* mc, const jstring& host, int port)
	: disconnected(false)
	, field_1209_a(u"")  // Alpha 1.2.6: field_1209_a should be set by server, not initialized to hostname
	, mc(mc)
	, worldClient(nullptr)
	, field_1210_g(false)
{
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("WSAStartup failed");
	}
#endif

	std::string hostStr = String::toUTF8(host);
	
	// Java: Socket var4 = new Socket(InetAddress.getByName(var2), var3);
	struct sockaddr_in serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	
	// Try DNS resolution (matching Java InetAddress.getByName)
	struct hostent* hostEntry = gethostbyname(hostStr.c_str());
	if (hostEntry == nullptr)
	{
		throw std::runtime_error("Failed to resolve host: " + hostStr);
	}
	std::memcpy(&serverAddr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);
	
	SocketHandle socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket == INVALID_SOCKET_HANDLE)
	{
		throw std::runtime_error("Failed to create socket");
	}
	
	if (connect(socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
#ifdef _WIN32
		closesocket(socket);
#else
		close(socket);
#endif
		throw std::runtime_error("Failed to connect to server");
	}
	
	// Java: this.netManager = new NetworkManager(var4, "Client", this);
	netManager = std::make_unique<NetworkManager>(socket, u"Client", this);
}

void NetClientHandler::processReadPackets()
{
	// Java: if(!this.disconnected) {
	if (!disconnected)
	{
		// Java: this.netManager.processReadPackets();
		netManager->processReadPackets();
	}
	
	// Java: this.netManager.wakeThreads();
	netManager->wakeThreads();
}

bool NetClientHandler::isServerHandler()
{
	// Java: return false;
	return false;
}

void NetClientHandler::handleLogin(Packet1Login* var1)
{
	// Java: this.mc.field_6327_b = new PlayerControllerMP(this.mc, this);
	mc->gameMode = std::make_shared<MultiplayerMode>(*mc, this);
	
	// Java: if(var1.field_4073_e == 1) {
	//           var1.field_4073_e = 0;
	//       }
	int dimension = var1->field_4073_e;
	if (dimension == 1)
	{
		dimension = 0;
	}
	
	// Java: this.worldClient = new WorldClient(this, var1.field_4074_d, var1.field_4073_e);
	// MultiPlayerLevel constructor sets isOnline = true (equivalent to multiplayerWorld = true)
	// Mark old worldClient as invalid if it exists
	if (worldClient != nullptr)
	{
		worldClient->disconnect();  // This sets isValid = false
	}
	worldClient = new MultiPlayerLevel(this, var1->field_4074_d, dimension);
	
	// Java: this.worldClient.multiplayerWorld = true;
	// Already set in MultiPlayerLevel constructor (isOnline = true)
	
	// Java: this.mc.func_6261_a(this.worldClient);
	mc->setLevel(std::shared_ptr<Level>(worldClient));
	
	// Java: this.mc.thePlayer.dimension = var1.field_4073_e;
	if (mc->player != nullptr)
	{
		mc->player->dimension = dimension;
	}
	
	// Java: this.mc.displayGuiScreen(new GuiDownloadTerrain(this));
	mc->setScreen(std::make_shared<GuiDownloadTerrain>(*mc, this));
	
	// Java: this.mc.thePlayer.field_620_ab = var1.protocolVersion;
	if (mc->player != nullptr)
	{
		mc->player->entityId = var1->protocolVersion;
	}
	
}

// Placeholder implementations for all other handlers - will be implemented when packet classes exist
// For now, matching Java structure exactly

void NetClientHandler::handlePickupSpawn(Packet21PickupSpawn* var1)
{
	// Alpha 1.2.6: NetClientHandler.handlePickupSpawn() - creates EntityItem
	// Java: public void handlePickupSpawn(Packet21PickupSpawn var1) {
	//     double var2 = (double)var1.xPosition / 32.0D;
	//     double var4 = (double)var1.yPosition / 32.0D;
	//     double var6 = (double)var1.zPosition / 32.0D;
	//     EntityItem var8 = new EntityItem(this.worldClient, var2, var4, var6, new ItemStack(var1.itemId, var1.count, var1.itemDamage));
	//     var8.motionX = (double)var1.rotation / 128.0D;
	//     var8.motionY = (double)var1.pitch / 128.0D;
	//     var8.motionZ = (double)var1.roll / 128.0D;
	//     var8.field_9303_br = var1.xPosition;
	//     var8.field_9302_bs = var1.yPosition;
	//     var8.field_9301_bt = var1.zPosition;
	//     this.worldClient.func_712_a(var1.entityId, var8);
	// }
	if (worldClient == nullptr)
		return;
	
	double x = static_cast<double>(var1->xPosition) / 32.0;
	double y = static_cast<double>(var1->yPosition) / 32.0;
	double z = static_cast<double>(var1->zPosition) / 32.0;
	
	// Java: new ItemStack(var1.itemId, var1.count, var1.itemDamage)
	ItemStack stack(var1->itemId, var1->count, var1->itemDamage);
	
	// Java: EntityItem var8 = new EntityItem(this.worldClient, var2, var4, var6, ...);
	std::shared_ptr<EntityItem> itemEntity = Util::make_shared<EntityItem>(*worldClient, x, y, z, stack);
	
	// Java: var8.motionX = (double)var1.rotation / 128.0D;
	//       var8.motionY = (double)var1.pitch / 128.0D;
	//       var8.motionZ = (double)var1.roll / 128.0D;
	itemEntity->xd = static_cast<double>(static_cast<signed char>(var1->rotation)) / 128.0;
	itemEntity->yd = static_cast<double>(static_cast<signed char>(var1->pitch)) / 128.0;
	itemEntity->zd = static_cast<double>(static_cast<signed char>(var1->roll)) / 128.0;
	
	// Java: var8.field_9303_br = var1.xPosition;
	//       var8.field_9302_bs = var1.yPosition;
	//       var8.field_9301_bt = var1.zPosition;
	itemEntity->field_9303_br = var1->xPosition;
	itemEntity->field_9302_bs = var1->yPosition;
	itemEntity->field_9301_bt = var1->zPosition;
	
	// Java: this.worldClient.func_712_a(var1.entityId, var8);
	worldClient->putEntity(var1->entityId, std::static_pointer_cast<Entity>(itemEntity));
	worldClient->addEntity(std::static_pointer_cast<Entity>(itemEntity));
}

void NetClientHandler::handleVehicleSpawn(Packet23VehicleSpawn* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleVehicleSpawn() - creates vehicle/projectile entities
	// Java: public void handleVehicleSpawn(Packet23VehicleSpawn var1) {
	//     double var2 = (double)var1.xPosition / 32.0D;
	//     double var4 = (double)var1.yPosition / 32.0D;
	//     double var6 = (double)var1.zPosition / 32.0D;
	//     Object var8 = null;
	//     switch(var1.type) {
	//     case 1: var8 = new EntityBoat(...); break;
	//     case 10: var8 = new EntityMinecart(..., 0); break;
	//     case 11: var8 = new EntityMinecart(..., 1); break;
	//     case 12: var8 = new EntityMinecart(..., 2); break;
	//     case 50: var8 = new EntityTNTPrimed(...); break;
	//     case 60: var8 = new EntityArrow(...); break;
	//     case 61: var8 = new EntitySnowball(...); break;
	//     case 63: var8 = new EntityFireball(..., velocity); break;
	//     case 70: var8 = new EntityFallingSand(..., Block.sand.blockID); break;
	//     case 71: var8 = new EntityFallingSand(..., Block.gravel.blockID); break;
	//     case 90: var8 = new EntityFish(...); break;
	//     }
	//     if(var8 != null) {
	//         ((Entity)var8).field_9303_br = var1.xPosition;
	//         ((Entity)var8).field_9302_bs = var1.yPosition;
	//         ((Entity)var8).field_9301_bt = var1.zPosition;
	//         ((Entity)var8).rotationYaw = 0.0F;
	//         ((Entity)var8).rotationPitch = 0.0F;
	//         ((Entity)var8).field_620_ab = var1.entityId;
	//         this.worldClient.func_712_a(var1.entityId, (Entity)var8);
	//         if(var1.field_28044_i > 0) {
	//             if(var1.type == 60) {
	//                 Entity var9 = this.func_12246_a(var1.field_28044_i);
	//                 if(var9 instanceof EntityLiving) {
	//                     ((EntityArrow)var8).field_682_g = (EntityLiving)var9;
	//                 }
	//             }
	//             ((Entity)var8).setVelocity((double)var1.field_28047_e / 8000.0D, (double)var1.field_28046_f / 8000.0D, (double)var1.field_28045_g / 8000.0D);
	//         }
	//     }
	// }
	if (worldClient == nullptr)
		return;
	
	double x = static_cast<double>(var1->xPosition) / 32.0;
	double y = static_cast<double>(var1->yPosition) / 32.0;
	double z = static_cast<double>(var1->zPosition) / 32.0;
	
	std::shared_ptr<Entity> entity = nullptr;
	
	switch (var1->type)
	{
	case 1:  // Boat
		entity = Util::make_shared<Boat>(*worldClient, x, y, z);
		break;
	case 10:  // Minecart (Rideable)
		entity = Util::make_shared<Minecart>(*worldClient, x, y, z, 0);
		break;
	case 11:  // Minecart (Chest)
		entity = Util::make_shared<Minecart>(*worldClient, x, y, z, 1);
		break;
	case 12:  // Minecart (Furnace)
		entity = Util::make_shared<Minecart>(*worldClient, x, y, z, 2);
		break;
	case 50:  // PrimedTnt
		entity = Util::make_shared<PrimedTnt>(*worldClient, x, y, z);
		break;
	case 60:  // Arrow
		entity = Util::make_shared<Arrow>(*worldClient, x, y, z);
		break;
	case 61:  // Snowball
		entity = Util::make_shared<Snowball>(*worldClient, x, y, z);
		break;
	case 63:  // Fireball
	{
		double vx = static_cast<double>(var1->field_28047_e) / 8000.0;
		double vy = static_cast<double>(var1->field_28046_f) / 8000.0;
		double vz = static_cast<double>(var1->field_28045_g) / 8000.0;
		// Fireball constructor: Fireball(Level &level, Mob &owner, double x, double y, double z)
		// But we don't have owner, so use default constructor then set velocity
		entity = Util::make_shared<Fireball>(*worldClient);
		entity->moveTo(x, y, z, 0.0f, 0.0f);
		entity->lerpMotion(vx, vy, vz);
		var1->field_28044_i = 0;  // Java sets this to 0 for Fireball
		break;
	}
	case 70:  // FallingTile (Sand)
		entity = Util::make_shared<FallingTile>(*worldClient, x, y, z, 12);  // Block.sand.blockID = 12
		break;
	case 71:  // FallingTile (Gravel)
		entity = Util::make_shared<FallingTile>(*worldClient, x, y, z, 13);  // Block.gravel.blockID = 13
		break;
	case 90:  // FishingHook (EntityFish in Alpha/Beta)
	{
		// Beta: Create FishingHook entity (FishingHook.java:55-58)
		entity = Util::make_shared<FishingHook>(*worldClient, x, y, z);
		break;
	}
	default:
		return;
	}
	
	if (entity != nullptr)
	{
		// Java: ((Entity)var8).field_9303_br = var1.xPosition;
		//       ((Entity)var8).field_9302_bs = var1.yPosition;
		//       ((Entity)var8).field_9301_bt = var1.zPosition;
		//       ((Entity)var8).rotationYaw = 0.0F;
		//       ((Entity)var8).rotationPitch = 0.0F;
		//       ((Entity)var8).field_620_ab = var1.entityId;
		entity->field_9303_br = var1->xPosition;
		entity->field_9302_bs = var1->yPosition;
		entity->field_9301_bt = var1->zPosition;
		entity->yRot = 0.0f;
		entity->xRot = 0.0f;
		entity->entityId = var1->entityId;
		
		// Java: this.worldClient.func_712_a(var1.entityId, (Entity)var8);
		worldClient->putEntity(var1->entityId, entity);
		worldClient->addEntity(entity);
		
		// Java: if(var1.field_28044_i > 0) {
		//     if(var1.type == 60) {
		//         Entity var9 = this.func_12246_a(var1.field_28044_i);
		//         if(var9 instanceof EntityLiving) {
		//             ((EntityArrow)var8).field_682_g = (EntityLiving)var9;
		//         }
		//     }
		//     ((Entity)var8).setVelocity((double)var1.field_28047_e / 8000.0D, (double)var1.field_28046_f / 8000.0D, (double)var1.field_28045_g / 8000.0D);
		// }
		if (var1->field_28044_i > 0)
		{
			if (var1->type == 60)  // Arrow
			{
				Entity* ownerEntity = func_12246_a(var1->field_28044_i);
				if (ownerEntity != nullptr)
				{
					Mob* mobOwner = dynamic_cast<Mob*>(ownerEntity);
					if (mobOwner != nullptr)
					{
						Arrow* arrow = dynamic_cast<Arrow*>(entity.get());
						if (arrow != nullptr)
						{
							arrow->owner = mobOwner;
						}
					}
				}
			}
			else if (var1->type == 90)  // FishingHook
			{
				// Beta: Set FishingHook owner from field_28044_i (player entity ID)
				Entity* ownerEntity = func_12246_a(var1->field_28044_i);
				if (ownerEntity != nullptr)
				{
					Player* playerOwner = dynamic_cast<Player*>(ownerEntity);
					if (playerOwner != nullptr)
					{
						FishingHook* hook = dynamic_cast<FishingHook*>(entity.get());
						if (hook != nullptr)
						{
							hook->owner = playerOwner;
							playerOwner->fishing = hook;  // Beta: Set player.fishing reference
						}
					}
				}
			}
			
			// Java: setVelocity((double)var1.field_28047_e / 8000.0D, ...)
			double vx = static_cast<double>(var1->field_28047_e) / 8000.0;
			double vy = static_cast<double>(var1->field_28046_f) / 8000.0;
			double vz = static_cast<double>(var1->field_28045_g) / 8000.0;
			entity->lerpMotion(vx, vy, vz);
		}
	}
}

void NetClientHandler::handlePainting(Packet25EntityPainting* var1)
{
	// Alpha 1.2.6: NetClientHandler.handlePainting() - creates EntityPainting
	// Java: public void handlePainting(Packet25EntityPainting var1) {
	//     EntityPainting var2 = new EntityPainting(this.worldClient, var1.xPosition, var1.yPosition, var1.zPosition, var1.direction, var1.title);
	//     this.worldClient.func_712_a(var1.entityId, var2);
	// }
	if (worldClient == nullptr)
		return;
	
	// Java: new EntityPainting(this.worldClient, var1.xPosition, var1.yPosition, var1.zPosition, var1.direction, var1.title)
	// Note: Painting constructor is Painting(Level &level, int_t x, int_t y, int_t z, int_t dir) or with title
	std::shared_ptr<Painting> painting = Util::make_shared<Painting>(*worldClient, var1->xPosition, var1->yPosition, var1->zPosition, var1->direction, var1->title);
	
	// Java: this.worldClient.func_712_a(var1.entityId, var2);
	worldClient->putEntity(var1->entityId, std::static_pointer_cast<Entity>(painting));
	worldClient->addEntity(std::static_pointer_cast<Entity>(painting));
}

void NetClientHandler::func_6498_a(Packet28* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_6498_a() - handles entity velocity (knockback from damage)
	// Java: public void func_6498_a(Packet28 var1) {
	//     Entity var2 = this.func_12246_a(var1.field_6367_a);
	//     if(var2 != null) {
	//         var2.setVelocity((double)var1.field_6366_b / 8000.0D, (double)var1.field_6369_c / 8000.0D, (double)var1.field_6368_d / 8000.0D);
	//     }
	// }
	Entity* entity = func_12246_a(var1->entityId);
	if (entity != nullptr)
	{
		// Java: var2.setVelocity((double)var1.field_6366_b / 8000.0D, (double)var1.field_6369_c / 8000.0D, (double)var1.field_6368_d / 8000.0D);
		// Alpha 1.2.6: Packet28 stores velocity as shorts (field_6366_b, field_6369_c, field_6368_d)
		// Decode by dividing by 8000.0 to get actual velocity
		double motionX = static_cast<double>(var1->motionX) / 8000.0;
		double motionY = static_cast<double>(var1->motionY) / 8000.0;
		double motionZ = static_cast<double>(var1->motionZ) / 8000.0;
		entity->setVelocity(motionX, motionY, motionZ);
	}
}

void NetClientHandler::handleEntityMetadata(Packet40EntityMetadata* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleEntityMetadata() - updates entity metadata
	// Java: Entity var2 = this.func_12246_a(var1.field_21048_a);
	//       if(var2 != null) {
	//           var2.getDataWatcher().updateWatchedObjectsFromList(var1.func_21047_b());
	//       }
	Entity* entity = func_12246_a(var1->entityId);
	if (entity != nullptr)
	{
		// Java: var2.getDataWatcher().updateWatchedObjectsFromList(var1.func_21047_b());
		// Packet40EntityMetadata stores metadata in field_21048_b vector
		if (!var1->field_21048_b.empty())
		{
			entity->getDataWatcher().updateWatchedObjectsFromList(var1->field_21048_b);
			
			// Alpha 1.2.6: Sync shared flags (sneaking, onFire, riding) from DataWatcher
			// Data ID 0 = shared flags byte (FLAG_ONFIRE=0, FLAG_SNEAKING=1, FLAG_RIDING=2)
			for (const auto& watchableObj : var1->field_21048_b)
			{
				if (watchableObj->getDataValueId() == 0 && watchableObj->getObjectType() == 0)  // Data ID 0, Type 0 = Byte
				{
					byte_t flags = watchableObj->getByte();
					// Note: onFire flag is stored in DataWatcher and checked via isOnFire() which reads from DataWatcher in multiplayer
					// We don't need to update entity->onFire here since baseTick() resets it and isOnFire() checks DataWatcher directly
					
					// Update sneaking flag from DataWatcher
					// Java: FLAG_SNEAKING = 1, so check bit 1
					bool sneaking = (flags & (1 << Entity::FLAG_SNEAKING)) != 0;
					entity->setSneaking(sneaking);
				}
			}
			
			// Alpha 1.2.6: Note - hurtTime and deathTime are primarily handled via Packet38 (Entity Status)
			// Packet40EntityMetadata may also contain these values, but Packet38 is the main mechanism
			// We still check metadata for any additional synchronization
			Mob* mobEntity = dynamic_cast<Mob*>(entity);
			if (mobEntity != nullptr)
			{
				// Iterate through all metadata objects and apply short/int values
				// Data IDs may vary - we check for short values that could be hurtTime/deathTime/attackTime
				for (const auto& watchableObj : var1->field_21048_b)
				{
					if (watchableObj->getObjectType() == 1)  // Type 1 = Short
					{
						short_t value = watchableObj->getShort();
						int dataId = watchableObj->getDataValueId();
						
						// Try common data IDs (may need adjustment based on server)
						// Note: Packet38 is the primary mechanism for hurtTime (status 2) and death (status 3)
						if (dataId >= 1 && dataId <= 5)
						{
							// Apply short values - exact mapping may vary
							// Common: 2=hurtTime, 3=deathTime, 4=attackTime
							if (dataId == 2)
							{
								mobEntity->hurtTime = static_cast<int_t>(value);
								mobEntity->hurtDuration = static_cast<int_t>(value);
							}
							else if (dataId == 3)
							{
								mobEntity->deathTime = static_cast<int_t>(value);
							}
							else if (dataId == 4)
							{
								mobEntity->attackTime = static_cast<int_t>(value);
							}
						}
					}
				}
			}
		}
	}
}

void NetClientHandler::handleNamedEntitySpawn(Packet20NamedEntitySpawn* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleNamedEntitySpawn() - creates EntityOtherPlayerMP
	// Java: double var2 = (double)var1.xPosition / 32.0D;
	//       double var4 = (double)var1.yPosition / 32.0D;
	//       double var6 = (double)var1.zPosition / 32.0D;
	//       float var8 = (float)(var1.rotation * 360) / 256.0F;
	//       float var9 = (float)(var1.pitch * 360) / 256.0F;
	//       EntityOtherPlayerMP var10 = new EntityOtherPlayerMP(this.mc.theWorld, var1.name);
	if (worldClient == nullptr)
		return;
	
	double x = static_cast<double>(var1->xPosition) / 32.0;
	double y = static_cast<double>(var1->yPosition) / 32.0;
	double z = static_cast<double>(var1->zPosition) / 32.0;
	float yaw = static_cast<float>(var1->rotation * 360) / 256.0f;
	float pitch = static_cast<float>(var1->pitch * 360) / 256.0f;
	
	// Java: EntityOtherPlayerMP var10 = new EntityOtherPlayerMP(this.mc.theWorld, var1.name);
	// Alpha 1.2.6: EntityOtherPlayerMP has yOffset = 0.0F (unlike Player which has heightOffset = 1.62f)
	// This is critical for correct rendering position
	std::shared_ptr<EntityOtherPlayerMP> otherPlayer = Util::make_shared<EntityOtherPlayerMP>(*worldClient, var1->name);
	otherPlayer->entityId = var1->entityId;
	// Note: EntityOtherPlayerMP already sets heightOffset = 0.0f in constructor
	
	// Java: var10.prevPosX = var10.lastTickPosX = (double)(var10.field_9303_br = var1.xPosition);
	//       var10.prevPosY = var10.lastTickPosY = (double)(var10.field_9302_bs = var1.yPosition);
	//       var10.prevPosZ = var10.lastTickPosZ = (double)(var10.field_9301_bt = var1.zPosition);
	// Alpha 1.2.6: Set encoded positions and prevPos to decoded position
	otherPlayer->field_9303_br = var1->xPosition;
	otherPlayer->field_9302_bs = var1->yPosition;
	otherPlayer->field_9301_bt = var1->zPosition;
	otherPlayer->xo = x;  // prevPosX
	otherPlayer->yo = y;  // prevPosY
	otherPlayer->zo = z;  // prevPosZ
	otherPlayer->xOld = x;  // lastTickPosX
	otherPlayer->yOld = y;  // lastTickPosY
	otherPlayer->zOld = z;  // lastTickPosZ
	
	// Java: int var11 = var1.currentItem;
	//       if(var11 == 0) {
	//           var10.inventory.mainInventory[var10.inventory.currentItem] = null;
	//       } else {
	//           var10.inventory.mainInventory[var10.inventory.currentItem] = new ItemStack(var11, 1, 0);
	//       }
	int_t currentItem = var1->currentItem;
	if (currentItem == 0)
	{
		// Java: null means empty slot
		otherPlayer->inventory.mainInventory[otherPlayer->inventory.currentItem] = ItemStack();
	}
	else
	{
		// Java: new ItemStack(var11, 1, 0) - item ID, stack size 1, damage 0
		otherPlayer->inventory.mainInventory[otherPlayer->inventory.currentItem] = ItemStack(currentItem, 1, 0);
	}
	
	// Java: var10.setPositionAndRotation(var2, var4, var6, var8, var9);
	// Alpha 1.2.6: setPositionAndRotation sets posX/Y/Z and rotationYaw/Pitch
	// For initial spawn, moveTo is fine (sets old positions to new position)
	otherPlayer->moveTo(x, y, z, yaw, pitch);
	
	// Java: this.worldClient.func_712_a(var1.entityId, var10);
	// Alpha 1.2.6: func_712_a adds entity to world with entityId mapping
	// Java: public void func_712_a(int var1, Entity var2) {
	//     Entity var3 = this.func_709_b(var1);  // Get existing entity by ID
	//     if(var3 != null) {
	//         this.setEntityDead(var3);  // Remove existing entity
	//     }
	//     this.F.add(var2);  // Add to forced set
	//     var2.field_620_ab = var1;  // Set entityId
	//     if(!this.entityJoinedWorld(var2)) {
	//         this.field_1053_F.add(var2);  // Add to reEntries if not joined
	//     }
	//     this.field_1055_D.addKey(var1, var2);  // Add to entity ID map
	// }
	// In C++, we use putEntity() which does the ID mapping, then addEntity() to add to world
	worldClient->putEntity(var1->entityId, otherPlayer);
	worldClient->addEntity(otherPlayer);
}

void NetClientHandler::handleEntityTeleport(Packet34EntityTeleport* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleEntityTeleport() - teleports entity
	// Java: public void handleEntityTeleport(Packet34EntityTeleport var1) {
	//     Entity var2 = this.func_12246_a(var1.entityId);
	//     if(var2 != null) {
	//         var2.field_9303_br = var1.xPosition;
	//         var2.field_9302_bs = var1.yPosition;
	//         var2.field_9301_bt = var1.zPosition;
	//         double var3 = (double)var2.field_9303_br / 32.0D;
	//         double var5 = (double)var2.field_9302_bs / 32.0D + 1.0D / 64.0D;
	//         double var7 = (double)var2.field_9301_bt / 32.0D;
	//         float var9 = (float)(var1.yaw * 360) / 256.0F;
	//         float var10 = (float)(var1.pitch * 360) / 256.0F;
	//         var2.setPositionAndRotation2(var3, var5, var7, var9, var10, 3);
	//     }
	// }
	Entity* entity = func_12246_a(var1->entityId);
	if (entity != nullptr)
	{
		// Java: var2.field_9303_br = var1.xPosition;
		//       var2.field_9302_bs = var1.yPosition;
		//       var2.field_9301_bt = var1.zPosition;
		// Alpha 1.2.6: Set encoded positions directly (absolute teleport)
		entity->field_9303_br = var1->xPosition;
		entity->field_9302_bs = var1->yPosition;
		entity->field_9301_bt = var1->zPosition;
		
		// Java: double var3 = (double)var2.field_9303_br / 32.0D;
		//       double var5 = (double)var2.field_9302_bs / 32.0D + 1.0D / 64.0D;
		//       double var7 = (double)var2.field_9301_bt / 32.0D;
		double x = static_cast<double>(entity->field_9303_br) / 32.0;
		double y = static_cast<double>(entity->field_9302_bs) / 32.0 + (1.0 / 64.0);
		double z = static_cast<double>(entity->field_9301_bt) / 32.0;
		// Java: float var9 = (float)(var1.yaw * 360) / 256.0F;
		//       float var10 = (float)(var1.pitch * 360) / 256.0F;
		// CRITICAL: Java treats bytes as SIGNED (-128 to 127), not unsigned
		// Wiki confirms: "byte" is "Signed, two's complement" with range "-128 to 127"
		// The formula (byte * 360) / 256 converts signed byte to degrees
		// Example from wiki: "Yaw field 64 means a 90 degree turn" = (64 * 360) / 256 = 90
		// For negative values: (-64 * 360) / 256 = -90 degrees
		// Since byte_t is std::int8_t (signed), we can use it directly
		float yaw = static_cast<float>(var1->yaw * 360) / 256.0f;
		float pitch = static_cast<float>(var1->pitch * 360) / 256.0f;
		
		// Java: var2.setPositionAndRotation2(var3, var5, var7, var9, var10, 3);
		// Alpha 1.2.6: setPositionAndRotation2 stores target position and interpolates over 3 ticks
		entity->lerpTo(x, y, z, yaw, pitch, 3);
	}
}

void NetClientHandler::handleEntity(Packet30Entity* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleEntity() - updates entity position
	// Java: public void handleEntity(Packet30Entity var1) {
	//     Entity var2 = this.func_12246_a(var1.entityId);
	//     if(var2 != null) {
	//         var2.field_9303_br += var1.xPosition;
	//         var2.field_9302_bs += var1.yPosition;
	//         var2.field_9301_bt += var1.zPosition;
	//         double var3 = (double)var2.field_9303_br / 32.0D;
	//         double var5 = (double)var2.field_9302_bs / 32.0D + 1.0D / 64.0D;
	//         double var7 = (double)var2.field_9301_bt / 32.0D;
	//         float var9 = var1.rotating ? (float)(var1.yaw * 360) / 256.0F : var2.rotationYaw;
	//         float var10 = var1.rotating ? (float)(var1.pitch * 360) / 256.0F : var2.rotationPitch;
	//         var2.setPositionAndRotation2(var3, var5, var7, var9, var10, 3);
	//     }
	// }
	// Note: Packet30Entity only has entityId, but Packet31RelEntityMove, Packet32EntityLook, Packet33RelEntityMoveLook
	// extend it and add movement/rotation data. They call handleEntity with the full packet.
	// For Packet30Entity base class, there's no movement data, so this is a no-op.
	// The actual movement is handled by Packet31RelEntityMove, Packet32EntityLook, Packet33RelEntityMoveLook
	// which override processPacket to call handleEntity with their own data.
	Entity* entity = func_12246_a(var1->entityId);
	if (entity != nullptr)
	{
		// Java: var2.field_9303_br += var1.xPosition;
		//       var2.field_9302_bs += var1.yPosition;
		//       var2.field_9301_bt += var1.zPosition;
		// Alpha 1.2.6: Add relative movement bytes directly to encoded positions
		// Wiki confirms: "byte" is "Signed, two's complement" with range "-128 to 127"
		// "This packet allows at most four blocks movement in any direction, because byte range is from -128 to 127"
		// Since byte_t is std::int8_t (signed), we can use it directly - C++ will sign-extend when adding to int
		entity->field_9303_br += var1->xPosition;
		entity->field_9302_bs += var1->yPosition;
		entity->field_9301_bt += var1->zPosition;
		
		// Java: double var3 = (double)var2.field_9303_br / 32.0D;
		//       double var5 = (double)var2.field_9302_bs / 32.0D + 1.0D / 64.0D;
		//       double var7 = (double)var2.field_9301_bt / 32.0D;
		double x = static_cast<double>(entity->field_9303_br) / 32.0;
		double y = static_cast<double>(entity->field_9302_bs) / 32.0 + (1.0 / 64.0);
		double z = static_cast<double>(entity->field_9301_bt) / 32.0;
		
		// Java: float var9 = var1.rotating ? (float)(var1.yaw * 360) / 256.0F : var2.rotationYaw;
		//       float var10 = var1.rotating ? (float)(var1.pitch * 360) / 256.0F : var2.rotationPitch;
		// Java: float var9 = var1.rotating ? (float)(var1.yaw * 360) / 256.0F : var2.rotationYaw;
		//       float var10 = var1.rotating ? (float)(var1.pitch * 360) / 256.0F : var2.rotationPitch;
		// CRITICAL: Java treats bytes as SIGNED (-128 to 127), not unsigned
		// Wiki confirms: "byte" is "Signed, two's complement" with range "-128 to 127"
		// The formula (byte * 360) / 256 converts signed byte to degrees
		// Example from wiki: "Yaw field 64 means a 90 degree turn" = (64 * 360) / 256 = 90
		// For negative values: (-64 * 360) / 256 = -90 degrees
		// Since byte_t is std::int8_t (signed), we can use it directly
		float yaw = var1->rotating ? static_cast<float>(var1->yaw * 360) / 256.0f : entity->yRot;
		float pitch = var1->rotating ? static_cast<float>(var1->pitch * 360) / 256.0f : entity->xRot;
		
		// Java: var2.setPositionAndRotation2(var3, var5, var7, var9, var10, 3);
		// Alpha 1.2.6: setPositionAndRotation2 stores target position and interpolates over 3 ticks
		entity->lerpTo(x, y, z, yaw, pitch, 3);
	}
}

void NetClientHandler::handleDestroyEntity(Packet29DestroyEntity* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleDestroyEntity() - removes entity from world
	// Java: public void handleDestroyEntity(Packet29DestroyEntity var1) {
	//     this.worldClient.func_710_c(var1.entityId);
	// }
	// Java: public Entity func_710_c(int var1) {
	//     Entity var2 = (Entity)this.field_1055_D.removeObject(var1);
	//     if(var2 != null) {
	//         this.F.remove(var2);
	//         this.setEntityDead(var2);
	//     }
	//     return var2;
	// }
	if (worldClient != nullptr)
	{
		worldClient->removeEntity(var1->entityId);
	}
}

void NetClientHandler::handleFlying(Packet10Flying* var1)
{
	// Java: EntityPlayerSP var2 = this.mc.thePlayer;
	if (mc->player == nullptr)
	{
		return;
	}
	
	// Java: double var3 = var2.posX;
	//       double var5 = var2.posY;
	//       double var7 = var2.posZ;
	//       float var9 = var2.rotationYaw;
	//       float var10 = var2.rotationPitch;
	double x = mc->player->x;
	double y = mc->player->y;
	double z = mc->player->z;
	float yaw = mc->player->yRot;
	float pitch = mc->player->xRot;
	
	// Java: if(var1.moving) {
	//           var3 = var1.xPosition;
	//           var5 = var1.yPosition;
	//           var7 = var1.zPosition;
	//       }
	if (var1->moving)
	{
		x = var1->xPosition;
		y = var1->yPosition;
		z = var1->zPosition;
	}
	
	// Java: if(var1.rotating) {
	//           var9 = var1.yaw;
	//           var10 = var1.pitch;
	//       }
	if (var1->rotating)
	{
		yaw = var1->yaw;
		pitch = var1->pitch;
	}
	
	// Java: var2.field_9287_aY = 0.0F; (NetClientHandler.java:225)
	// Alpha 1.2.6: Reset vertical bobbing offset when server updates position
	mc->player->field_9287_aY = 0.0f;
	
	// Java: var2.motionX = var2.motionY = var2.motionZ = 0.0D;
	mc->player->xd = 0.0;
	mc->player->yd = 0.0;
	mc->player->zd = 0.0;
	
	// Java: var2.setPositionAndRotation(var3, var5, var7, var9, var10);
	mc->player->moveTo(x, y, z, yaw, pitch);
	
	// Java: var1.xPosition = var2.posX;
	//       var1.yPosition = var2.boundingBox.minY;
	//       var1.zPosition = var2.posZ;
	//       var1.stance = var2.posY;
	// Note: In Java, the same packet object is reused. In C++, we need to create a new packet
	// because the original packet is owned by NetworkManager's readPackets and will be destroyed
	// after processPacket returns. Since we're updating both position and rotation, we send
	// Packet13PlayerLookMove (which includes both position and rotation data).
	Packet13PlayerLookMove* replyPacket = new Packet13PlayerLookMove(
		mc->player->x,      // xPosition
		mc->player->bb.y0,  // yPosition (boundingBox.minY)
		mc->player->y,      // stance (posY)
		mc->player->z,      // zPosition
		yaw,                // yaw
		pitch,              // pitch
		var1->onGround      // onGround
	);
	
	// Java: this.netManager.addToSendQueue(var1);
	addToSendQueue(replyPacket);
	
	// Java: if(!this.field_1210_g) {
	//           this.mc.thePlayer.prevPosX = this.mc.thePlayer.posX;
	//           this.mc.thePlayer.prevPosY = this.mc.thePlayer.posY;
	//           this.mc.thePlayer.prevPosZ = this.mc.thePlayer.posZ;
	//           this.field_1210_g = true;
	//           this.mc.displayGuiScreen((GuiScreen)null);
	//       }
	if (!field_1210_g)
	{
		mc->player->xo = mc->player->x;
		mc->player->yo = mc->player->y;
		mc->player->zo = mc->player->z;
		field_1210_g = true;
		mc->setScreen(nullptr);  // Transition from GuiDownloadTerrain to game
	}
}

void NetClientHandler::handlePreChunk(Packet50PreChunk* var1)
{
	// Java: this.worldClient.func_713_a(var1.xPosition, var1.yPosition, var1.mode);
	if (worldClient != nullptr)
	{
		worldClient->setChunkVisible(var1->xPosition, var1->yPosition, var1->mode);
	}
}

void NetClientHandler::handleMultiBlockChange(Packet52MultiBlockChange* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleMultiBlockChange() - updates multiple blocks in a chunk
	// Java: public void handleMultiBlockChange(Packet52MultiBlockChange var1) {
	//     Chunk var2 = this.worldClient.getChunkFromChunkCoords(var1.xPosition, var1.zPosition);
	//     int var3 = var1.xPosition * 16;
	//     int var4 = var1.zPosition * 16;
	//     for(int var5 = 0; var5 < var1.size; ++var5) {
	//         short var6 = var1.coordinateArray[var5];
	//         int var7 = var1.typeArray[var5] & 255;
	//         byte var8 = var1.metadataArray[var5];
	//         int var9 = var6 >> 12 & 15;
	//         int var10 = var6 >> 8 & 15;
	//         int var11 = var6 & 255;
	//         var2.setBlockIDWithMetadata(var9, var11, var10, var7, var8);
	//         this.worldClient.func_711_c(var9 + var3, var11, var10 + var4, var9 + var3, var11, var10 + var4);
	//         this.worldClient.func_701_b(var9 + var3, var11, var10 + var4, var9 + var3, var11, var10 + var4);
	//     }
	// }
	if (worldClient == nullptr)
		return;
	
	// Get chunk at chunk coordinates
	std::shared_ptr<LevelChunk> chunk = worldClient->getChunk(var1->xPosition, var1->zPosition);
	if (chunk == nullptr)
		return;
	
	// Calculate world coordinates for this chunk
	int_t worldX = var1->xPosition * 16;
	int_t worldZ = var1->zPosition * 16;
	
	// Process each block change
	for (int_t i = 0; i < var1->size; ++i)
	{
		short_t coord = var1->coordinateArray[i];
		int_t type = static_cast<int_t>(var1->typeArray[i]) & 0xFF;
		byte_t metadata = var1->metadataArray[i];
		
		// Extract local coordinates from packed short
		// Format: xxxx zzzz yyyyyyyy (4 bits x, 4 bits z, 8 bits y)
		int_t localX = (coord >> 12) & 15;
		int_t localZ = (coord >> 8) & 15;
		int_t localY = coord & 255;
		
		// Set block in chunk (local coordinates)
		// Note: chunk->setTileAndData() already calls updateLight() for the block
		chunk->setTileAndData(localX, localY, localZ, type, metadata);
		
		// Calculate world coordinates
		int_t wx = localX + worldX;
		int_t wy = localY;
		int_t wz = localZ + worldZ;
		
		// Clear reset region for this block (func_711_c)
		worldClient->clearResetRegion(wx, wy, wz, wx, wy, wz);
		
		// Mark tiles dirty for this block (func_701_b)
		worldClient->setTilesDirty(wx, wy, wz, wx, wy, wz);
		
		// Alpha 1.2.6: Update lighting for neighboring steps and farmland
		// These blocks check their neighbors' stored lighting values, so we need to
		// ensure those stored values are updated when neighbors change
		int_t neighbors[6][3] = {
			{wx - 1, wy, wz}, {wx + 1, wy, wz},
			{wx, wy - 1, wz}, {wx, wy + 1, wz},
			{wx, wy, wz - 1}, {wx, wy, wz + 1}
		};
		
		for (int j = 0; j < 6; j++)
		{
			int_t nx = neighbors[j][0];
			int_t ny = neighbors[j][1];
			int_t nz = neighbors[j][2];
			int_t neighborTile = worldClient->getTile(nx, ny, nz);
			
			// If neighbor is a step or farmland, update its lighting to recalculate stored values
			if (neighborTile == Tile::stoneSlabHalf.id || neighborTile == Tile::farmland.id)
			{
				// Trigger lighting update to recalculate and store lighting values
				worldClient->updateLight(LightLayer::Sky, nx, ny, nz, nx, ny, nz);
				worldClient->updateLight(LightLayer::Block, nx, ny, nz, nx, ny, nz);
				// Mark dirty for rendering
				worldClient->setTilesDirty(nx, ny, nz, nx, ny, nz);
			}
		}
	}
}

void NetClientHandler::handleMapChunk(Packet51MapChunk* var1)
{
	// Java: this.worldClient.func_711_c(var1.xPosition, var1.yPosition, var1.zPosition, var1.xPosition + var1.xSize - 1, var1.yPosition + var1.ySize - 1, var1.zPosition + var1.zSize - 1);
	//       this.worldClient.func_693_a(var1.xPosition, var1.yPosition, var1.zPosition, var1.xSize, var1.ySize, var1.zSize, var1.chunk);
	if (worldClient != nullptr)
	{
		worldClient->clearResetRegion(var1->xPosition, var1->yPosition, var1->zPosition, 
		                              var1->xPosition + var1->xSize - 1, 
		                              var1->yPosition + var1->ySize - 1, 
		                              var1->zPosition + var1->zSize - 1);
		worldClient->setChunkData(var1->xPosition, var1->yPosition, var1->zPosition, 
		                         var1->xSize, var1->ySize, var1->zSize, 
		                         var1->chunk.data());
	}
}

void NetClientHandler::handleBlockChange(Packet53BlockChange* var1)
{
	// Java: this.worldClient.func_714_c(var1.xPosition, var1.yPosition, var1.zPosition, var1.type, var1.metadata);
	if (worldClient != nullptr)
	{
		worldClient->doSetTileAndData(var1->xPosition, var1->yPosition, var1->zPosition, var1->type, var1->metadata);
	}
}

void NetClientHandler::handleKickDisconnect(Packet255KickDisconnect* var1)
{
	// Java: this.netManager.networkShutdown("Got kicked", new Object[0]);
	netManager->networkShutdown(u"Got kicked", {});
	
	// Java: this.disconnected = true;
	disconnected = true;
	
	// Java: this.mc.func_6261_a((World)null);
	mc->setLevel(nullptr);
	
	// Java: this.mc.displayGuiScreen(new GuiConnectFailed("Disconnected by server", var1.reason));
	jstring reasonStr = var1->reason;
	mc->setScreen(std::make_shared<GuiConnectFailed>(*mc, u"Disconnected by server", reasonStr));
}

void NetClientHandler::handleErrorMessage(const jstring& var1, const std::vector<jstring>& var2)
{
	// Java: if(!this.disconnected) {
	if (!disconnected)
	{
		// Java: this.disconnected = true;
		disconnected = true;
		
		// Java: this.mc.func_6261_a((World)null);
		mc->setLevel(nullptr);
		
		// Java: this.mc.displayGuiScreen(new GuiConnectFailed("Connection lost", var1));
		// Build error message from reason and args
		jstring errorMsg = var1;
		if (!var2.empty())
		{
			// Format message with args (simplified - Java uses String.format)
			std::string msgStr = String::toUTF8(var1);
			for (size_t i = 0; i < var2.size(); ++i)
			{
				std::string argStr = String::toUTF8(var2[i]);
				// Simple replacement - in Java this uses String.format
				size_t pos = msgStr.find("{}");
				if (pos != std::string::npos)
				{
					msgStr.replace(pos, 2, argStr);
				}
			}
			errorMsg = String::fromUTF8(msgStr);
		}
		mc->setScreen(std::make_shared<GuiConnectFailed>(*mc, u"Connection lost", errorMsg));
	}
}

void NetClientHandler::handleCollect(Packet22Collect* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleCollect() - handles item collection
	// Java: public void handleCollect(Packet22Collect var1) {
	//     Entity var2 = this.func_12246_a(var1.collectedEntityId);
	//     Object var3 = (EntityLiving)this.func_12246_a(var1.collectorEntityId);
	//     if(var3 == null) {
	//         var3 = this.mc.thePlayer;
	//     }
	//     if(var2 != null) {
	//         if(!this.worldClient.multiplayerWorld) {
	//             this.worldClient.playSoundAtEntity(var2, "random.pop", 0.2F, ((this.rand.nextFloat() - this.rand.nextFloat()) * 0.7F + 1.0F) * 2.0F);
	//         }
	//         this.mc.field_6321_h.func_1192_a(new EntityPickupFX(this.mc.theWorld, var2, (Entity)var3, -0.5F));
	//         this.worldClient.func_710_c(var1.collectedEntityId);
	//     }
	// }
	Entity* collected = func_12246_a(var1->collectedEntityId);
	Entity* collector = func_12246_a(var1->collectorEntityId);
	if (collector == nullptr && mc != nullptr && mc->player != nullptr)
	{
		collector = mc->player.get();
	}
	
	if (collected != nullptr && collector != nullptr && mc != nullptr && worldClient != nullptr)
	{
		// Java: if(!this.worldClient.multiplayerWorld) {
		//           this.worldClient.playSoundAtEntity(var2, "random.pop", 0.2F, ((this.rand.nextFloat() - this.rand.nextFloat()) * 0.7F + 1.0F) * 2.0F);
		//       }
		// Note: In Alpha 1.2.6, sound is only played in single-player (multiplayerWorld = false)
		// In multiplayer, the server sends sounds via Packet62Sound
		if (!worldClient->isOnline)
		{
			worldClient->playSound(collected, u"random.pop", 0.2f, ((rand.nextFloat() - rand.nextFloat()) * 0.7f + 1.0f) * 2.0f);
		}
		
		// Java: this.mc.field_6321_h.func_1192_a(new EntityPickupFX(this.mc.theWorld, var2, (Entity)var3, -0.5F));
		// Find the entities in the level's entities set to get shared_ptrs (needed to keep them alive for the particle)
		std::shared_ptr<Entity> collectedPtr = nullptr;
		for (const auto &e : worldClient->getAllEntities())
		{
			if (e.get() == collected)
			{
				collectedPtr = e;
				break;
			}
		}
		
		std::shared_ptr<Entity> collectorPtr = nullptr;
		if (collector == mc->player.get())
		{
			// For local player, use the player's shared_ptr
			collectorPtr = mc->player;
		}
		else
		{
			// For other entities, find in the level's entities set
			for (const auto &e : worldClient->getAllEntities())
			{
				if (e.get() == collector)
				{
					collectorPtr = e;
					break;
				}
			}
		}
		
		if (collectedPtr && collectorPtr)
		{
			mc->particleEngine.add(std::make_unique<TakeAnimationParticle>(*worldClient, collectedPtr, collectorPtr, -0.5f));
		}
		
		// Java: this.worldClient.func_710_c(var1.collectedEntityId);
		worldClient->removeEntity(var1->collectedEntityId);
	}
}

void NetClientHandler::handleChat(Packet3Chat* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleChat() - displays chat message
	// Java: public void handleChat(Packet3Chat var1) {
	//     this.mc.ingameGUI.getChatGUI().printChatMessage(var1.message);
	// }
	if (mc != nullptr)
	{
		mc->gui.getChatGUI().printChatMessage(var1->message);
	}
}

void NetClientHandler::handleArmAnimation(Packet18ArmAnimation* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleArmAnimation() - plays arm animation for entity
	// Java: public void handleArmAnimation(Packet18ArmAnimation var1) {
	//     Entity var2 = this.func_12246_a(var1.entityId);
	//     if(var2 != null) {
	//         if(var1.animate == 1) {
	//             EntityPlayer var3 = (EntityPlayer)var2;
	//             var3.func_457_w();
	//         } else if(var1.animate == 2) {
	//             var2.func_9280_g();
	//         }
	//     }
	// }
	Entity* entity = func_12246_a(var1->entityId);
	if (entity != nullptr)
	{
		if (var1->animate == 1)
		{
			// Java: EntityPlayer var3 = (EntityPlayer)var2; var3.func_457_w();
			// func_457_w() is the arm swing method
			Player* player = dynamic_cast<Player*>(entity);
			if (player != nullptr)
			{
				player->swing();  // Calls swing() which triggers arm animation
			}
		}
		else if (var1->animate == 2)
		{
			// Java: var2.func_9280_g(); (NetClientHandler.java:335)
			// Alpha 1.2.6: Trigger hurt animation (base Entity has empty implementation, subclasses may override)
			entity->func_9280_g();
		}
	}
}

void NetClientHandler::handleHandshake(Packet2Handshake* var1)
{
	// Java: if(var1.username.equals("-")) {
	//           this.addToSendQueue(new Packet1Login(this.mc.field_6320_i.inventory, 2000));
	//       } else {
	//           // Session authentication (skipped for now)
	//           this.addToSendQueue(new Packet1Login(this.mc.field_6320_i.inventory, 2000));
	//       }
	// For now, skip session auth and directly send login
	// Get username from mc->options.username, or use "-" as default (offline mode)
	jstring username = mc && !mc->options.username.empty() ? mc->options.username : u"-";
	addToSendQueue(new Packet1Login(username, 2000));
}

void NetClientHandler::flushOutput()
{
	// Flush the output stream to ensure queued packets are sent
	// This is used before disconnecting to ensure the disconnect packet is sent
	try
	{
		SocketOutputStream* out = NetworkManager::func_28140_f(netManager.get());
		if (out != nullptr)
		{
			out->flush();
		}
	}
	catch (...)
	{
		// Ignore flush errors - we're about to disconnect anyway
	}
}

void NetClientHandler::disconnect()
{
	// Alpha 1.2.6: queue Packet255KickDisconnect("Quitting") then stage close via
	// NetworkManager.func_28142_c() (ThreadCloseConnection delays the hard-close).
	// We send Packet255 directly via TCP to guarantee it's sent before closing.
	if (!disconnected)
	{
		// Send Packet255 directly via TCP (bypasses queue) - guarantees it's sent
		netManager->sendDisconnectPacketDirect(u"Quitting");
		
		// Also queue it (for compatibility, though we already sent it)
		func_28117_a(new Packet255KickDisconnect(u"Quitting"));
		disconnected = true;

		// Flush output to ensure everything is sent
		flushOutput();

		// Stage shutdown (do not hard-close immediately; give writer time to flush)
		netManager->wakeWriterThread();
	}
}

void NetClientHandler::handleMobSpawn(Packet24MobSpawn* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleMobSpawn() - creates mob entity
	// Java: double var2 = (double)var1.xPosition / 32.0D;
	//       double var4 = (double)var1.yPosition / 32.0D;
	//       double var6 = (double)var1.zPosition / 32.0D;
	//       float var8 = (float)(var1.yaw * 360) / 256.0F;
	//       float var9 = (float)(var1.pitch * 360) / 256.0F;
	//       EntityLiving var10 = (EntityLiving)EntityList.createEntity(var1.type, this.mc.theWorld);
	if (worldClient == nullptr)
		return;
	
	double x = static_cast<double>(var1->xPosition) / 32.0;
	double y = static_cast<double>(var1->yPosition) / 32.0;
	double z = static_cast<double>(var1->zPosition) / 32.0;
	float yaw = static_cast<float>(var1->yaw * 360) / 256.0f;
	float pitch = static_cast<float>(var1->pitch * 360) / 256.0f;
	
	// Java: EntityLiving var10 = (EntityLiving)EntityList.createEntity(var1.type, this.mc.theWorld);
	std::shared_ptr<Entity> entity = EntityIO::createEntity(var1->type, *worldClient);
	if (entity == nullptr)
		return;
	
	// Java: var10.field_9303_br = var1.xPosition;
	//       var10.field_9302_bs = var1.yPosition;
	//       var10.field_9301_bt = var1.zPosition;
	//       var10.field_620_ab = var1.entityId;
	// Alpha 1.2.6: Set encoded positions for network synchronization
	entity->field_9303_br = var1->xPosition;
	entity->field_9302_bs = var1->yPosition;
	entity->field_9301_bt = var1->zPosition;
	entity->entityId = var1->entityId;
	
	// Java: var10.setPositionAndRotation(var2, var4, var6, var8, var9);
	entity->moveTo(x, y, z, yaw, pitch);
	
	// Java: var10.field_9343_G = true;
	// Alpha 1.2.6: field_9343_G indicates entity is client-side (spawned from network)
	// When true, entity should NOT run AI - only interpolate positions
	Mob* mobEntity = dynamic_cast<Mob*>(entity.get());
	if (mobEntity != nullptr)
	{
		mobEntity->field_9343_G = true;
		mobEntity->interpolateOnly = true;  // Also set interpolateOnly for compatibility
	}
	
	// Java: this.worldClient.func_712_a(var1.entityId, var10);
	worldClient->putEntity(var1->entityId, entity);
	worldClient->addEntity(entity);  // CRITICAL: Must also call addEntity to add to world for rendering
	
	// Java: List var11 = var1.getMetadata();
	//       if(var11 != null) {
	//           var10.getDataWatcher().updateWatchedObjectsFromList(var11);
	//       }
	// Alpha 1.2.6: Apply metadata from packet to entity's DataWatcher
	if (!var1->receivedMetadata.empty())
	{
		entity->getDataWatcher().updateWatchedObjectsFromList(var1->receivedMetadata);
	}
}

void NetClientHandler::handleUpdateTime(Packet4UpdateTime* var1)
{
	// Java: this.mc.theWorld.setWorldTime(var1.time);
	if (mc != nullptr && mc->level != nullptr)
	{
		mc->level->time = var1->time;
	}
}

void NetClientHandler::handlePlayerInventory(Packet5PlayerInventory* var1)
{
	// Alpha 1.2.6: NetClientHandler.handlePlayerInventory() - updates entity equipment
	// Java: public void handlePlayerInventory(Packet5PlayerInventory var1) {
	//     Entity var2 = this.func_12246_a(var1.entityID);
	//     if(var2 != null) {
	//         var2.outfitWithItem(var1.slot, var1.itemID, var1.itemDamage);
	//     }
	// }
	Entity* entity = func_12246_a(var1->entityID);
	if (entity != nullptr)
	{
		// Java: var2.outfitWithItem(var1.slot, var1.itemID, var1.itemDamage);
		// Entity.outfitWithItem() is empty in base Entity, but EntityOtherPlayerMP overrides it
		// For other players, we need to call outfitWithItem() on EntityOtherPlayerMP
		EntityOtherPlayerMP* otherPlayer = dynamic_cast<EntityOtherPlayerMP*>(entity);
		if (otherPlayer != nullptr)
		{
			// Java: EntityOtherPlayerMP.outfitWithItem() sets inventory slot
			otherPlayer->outfitWithItem(var1->slot, var1->itemID, var1->itemDamage);
		}
		// For non-player entities, Entity.outfitWithItem() is empty (no-op) - matches Java
	}
}

void NetClientHandler::handleSpawnPosition(Packet6SpawnPosition* var1)
{
	// Java: this.worldClient.spawnX = var1.xPosition;
	//       this.worldClient.spawnY = var1.yPosition;
	//       this.worldClient.spawnZ = var1.zPosition;
	if (worldClient != nullptr)
	{
		worldClient->xSpawn = var1->xPosition;
		worldClient->ySpawn = var1->yPosition;
		worldClient->zSpawn = var1->zPosition;
		
		// Position player at spawn if player exists
		if (mc->player != nullptr)
		{
			mc->player->moveTo(var1->xPosition + 0.5, var1->yPosition + 1.0, var1->zPosition + 0.5, 0.0f, 0.0f);
			
			// Initialize position tracking so sendPlayerPosition() calculates correct deltas
			worldClient->initializePositionTracking();
		}
	}
}

void NetClientHandler::func_6497_a(Packet39* var1)
{
	// Beta 1.2: SetRidingPacket handling - matches newb12 ClientConnection.handleRidePacket() exactly
	// Beta: Entity rider = this.getEntity(packet.riderId);
	// Get rider entity - check if it's the local player first
	Entity* rider = nullptr;
	if (mc != nullptr && mc->player != nullptr && var1->riderId == mc->player->entityId)
	{
		// Beta: if (packet.riderId == this.minecraft.player.entityId) { rider = this.minecraft.player; }
		rider = mc->player.get();
	}
	else if (worldClient != nullptr)
	{
		// Beta: this.level.getEntity(packet.riderId)
		std::shared_ptr<Entity> riderPtr = worldClient->getEntity(var1->riderId);
		if (riderPtr != nullptr)
		{
			rider = riderPtr.get();
		}
	}
	
	// Beta 1.2: SetRidingPacket handling - matches newb12 ClientConnection.handleRidePacket() exactly
	// Beta: Entity ridden = this.getEntity(packet.riddenId);
	// Get ridden entity shared_ptr - can be null for dismount (riddenId = -1)
	std::shared_ptr<Entity> ridden = nullptr;
	if (var1->riddenId >= 0 && worldClient != nullptr)
	{
		// Beta: getEntity() checks if ID matches player first, then gets from world
		if (mc != nullptr && mc->player != nullptr && var1->riddenId == mc->player->entityId)
		{
			// Player is the ridden entity (shouldn't happen, but handle it)
			ridden = mc->player;
		}
		else
		{
			// Beta: this.level.getEntity(packet.riddenId)
			ridden = worldClient->getEntity(var1->riddenId);
		}
	}
	// If riddenId < 0 (e.g., -1), ridden stays nullptr (dismount)
	
	// Beta 1.2: if (rider != null) { rider.ride(ridden); }
	// Matches newb12 ClientConnection.handleRidePacket():472-474 exactly
	if (rider != nullptr)
	{
		// Beta: Use ride() with shared_ptr - matches newb12 Entity.ride() behavior
		// If already mounted to same vehicle, ride() will dismount (matches Java line 930-933)
		// If ridden is nullptr, ride(nullptr) dismounts (matches Java line 923-929)
		rider->ride(ridden);
	}
}

void NetClientHandler::func_6499_a(Packet7* var1)
{
	// Beta 1.2: InteractPacket handling - client-to-server only
	// Packet7 (InteractPacket) is sent FROM client TO server, so on the client side
	// this handler should never be called (server handles it)
	// This is a no-op on the client side, matching newb12 behavior
	// The client sends Packet7 via MultiplayerMode::interact() and MultiplayerMode::attack()
}

void NetClientHandler::func_9447_a(Packet38* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_9447_a() - handles entity status (Packet38)
	// Java: public void func_9447_a(Packet38 var1) {
	//     Entity var2 = this.func_12246_a(var1.field_9274_a);
	//     if(var2 != null) {
	//         var2.func_9282_a(var1.field_9273_b);
	//     }
	// }
	// Status values: 2 = entity hurt (triggers damage overlay), 3 = entity death
	Entity* entity = func_12246_a(var1->field_9274_a);
	if (entity != nullptr)
	{
		// Java: var2.func_9282_a(var1.field_9273_b);
		entity->func_9282_a(var1->field_9273_b);
	}
}

void NetClientHandler::handleHealth(Packet8* var1)
{
	// Java: this.mc.thePlayer.setHealth(var1.healthMP);
	// Alpha 1.2.6: EntityClientPlayerMP.setHealth() has special first-call behavior
	if (mc != nullptr && mc->player != nullptr)
	{
		EntityClientPlayerMP* clientPlayer = dynamic_cast<EntityClientPlayerMP*>(mc->player.get());
		if (clientPlayer != nullptr)
		{
			clientPlayer->setHealth(var1->healthMP);
		}
		else
		{
			// Fallback: directly set health if not EntityClientPlayerMP
			mc->player->health = var1->healthMP;
		}
	}
}

void NetClientHandler::func_9448_a(Packet9* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_9448_a() - handle respawn packet
	// Java: public void func_9448_a(Packet9 var1) {
	//     if(var1.field_28048_a == 1) {
	//         var1.field_28048_a = 0;
	//     }
	int_t dimension = var1->field_28048_a;
	if (dimension == 1)
	{
		dimension = 0;
	}
	
	// Java: String var2 = "Respawning";
	//       if(var1.field_28048_a == 0 && this.mc.thePlayer.dimension == -1) {
	//           var2 = "Leaving the Nether";
	//       } else if(var1.field_28048_a == -1 && this.mc.thePlayer.dimension == 0) {
	//           var2 = "Entering the Nether";
	//       }
	jstring respawnMessage = u"Respawning";
	if (mc != nullptr && mc->player != nullptr)
	{
		if (dimension == 0 && mc->player->dimension == -1)
		{
			respawnMessage = u"Leaving the Nether";
		}
		else if (dimension == -1 && mc->player->dimension == 0)
		{
			respawnMessage = u"Entering the Nether";
		}
	}
	
	// Java: if(var1.field_28048_a != this.mc.thePlayer.dimension) {
	//     this.field_1210_g = false;
	//     this.worldClient = new WorldClient(this, var1.seed, var1.field_28048_a);
	//     this.worldClient.multiplayerWorld = true;
	//     this.mc.func_6261_a(this.worldClient);
	//     this.mc.thePlayer.dimension = var1.field_28048_a;
	//     this.mc.displayGuiScreen(new GuiDownloadTerrain(this));
	// }
	// Alpha 1.2.6: Always respawn, but handle dimension switch first if needed
	// Java: this.mc.respawn(true, var1.field_28048_a, var2);
	if (mc != nullptr && mc->player != nullptr && dimension != mc->player->dimension)
	{
		field_1210_g = false;
		
		// Alpha 1.2.6: Store old level reference before switching
		std::shared_ptr<Level> oldLevel = mc->level;
		
		// Alpha 1.2.6: Mark old MultiPlayerLevel as invalid if it exists
		// This prevents crashes when tick() is called on a destroyed/moved level
		if (oldLevel != nullptr)
		{
			MultiPlayerLevel* oldMPLevel = dynamic_cast<MultiPlayerLevel*>(oldLevel.get());
			if (oldMPLevel != nullptr)
			{
				oldMPLevel->disconnect();  // This sets isValid = false
			}
			
			// Remove player from old level before switching dimensions
			if (mc->player != nullptr)
			{
				oldLevel->removeEntity(mc->player);
			}
		}
		
		// Alpha 1.2.6: Mark old worldClient as invalid before creating new one
		// This prevents crashes when tick() is called on a destroyed/moved level
		if (worldClient != nullptr)
		{
			worldClient->disconnect();  // This sets isValid = false
		}
		
		// Alpha 1.2.6: Mark old worldClient as invalid before creating new one
		// This prevents crashes when tick() is called on a destroyed/moved level
		if (worldClient != nullptr)
		{
			worldClient->disconnect();  // This sets isValid = false
		}
		
		// Alpha 1.2.6: Create new world client for new dimension
		// Note: The old worldClient will be cleaned up when mc->setLevel() replaces the old level
		// The shared_ptr in mc->level will handle cleanup of the old level
		// setLevel() will also call levelRenderer.setLevel() which cleans up old chunks
		worldClient = new MultiPlayerLevel(this, var1->seed, dimension);
		mc->setLevel(std::shared_ptr<Level>(worldClient));
		mc->player->dimension = dimension;
		mc->setScreen(std::make_shared<GuiDownloadTerrain>(*mc, this));
		
		// Alpha 1.2.6: Explicitly clear old level reference to ensure it's cleaned up immediately
		// This helps prevent any lingering references from causing crashes
		oldLevel.reset();
	}
	
	// Alpha 1.2.6: Respawn player (in new dimension if switched, or same dimension if not)
	// Java: this.mc.respawn(true, var1.field_28048_a, var2);
	// respawnPlayer() will:
	// 1. Remove old player entity (if not already removed)
	// 2. Create new player via gameMode->createPlayer() (creates EntityClientPlayerMP)
	// 3. Reset player position
	// 4. Clear death screen
	// Note: respawnPlayer() now has null checks for level and dimension
	if (mc != nullptr && mc->level != nullptr && mc->level->dimension != nullptr)
	{
		mc->respawnPlayer();
	}
}

void NetClientHandler::func_12245_a(Packet60* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_12245_a() - handles explosion
	// Java: public void func_12245_a(Packet60 var1) {
	//     Explosion var2 = new Explosion(this.mc.theWorld, (Entity)null, var1.field_12236_a, var1.field_12235_b, var1.field_12239_c, var1.field_12238_d);
	//     var2.field_12251_g = var1.field_12237_e;
	//     var2.func_12247_b();
	// }
	if (worldClient == nullptr)
		return;
	
	// Java: Explosion var2 = new Explosion(this.mc.theWorld, (Entity)null, ...);
	Explosion explosion(*worldClient, nullptr, var1->field_12236_a, var1->field_12235_b, var1->field_12239_c, var1->field_12238_d);
	
	// Java: var2.field_12251_g = var1.field_12237_e;
	// Convert ChunkCoordinates to TilePos
	for (const auto& coord : var1->field_12237_e)
	{
		explosion.toBlow.insert(TilePos(coord.x, coord.y, coord.z));
	}
	
	// Java: var2.func_12247_b(); (explode() method)
	explosion.explode();
	explosion.addParticles();
}

void NetClientHandler::func_20087_a(Packet100OpenWindow* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_20087_a() - opens inventory window
	// Java: public void func_20087_a(Packet100OpenWindow var1) {
	//     if(var1.inventoryType == 0) {
	//         InventoryBasic var2 = new InventoryBasic(var1.windowTitle, var1.slotsCount);
	//         this.mc.thePlayer.displayGUIChest(var2);
	//         this.mc.thePlayer.craftingInventory.windowId = var1.windowId;
	//     } else if(var1.inventoryType == 2) {
	//         TileEntityFurnace var3 = new TileEntityFurnace();
	//         this.mc.thePlayer.displayGUIFurnace(var3);
	//         this.mc.thePlayer.craftingInventory.windowId = var1.windowId;
	//     } else if(var1.inventoryType == 1) {
	//         EntityPlayerSP var4 = this.mc.thePlayer;
	//         this.mc.thePlayer.displayWorkbenchGUI(MathHelper.floor_double(var4.posX), MathHelper.floor_double(var4.posY), MathHelper.floor_double(var4.posZ));
	//         this.mc.thePlayer.craftingInventory.windowId = var1.windowId;
	//     }
	// }
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	LocalPlayer* localPlayer = dynamic_cast<LocalPlayer*>(mc->player.get());
	if (localPlayer == nullptr)
		return;
	
	if (var1->inventoryType == 0)
	{
		// Chest inventory
		// Java: InventoryBasic var2 = new InventoryBasic(var1.windowTitle, var1.slotsCount);
		// Java: this.mc.thePlayer.displayGUIChest(var2);
		auto inventory = std::make_shared<InventoryBasic>(var1->windowTitle, var1->slotsCount);
		
		// Alpha 1.2.6: Use InventoryBasic directly for multiplayer chests (supports both single and double chests)
		// Create ChestScreen with InventoryBasic (slotsCount can be 27 for single or 54 for double)
		// Java: this.mc.thePlayer.displayGUIChest(var2) creates GuiChest(IInventory playerInventory, IInventory chestInventory)
		// We create ChestScreen directly with the InventoryBasic
		auto chestScreen = Util::make_shared<ChestScreen>(*mc, inventory);
		chestScreen->windowId = var1->windowId;
		mc->setScreen(chestScreen);
	}
	else if (var1->inventoryType == 2)
	{
		// Furnace inventory
		// Java: TileEntityFurnace var3 = new TileEntityFurnace();
		auto furnaceEntity = std::make_shared<FurnaceTileEntity>();
		
		// Java: this.mc.thePlayer.displayGUIFurnace(var3);
		localPlayer->openFurnace(furnaceEntity);
		
		// Set windowId on the screen
		FurnaceScreen* furnaceScreen = dynamic_cast<FurnaceScreen*>(mc->screen.get());
		if (furnaceScreen != nullptr)
		{
			furnaceScreen->windowId = var1->windowId;
		}
	}
	else if (var1->inventoryType == 1)
	{
		// Workbench inventory
		// Java: EntityPlayerSP var4 = this.mc.thePlayer;
		//       this.mc.thePlayer.displayWorkbenchGUI(MathHelper.floor_double(var4.posX), MathHelper.floor_double(var4.posY), MathHelper.floor_double(var4.posZ));
		int_t x = static_cast<int_t>(std::floor(localPlayer->x));
		int_t y = static_cast<int_t>(std::floor(localPlayer->y));
		int_t z = static_cast<int_t>(std::floor(localPlayer->z));
		localPlayer->startCrafting(x, y, z);
		
		// Set windowId on the screen
		WorkbenchScreen* workbenchScreen = dynamic_cast<WorkbenchScreen*>(mc->screen.get());
		if (workbenchScreen != nullptr)
		{
			workbenchScreen->windowId = var1->windowId;
		}
	}
}

void NetClientHandler::func_20091_a(Packet102WindowClick* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_20091_a() - Packet102WindowClick is client-to-server only
	// This method should never be called on the client (packet is only sent, never received)
	// Java: NetClientHandler doesn't override func_20091_a, so it uses NetHandler default (no-op)
	// We implement it here to satisfy the virtual override requirement
}

void NetClientHandler::func_20088_a(Packet103SetSlot* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_20088_a() - updates inventory slot
	// Java: public void func_20088_a(Packet103SetSlot var1) {
	//     if(var1.windowId == -1) {
	//         this.mc.thePlayer.inventory.setItemStack(var1.myItemStack);
	//     } else if(var1.windowId == 0 && var1.itemSlot >= 36 && var1.itemSlot < 45) {
	//         ItemStack var2 = this.mc.thePlayer.inventorySlots.getSlot(var1.itemSlot).getStack();
	//         if(var1.myItemStack != null && (var2 == null || var2.stackSize < var1.myItemStack.stackSize)) {
	//             var1.myItemStack.animationsToGo = 5;
	//         }
	//         this.mc.thePlayer.inventorySlots.putStackInSlot(var1.itemSlot, var1.myItemStack);
	//     } else if(var1.windowId == this.mc.thePlayer.craftingInventory.windowId) {
	//         this.mc.thePlayer.craftingInventory.putStackInSlot(var1.itemSlot, var1.myItemStack);
	//     }
	// }
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	if (var1->windowId == -1)
	{
		// Java: this.mc.thePlayer.inventory.setItemStack(var1.myItemStack);
		if (var1->myItemStack != nullptr)
		{
			mc->player->inventory.setCarried(*var1->myItemStack);
		}
		else
		{
			mc->player->inventory.setCarriedNull();
		}
	}
	else if (var1->windowId == 0 && var1->itemSlot >= 36 && var1->itemSlot < 45)
	{
		// Java: ItemStack var2 = this.mc.thePlayer.inventorySlots.getSlot(var1.itemSlot).getStack();
		// Container slot 36-44 maps to mainInventory[0-8] (hotbar)
		int_t hotbarSlot = var1->itemSlot - 36;
		ItemStack* var2 = (hotbarSlot >= 0 && hotbarSlot < 9 && !mc->player->inventory.mainInventory[hotbarSlot].isEmpty()) 
			? &mc->player->inventory.mainInventory[hotbarSlot] 
			: nullptr;
		
		// Java: if(var1.myItemStack != null && (var2 == null || var2.stackSize < var1.myItemStack.stackSize)) {
		//     var1.myItemStack.animationsToGo = 5;
		// }
		// Beta: animationsToGo is popTime in our implementation
		if (var1->myItemStack != nullptr && (var2 == nullptr || var2->stackSize < var1->myItemStack->stackSize))
		{
			var1->myItemStack->popTime = 5;  // Beta: animationsToGo = 5 (Java uses animationsToGo, we use popTime)
		}
		
		// Java: this.mc.thePlayer.inventorySlots.putStackInSlot(var1.itemSlot, var1.myItemStack);
		// Container slot 36 = mainInventory[0], 37 = mainInventory[1], ..., 44 = mainInventory[8]
		if (var1->myItemStack != nullptr)
		{
			mc->player->inventory.mainInventory[hotbarSlot] = *var1->myItemStack;
		}
		else
		{
			mc->player->inventory.mainInventory[hotbarSlot] = ItemStack();
		}
	}
	else if (mc->screen != nullptr)
	{
		// Java: else if(var1.windowId == this.mc.thePlayer.craftingInventory.windowId) {
		//     this.mc.thePlayer.craftingInventory.putStackInSlot(var1.itemSlot, var1.myItemStack);
		// }
		// craftingInventory is ContainerCrafting (workbench) - check if current screen is WorkbenchScreen
		WorkbenchScreen* workbenchScreen = dynamic_cast<WorkbenchScreen*>(mc->screen.get());
		if (workbenchScreen != nullptr && workbenchScreen->windowId == var1->windowId)
		{
			// Java: ContainerWorkbench slot mapping:
			// Slot 0: Result slot
			// Slots 1-9: Crafting grid (3x3)
			if (var1->itemSlot == 0)
			{
				// Result slot
				if (var1->myItemStack != nullptr)
				{
					workbenchScreen->setResultSlot(*var1->myItemStack);
				}
				else
				{
					workbenchScreen->setResultSlot(ItemStack());
				}
			}
			else if (var1->itemSlot >= 1 && var1->itemSlot <= 9)
			{
				// Crafting grid slots (container slots 1-9 map to craftSlots[0-8])
				int_t craftIndex = var1->itemSlot - 1;  // Map 1->0, 2->1, ..., 9->8
				if (var1->myItemStack != nullptr)
				{
					workbenchScreen->setCraftSlot(craftIndex, *var1->myItemStack);
				}
				else
				{
					workbenchScreen->setCraftSlot(craftIndex, ItemStack());
				}
			}
		}
		
		// C++-specific: Handle containers (chest, furnace) for real-time updates
		// Java doesn't handle containers in Packet103SetSlot, but C++ needs it for real-time updates
		FurnaceScreen* furnaceScreen = dynamic_cast<FurnaceScreen*>(mc->screen.get());
		if (furnaceScreen != nullptr && furnaceScreen->windowId == var1->windowId)
		{
			// Furnace slots: 0 = input, 1 = fuel, 2 = output
			if (var1->itemSlot >= 0 && var1->itemSlot < 3)
			{
				std::shared_ptr<ItemStack> stackPtr = (var1->myItemStack != nullptr) ? 
					std::make_shared<ItemStack>(*var1->myItemStack) : nullptr;
				furnaceScreen->setFurnaceSlot(var1->itemSlot, stackPtr);
			}
		}
		
		ChestScreen* chestScreen = dynamic_cast<ChestScreen*>(mc->screen.get());
		if (chestScreen != nullptr && chestScreen->windowId == var1->windowId)
		{
			// Chest slots: 0 to (chestSize - 1)
			int_t chestSize = chestScreen->getChestSize();
			if (var1->itemSlot >= 0 && var1->itemSlot < chestSize)
			{
				std::shared_ptr<ItemStack> stackPtr = (var1->myItemStack != nullptr) ? 
					std::make_shared<ItemStack>(*var1->myItemStack) : nullptr;
				chestScreen->setChestSlot(var1->itemSlot, stackPtr);
			}
		}
	}
}

void NetClientHandler::func_20089_a(Packet106Transaction* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_20089_a() - handles transaction confirmations
	// Java: public void func_20089_a(Packet106Transaction var1) {
	//     Container var2 = null;
	//     if(var1.windowId == 0) {
	//         var2 = this.mc.thePlayer.inventorySlots;
	//     } else if(var1.windowId == this.mc.thePlayer.craftingInventory.windowId) {
	//         var2 = this.mc.thePlayer.craftingInventory;
	//     }
	//     if(var2 != null) {
	//         if(var1.field_20030_c) {
	//             var2.func_20113_a(var1.field_20028_b);
	//         } else {
	//             var2.func_20110_b(var1.field_20028_b);
	//             this.addToSendQueue(new Packet106Transaction(var1.windowId, var1.field_20028_b, true));
	//         }
	//     }
	// }
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	// Java: Find container based on windowId
	// windowId == 0: Player inventory (InventoryScreen)
	// windowId == craftingInventory.windowId: Workbench/Chest/Furnace
	bool foundContainer = false;
	
	if (var1->windowId == 0)
	{
		// Java: var2 = this.mc.thePlayer.inventorySlots;
		// Player inventory - InventoryScreen handles this
		foundContainer = true;
		// Note: func_20113_a/func_20110_b are empty in Container.java, so no-op
		// But we still need to handle the accepted/rejected logic
		if (var1->field_20030_c)
		{
			// Java: var2.func_20113_a(var1.field_20028_b); - accept transaction (no-op in base Container)
			// Transaction accepted - no action needed
		}
		else
		{
			// Java: var2.func_20110_b(var1.field_20028_b); - reject transaction (no-op in base Container)
			// Java: this.addToSendQueue(new Packet106Transaction(var1.windowId, var1.field_20028_b, true));
			// Transaction rejected - resend with accepted=true
			auto response = new Packet106Transaction();
			response->windowId = var1->windowId;
			response->field_20028_b = var1->field_20028_b;
			response->field_20030_c = true;
			addToSendQueue(response);
		}
	}
	else if (mc->screen != nullptr)
	{
		// Java: else if(var1.windowId == this.mc.thePlayer.craftingInventory.windowId) {
		//     var2 = this.mc.thePlayer.craftingInventory;
		// }
		// Check if current screen matches windowId (workbench, chest, furnace)
		WorkbenchScreen* workbenchScreen = dynamic_cast<WorkbenchScreen*>(mc->screen.get());
		ChestScreen* chestScreen = dynamic_cast<ChestScreen*>(mc->screen.get());
		FurnaceScreen* furnaceScreen = dynamic_cast<FurnaceScreen*>(mc->screen.get());
		
		if ((workbenchScreen != nullptr && workbenchScreen->windowId == var1->windowId) ||
		    (chestScreen != nullptr && chestScreen->windowId == var1->windowId) ||
		    (furnaceScreen != nullptr && furnaceScreen->windowId == var1->windowId))
		{
			foundContainer = true;
			// Java: var2.func_20113_a(var1.field_20028_b) or var2.func_20110_b(var1.field_20028_b)
			// Note: func_20113_a/func_20110_b are empty in Container.java, so no-op
			if (var1->field_20030_c)
			{
				// Transaction accepted - no action needed
			}
			else
			{
				// Transaction rejected - resend with accepted=true
				auto response = new Packet106Transaction();
				response->windowId = var1->windowId;
				response->field_20028_b = var1->field_20028_b;
				response->field_20030_c = true;
				addToSendQueue(response);
			}
		}
	}
	
	// If container not found, ignore transaction (matches Java behavior when var2 == null)
}

void NetClientHandler::func_20094_a(Packet104WindowItems* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_20094_a() - bulk updates inventory slots
	// Java: public void func_20094_a(Packet104WindowItems var1) {
	//     if(var1.windowId == 0) {
	//         this.mc.thePlayer.inventorySlots.putStacksInSlots(var1.itemStack);
	//     } else if(var1.windowId == this.mc.thePlayer.craftingInventory.windowId) {
	//         this.mc.thePlayer.craftingInventory.putStacksInSlots(var1.itemStack);
	//     }
	// }
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	// Debug: Log when we receive inventory updates
	
	if (var1->windowId == 0)
	{
		// Player inventory (ContainerPlayer has 45 slots total)
		// Java: this.mc.thePlayer.inventorySlots.putStacksInSlots(var1.itemStack);
		// ContainerPlayer slot layout:
		// Slot 0: Crafting result (ignored - client calculates)
		// Slots 1-4: Crafting matrix (2x2) - stored in InventoryScreen craftSlots
		// Slots 5-8: Armor slots - map to armorInventory[3], armorInventory[2], armorInventory[1], armorInventory[0]
		// Slots 9-35: Main inventory (rows 1-3) - map to mainInventory[9-35]
		// Slots 36-44: Hotbar - map to mainInventory[0-8]
		
		for (size_t i = 0; i < var1->itemStack.size(); ++i)
		{
			if (i == 0)
			{
				// Slot 0: Crafting result - ignore (client calculates)
				continue;
			}
			else if (i >= 1 && i <= 4)
			{
				// Slots 1-4: Crafting matrix (2x2 grid)
				// These are stored in InventoryScreen when it's open
				// Update InventoryScreen craftSlots if inventory screen is open
				InventoryScreen* invScreen = dynamic_cast<InventoryScreen*>(mc->screen.get());
				if (invScreen != nullptr)
				{
					int_t craftIndex = static_cast<int_t>(i) - 1;  // Map 1->0, 2->1, 3->2, 4->3
					if (var1->itemStack[i] != nullptr)
					{
						invScreen->setCraftSlot(craftIndex, *var1->itemStack[i]);
					}
					else
					{
						invScreen->setCraftSlot(craftIndex, ItemStack());
					}
				}
			}
			else if (i >= 5 && i <= 8)
			{
				// Slots 5-8: Armor slots
				// Container slot 5 = armorInventory[3] (boots)
				// Container slot 6 = armorInventory[2] (leggings)
				// Container slot 7 = armorInventory[1] (chestplate)
				// Container slot 8 = armorInventory[0] (helmet)
				int_t armorIndex = 8 - static_cast<int_t>(i);  // Maps 5->3, 6->2, 7->1, 8->0
				if (var1->itemStack[i] != nullptr)
				{
					mc->player->inventory.armorInventory[armorIndex] = *var1->itemStack[i];
				}
				else
				{
					mc->player->inventory.armorInventory[armorIndex] = ItemStack();
				}
			}
			else if (i >= 9 && i <= 35)
			{
				// Slots 9-35: Main inventory (rows 1-3)
				// These map directly to mainInventory[9-35]
				int_t invSlot = static_cast<int_t>(i);
				if (var1->itemStack[i] != nullptr)
				{
					mc->player->inventory.mainInventory[invSlot] = *var1->itemStack[i];
				}
				else
				{
					mc->player->inventory.mainInventory[invSlot] = ItemStack();
				}
			}
			else if (i >= 36 && i <= 44)
			{
				// Slots 36-44: Hotbar
				// Container slot 36 = mainInventory[0]
				// Container slot 37 = mainInventory[1]
				// ...
				// Container slot 44 = mainInventory[8]
				int_t hotbarSlot = static_cast<int_t>(i) - 36;
				if (var1->itemStack[i] != nullptr)
				{
					mc->player->inventory.mainInventory[hotbarSlot] = *var1->itemStack[i];
				}
				else
				{
					mc->player->inventory.mainInventory[hotbarSlot] = ItemStack();
				}
			}
		}
	}
	else
	{
		// Container slots (chest, furnace, workbench)
		// Check current screen and update slots if windowId matches
		if (mc->screen != nullptr)
		{
			ChestScreen* chestScreen = dynamic_cast<ChestScreen*>(mc->screen.get());
			if (chestScreen != nullptr && chestScreen->windowId == var1->windowId)
			{
				// Packet104WindowItems includes all slots: chest slots + player inventory slots
				// First slots are chest slots, then player inventory slots
				int_t chestSize = chestScreen->getChestSize();
				
				for (size_t i = 0; i < var1->itemStack.size(); ++i)
				{
					if (i < static_cast<size_t>(chestSize))
					{
						// Chest slots (0 to chestSize-1)
						std::shared_ptr<ItemStack> stackPtr = (var1->itemStack[i] != nullptr) ? 
							std::make_shared<ItemStack>(*var1->itemStack[i]) : nullptr;
						chestScreen->setChestSlot(static_cast<int_t>(i), stackPtr);
					}
					else
					{
						// Player inventory slots in chest window (chestSize to chestSize+35)
						// ContainerChest slot mapping (ContainerChest.java:14-28):
						// Slots chestSize to chestSize+26: Player main inventory slots 9-35
						// Slots chestSize+27 to chestSize+35: Player hotbar slots 0-8
						int_t containerSlot = static_cast<int_t>(i);
						if (containerSlot >= chestSize && containerSlot < chestSize + 27)
						{
							// Player main inventory slots 9-35
							// Container slot chestSize = player slot 9, chestSize+1 = player slot 10, etc.
							int_t playerSlot = 9 + (containerSlot - chestSize);
							if (playerSlot >= 9 && playerSlot <= 35)
							{
								if (var1->itemStack[i] != nullptr)
								{
									mc->player->inventory.mainInventory[playerSlot] = *var1->itemStack[i];
								}
								else
								{
									mc->player->inventory.mainInventory[playerSlot] = ItemStack();
								}
							}
						}
						else if (containerSlot >= chestSize + 27 && containerSlot < chestSize + 36)
						{
							// Player hotbar slots 0-8
							// Container slot chestSize+27 = player slot 0, chestSize+28 = player slot 1, etc.
							int_t hotbarSlot = containerSlot - (chestSize + 27);
							if (hotbarSlot >= 0 && hotbarSlot < 9)
							{
								if (var1->itemStack[i] != nullptr)
								{
									mc->player->inventory.mainInventory[hotbarSlot] = *var1->itemStack[i];
								}
								else
								{
									mc->player->inventory.mainInventory[hotbarSlot] = ItemStack();
								}
							}
						}
					}
				}
			}
			
			FurnaceScreen* furnaceScreen = dynamic_cast<FurnaceScreen*>(mc->screen.get());
			if (furnaceScreen != nullptr && furnaceScreen->windowId == var1->windowId)
			{
				// Packet104WindowItems includes all slots: furnace slots (0-2) + player inventory slots
				for (size_t i = 0; i < var1->itemStack.size(); ++i)
				{
					if (i < 3)
					{
						// Furnace slots (0 = input, 1 = fuel, 2 = output)
						std::shared_ptr<ItemStack> stackPtr = (var1->itemStack[i] != nullptr) ? 
							std::make_shared<ItemStack>(*var1->itemStack[i]) : nullptr;
						furnaceScreen->setFurnaceSlot(static_cast<int_t>(i), stackPtr);
					}
					else
					{
						// Player inventory slots in furnace window (3+)
						// ContainerFurnace slot mapping (ContainerFurnace.java:15-24):
						// Slots 0-2: Furnace slots (input, fuel, output)
						// Slots 3-29: Player main inventory slots 9-35
						// Slots 30-38: Player hotbar slots 0-8
						int_t containerSlot = static_cast<int_t>(i);
						if (containerSlot >= 3 && containerSlot < 30)
						{
							// Player main inventory slots 9-35
							// Container slot 3 = player slot 9, container slot 4 = player slot 10, etc.
							int_t playerSlot = 9 + (containerSlot - 3);
							if (playerSlot >= 9 && playerSlot <= 35)
							{
								if (var1->itemStack[i] != nullptr)
								{
									mc->player->inventory.mainInventory[playerSlot] = *var1->itemStack[i];
								}
								else
								{
									mc->player->inventory.mainInventory[playerSlot] = ItemStack();
								}
							}
						}
						else if (containerSlot >= 30 && containerSlot < 39)
						{
							// Player hotbar slots 0-8
							// Container slot 30 = player slot 0, container slot 31 = player slot 1, etc.
							int_t hotbarSlot = containerSlot - 30;
							if (hotbarSlot >= 0 && hotbarSlot < 9)
							{
								if (var1->itemStack[i] != nullptr)
								{
									mc->player->inventory.mainInventory[hotbarSlot] = *var1->itemStack[i];
								}
								else
								{
									mc->player->inventory.mainInventory[hotbarSlot] = ItemStack();
								}
							}
						}
					}
				}
			}
			
			WorkbenchScreen* workbenchScreen = dynamic_cast<WorkbenchScreen*>(mc->screen.get());
			if (workbenchScreen != nullptr && workbenchScreen->windowId == var1->windowId)
			{
				// Java: this.mc.thePlayer.craftingInventory.putStacksInSlots(var1.itemStack);
				// Java: ContainerWorkbench slot mapping:
				// Slot 0: Result (SlotResult)
				// Slots 1-9: Crafting grid (i1 + l * 3 where i1=col, l=row)
				// Packet104WindowItems includes all slots: result (0), crafting (1-9), player inventory (10+)
				for (size_t i = 0; i < var1->itemStack.size(); ++i)
				{
					if (i == 0)
					{
						// Result slot (container slot 0)
						if (var1->itemStack[i] != nullptr)
						{
							workbenchScreen->setResultSlot(*var1->itemStack[i]);
						}
						else
						{
							workbenchScreen->setResultSlot(ItemStack());
						}
					}
					else if (i >= 1 && i <= 9)
					{
						// Crafting grid slots (container slots 1-9 map to craftSlots[0-8])
						int_t craftIndex = static_cast<int_t>(i) - 1;  // Map 1->0, 2->1, ..., 9->8
						if (var1->itemStack[i] != nullptr)
						{
							workbenchScreen->setCraftSlot(craftIndex, *var1->itemStack[i]);
						}
						else
						{
							workbenchScreen->setCraftSlot(craftIndex, ItemStack());
						}
					}
					else
					{
						// Player inventory slots in workbench window (10+)
						// ContainerWorkbench slot mapping (ContainerWorkbench.java:26-34):
						// Slot 0: Result slot
						// Slots 1-9: Crafting grid (3x3)
						// Slots 10-36: Player main inventory slots 9-35
						// Slots 37-45: Player hotbar slots 0-8
						int_t containerSlot = static_cast<int_t>(i);
						if (containerSlot >= 10 && containerSlot < 37)
						{
							// Player main inventory slots 9-35
							// Container slot 10 = player slot 9, container slot 11 = player slot 10, etc.
							int_t playerSlot = 9 + (containerSlot - 10);
							if (playerSlot >= 9 && playerSlot <= 35)
							{
								if (var1->itemStack[i] != nullptr)
								{
									mc->player->inventory.mainInventory[playerSlot] = *var1->itemStack[i];
								}
								else
								{
									mc->player->inventory.mainInventory[playerSlot] = ItemStack();
								}
							}
						}
						else if (containerSlot >= 37 && containerSlot < 46)
						{
							// Player hotbar slots 0-8
							// Container slot 37 = player slot 0, container slot 38 = player slot 1, etc.
							int_t hotbarSlot = containerSlot - 37;
							if (hotbarSlot >= 0 && hotbarSlot < 9)
							{
								if (var1->itemStack[i] != nullptr)
								{
									mc->player->inventory.mainInventory[hotbarSlot] = *var1->itemStack[i];
								}
								else
								{
									mc->player->inventory.mainInventory[hotbarSlot] = ItemStack();
								}
							}
						}
					}
				}
			}
		}
	}
}

void NetClientHandler::handleSignUpdate(Packet130UpdateSign* var1)
{
	// Alpha 1.2.6: NetClientHandler.handleSignUpdate() (NetClientHandler.java:528-542)
	// Java: public void handleSignUpdate(Packet130UpdateSign var1) {
	//     if(this.mc.theWorld.blockExists(var1.xPosition, var1.yPosition, var1.zPosition)) {
	//         TileEntity var2 = this.mc.theWorld.getBlockTileEntity(var1.xPosition, var1.yPosition, var1.zPosition);
	//         if(var2 instanceof TileEntitySign) {
	//             TileEntitySign var3 = (TileEntitySign)var2;
	//             for(int var4 = 0; var4 < 4; ++var4) {
	//                 var3.signText[var4] = var1.signLines[var4];
	//             }
	//             var3.onInventoryChanged();
	//         }
	//     }
	// }
	// Performance optimization: Only update and invalidate if text actually changed
	// This prevents redundant texture rebakes when servers resend identical sign text
	if (worldClient != nullptr && worldClient->hasChunkAt(var1->xPosition, var1->yPosition, var1->zPosition))
	{
		// Alpha: Get tile entity at position
		std::shared_ptr<TileEntity> tileEntity = worldClient->getTileEntity(var1->xPosition, var1->yPosition, var1->zPosition);
		if (tileEntity != nullptr)
		{
			SignTileEntity* signEntity = dynamic_cast<SignTileEntity*>(tileEntity.get());
			if (signEntity != nullptr)
			{
				// Performance: Check if any line actually changed before invalidating
				bool textChanged = false;
				for (int_t i = 0; i < 4; ++i)
				{
					if (signEntity->messages[i] != var1->signLines[i])
					{
						textChanged = true;
						break;
					}
				}
				
				// Only update and invalidate if text actually changed
				if (textChanged)
				{
					// Alpha: Copy sign text from packet to tile entity
					for (int_t i = 0; i < 4; ++i)
					{
						signEntity->messages[i] = var1->signLines[i];
					}
					// Performance: Invalidate caches after updating messages
					signEntity->invalidateWidthCache();
					signEntity->invalidateTextDisplayList();  // This invalidates baked texture
				}
			}
		}
	}
}

void NetClientHandler::func_20092_a(Packet101CloseWindow* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_20092_a() - closes inventory window
	// Java: public void func_20092_a(Packet101CloseWindow var1) {
	//     this.mc.thePlayer.closeScreen();
	// }
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	// Java: this.mc.thePlayer.closeScreen();
	// In C++, closeScreen() is closeContainer() which closes the current screen
	mc->player->closeContainer();
	
	// Also close screen directly
	mc->setScreen(nullptr);
}

void NetClientHandler::func_28115_a(Packet61DoorChange* var1)
{
	// Alpha 1.2.6: NetClientHandler.func_28115_a() - handles door/effect changes
	// Java: public void func_28115_a(Packet61DoorChange var1) {
	//     this.mc.theWorld.func_28106_e(var1.field_28050_a, var1.field_28053_c, var1.field_28052_d, var1.field_28051_e, var1.field_28049_b);
	// }
	// Java: World.func_28106_e() calls func_28107_a() which notifies all IWorldAccess listeners
	// Java: RenderGlobal.func_28136_a() handles the actual effects (RenderGlobal.java:1151-1210)
	// Effect IDs:
	// 1000: Click sound (1.0F pitch)
	// 1001: Click sound (1.2F pitch)
	// 1002: Bow sound
	// 1003: Door open/close sound
	// 1004: Fizz sound
	// 1005: Record playing
	// 2000: Smoke particles
	// 2001: Block break particles
	
	if (mc == nullptr || mc->level == nullptr)
		return;
	
	int_t effectID = var1->field_28050_a;  // var2 in Java
	int_t x = var1->field_28053_c;  // var3 in Java
	int_t y = var1->field_28052_d;  // var4 in Java
	int_t z = var1->field_28051_e;  // var5 in Java
	int_t data = var1->field_28049_b;  // var6 in Java
	
	// Java: RenderGlobal.func_28136_a() switch statement (lines 1154-1208)
	switch (effectID)
	{
		case 1000:  // Java: case 1000 (line 1155)
			// Java: this.worldObj.playSoundEffect((double)var3, (double)var4, (double)var5, "random.click", 1.0F, 1.0F); (line 1156)
			mc->soundEngine.play(u"random.click", static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 1.0f, 1.0f);
			break;
			
		case 1001:  // Java: case 1001 (line 1158)
			// Java: this.worldObj.playSoundEffect((double)var3, (double)var4, (double)var5, "random.click", 1.0F, 1.2F); (line 1159)
			mc->soundEngine.play(u"random.click", static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 1.0f, 1.2f);
			break;
			
		case 1002:  // Java: case 1002 (line 1161)
			// Java: this.worldObj.playSoundEffect((double)var3, (double)var4, (double)var5, "random.bow", 1.0F, 1.2F); (line 1162)
			mc->soundEngine.play(u"random.bow", static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 1.0f, 1.2f);
			break;
			
		case 1003:  // Java: case 1003 (line 1164)
			// Java: Door open/close sound (lines 1165-1169)
			// if(Math.random() < 0.5D) {
			//     this.worldObj.playSoundEffect((double)var3 + 0.5D, (double)var4 + 0.5D, (double)var5 + 0.5D, "random.door_open", 1.0F, this.worldObj.rand.nextFloat() * 0.1F + 0.9F);
			// } else {
			//     this.worldObj.playSoundEffect((double)var3 + 0.5D, (double)var4 + 0.5D, (double)var5 + 0.5D, "random.door_close", 1.0F, this.worldObj.rand.nextFloat() * 0.1F + 0.9F);
			// }
			// Use Level's random generator for consistency (Java: this.worldObj.rand)
			if (mc->level->random.nextFloat() < 0.5f)
			{
				float pitch = mc->level->random.nextFloat() * 0.1f + 0.9f;
				mc->soundEngine.play(u"random.door_open", static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f, 1.0f, pitch);
			}
			else
			{
				float pitch = mc->level->random.nextFloat() * 0.1f + 0.9f;
				mc->soundEngine.play(u"random.door_close", static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f, 1.0f, pitch);
			}
			break;
			
		case 1004:  // Java: case 1004 (line 1171)
			// Java: this.worldObj.playSoundEffect((double)((float)var3 + 0.5F), (double)((float)var4 + 0.5F), (double)((float)var5 + 0.5F), "random.fizz", 0.5F, 2.6F + (var7.nextFloat() - var7.nextFloat()) * 0.8F); (line 1172)
			// Use Level's random generator (Java: var7 = this.worldObj.rand)
			{
				float pitch = 2.6f + (mc->level->random.nextFloat() - mc->level->random.nextFloat()) * 0.8f;
				mc->soundEngine.play(u"random.fizz", static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f, 0.5f, pitch);
			}
			break;
			
		case 1005:  // Java: case 1005 (line 1174)
			// Java: Record playing (lines 1175-1179)
			// if(Item.itemsList[var6] instanceof ItemRecord) {
			//     this.worldObj.playRecord(((ItemRecord)Item.itemsList[var6]).recordName, var3, var4, var5);
			// } else {
			//     this.worldObj.playRecord((String)null, var3, var4, var5);
			// }
			// var6 is the item shiftedIndex (data field in packet)
			// Beta: If var6 is 0 or invalid, stop the record (data == 0 means no record in jukebox)
			{
				int_t shiftedIndex = data;  // Java: var6
				if (shiftedIndex == 0 || shiftedIndex < 256 || shiftedIndex >= static_cast<int_t>(Item::itemsList.size()))
				{
					// Beta: Stop playing record (data == 0 means no record, or invalid shiftedIndex)
					// Java: this.worldObj.playRecord((String)null, var3, var4, var5);
					mc->level->playStreamingMusic(jstring(), x, y, z);
				}
				else if (Item::itemsList[shiftedIndex] != nullptr)
				{
					Item* item = Item::itemsList[shiftedIndex];
					ItemRecord* record = dynamic_cast<ItemRecord*>(item);
					if (record != nullptr)
					{
						// Java: this.worldObj.playRecord(((ItemRecord)Item.itemsList[var6]).recordName, var3, var4, var5);
						// Use ItemRecord::getRecordingName() to get the record name
						mc->level->playStreamingMusic(record->getRecordingName(), x, y, z);
					}
					else
					{
						// Java: this.worldObj.playRecord((String)null, var3, var4, var5);
						// Stop playing record (not a record item) - pass empty string
						mc->level->playStreamingMusic(jstring(), x, y, z);
					}
				}
				else
				{
					// Invalid item (null pointer) - stop record
					mc->level->playStreamingMusic(jstring(), x, y, z);
				}
			}
			break;
			
		case 2000:  // Java: case 2000 (line 1181)
			// Java: Smoke particles (lines 1182-1199)
			// Smoke direction is encoded in data: var6 % 3 - 1 for x, var6 / 3 % 3 - 1 for z
			{
				// Java: int var9 = var6 % 3 - 1; (line 1182)
				// Java: int var10 = var6 / 3 % 3 - 1; (line 1183)
				int_t var9 = data % 3 - 1;  // x direction: -1, 0, or 1
				int_t var10 = (data / 3) % 3 - 1;  // z direction: -1, 0, or 1
				
				// Java: double var11 = (double)var3 + (double)var9 * 0.6D + 0.5D; (line 1184)
				// Java: double var13 = (double)var4 + 0.5D; (line 1185)
				// Java: double var15 = (double)var5 + (double)var10 * 0.6D + 0.5D; (line 1186)
				double baseX = static_cast<double>(x) + static_cast<double>(var9) * 0.6 + 0.5;
				double baseY = static_cast<double>(y) + 0.5;
				double baseZ = static_cast<double>(z) + static_cast<double>(var10) * 0.6 + 0.5;
				
				// Java: for(var8 = 0; var8 < 10; ++var8) { (line 1188)
				// Java: Spawn 10 smoke particles (lines 1189-1196)
				for (int_t i = 0; i < 10; ++i)
				{
					// Java: double var31 = var7.nextDouble() * 0.2D + 0.01D; (line 1189)
					double var31 = mc->level->random.nextDouble() * 0.2 + 0.01;
					
					// Java: double var19 = var11 + (double)var9 * 0.01D + (var7.nextDouble() - 0.5D) * (double)var10 * 0.5D; (line 1190)
					// Java: double var21 = var13 + (var7.nextDouble() - 0.5D) * 0.5D; (line 1191)
					// Java: double var23 = var15 + (double)var10 * 0.01D + (var7.nextDouble() - 0.5D) * (double)var9 * 0.5D; (line 1192)
					double px = baseX + static_cast<double>(var9) * 0.01 + (mc->level->random.nextDouble() - 0.5) * static_cast<double>(var10) * 0.5;
					double py = baseY + (mc->level->random.nextDouble() - 0.5) * 0.5;
					double pz = baseZ + static_cast<double>(var10) * 0.01 + (mc->level->random.nextDouble() - 0.5) * static_cast<double>(var9) * 0.5;
					
					// Java: double var25 = (double)var9 * var31 + var7.nextGaussian() * 0.01D; (line 1193)
					// Java: double var27 = -0.03D + var7.nextGaussian() * 0.01D; (line 1194)
					// Java: double var29 = (double)var10 * var31 + var7.nextGaussian() * 0.01D; (line 1195)
					// Note: Random doesn't have nextGaussian, approximate using Box-Muller transform
					// For simplicity, use nextDouble() - 0.5 scaled appropriately to approximate Gaussian noise
					double gaussian1 = (mc->level->random.nextDouble() - 0.5) * (mc->level->random.nextDouble() - 0.5) * 2.0;
					double gaussian2 = (mc->level->random.nextDouble() - 0.5) * (mc->level->random.nextDouble() - 0.5) * 2.0;
					double gaussian3 = (mc->level->random.nextDouble() - 0.5) * (mc->level->random.nextDouble() - 0.5) * 2.0;
					double vx = static_cast<double>(var9) * var31 + gaussian1 * 0.01;
					double vy = -0.03 + gaussian2 * 0.01;
					double vz = static_cast<double>(var10) * var31 + gaussian3 * 0.01;
					
					// Java: this.spawnParticle("smoke", var19, var21, var23, var25, var27, var29); (line 1196)
					auto smoke = std::make_unique<SmokeParticle>(*mc->level, px, py, pz, vx, vy, vz);
					mc->particleEngine.add(std::move(smoke));
				}
			}
			break;
			
		case 2001:  // Java: case 2001 (line 1200)
			// Java: Block break particles (lines 1201-1207)
			// var8 = var6 & 255; (block ID) (line 1201)
			{
				int_t blockID = data & 255;  // Java: var8 = var6 & 255
				
				// Java: if(var8 > 0) { (line 1202)
				if (blockID > 0 && blockID < 256 && Tile::tiles[blockID] != nullptr)
				{
					// Java: Block var17 = Block.blocksList[var8]; (line 1203)
					Tile* tile = Tile::tiles[blockID];
					
				// Java: this.mc.sndManager.playSound(var17.stepSound.func_1146_a(), (float)var3 + 0.5F, (float)var4 + 0.5F, (float)var5 + 0.5F, (var17.stepSound.func_1147_b() + 1.0F) / 2.0F, var17.stepSound.func_1144_c() * 0.8F); (line 1204)
				// Play block break sound
				// Java: func_1146_a() = getBreakSound(), func_1147_b() = getVolume(), func_1144_c() = getPitch()
				const Tile::SoundType* soundType = tile->getSoundType();
				if (soundType != nullptr)
				{
					jstring breakSound = soundType->getBreakSound();  // Java: func_1146_a() -> getBreakSound()
					float volume = (soundType->getVolume() + 1.0f) / 2.0f;  // Java: func_1147_b() -> getVolume()
					float pitch = soundType->getPitch() * 0.8f;  // Java: func_1144_c() -> getPitch()
					mc->soundEngine.play(breakSound, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f, volume, pitch);
				}
				}
				
				// Java: this.mc.field_6321_h.func_1186_a(var3, var4, var5, var6 & 255, var6 >> 8 & 255); (line 1207)
				// func_1186_a spawns block break particles - this is ParticleEngine.destroy() in our codebase
				mc->particleEngine.destroy(x, y, z);
			}
			break;
	}
}

void NetClientHandler::handle62Sound(Packet62Sound* var1)
{
	// Java: if(this.mc.gameSettings.accept62) {
	//           this.mc.field_6323_f.playSound(var1.sound, var1.locX, var1.locY, var1.locZ, var1.f, var1.f1);
	//       }
	// Note: accept62 is a debug option in Alpha 1.2.6 (set via J key), but we'll always play sounds
	// field_6323_f is the SoundManager (soundEngine in our C++ code)
	if (mc != nullptr)
	{
		// Beta: SoundEngine.play(String name, float x, float y, float z, float volume, float pitch)
		mc->soundEngine.play(var1->sound, static_cast<float>(var1->locX), static_cast<float>(var1->locY), static_cast<float>(var1->locZ), var1->f, var1->f1);
	}
}

void NetClientHandler::handle63Digging(Packet63Digging* var1)
{
	// Alpha 1.2.6: NetClientHandler.handle63Digging() - shows block breaking particles and stores progress
	// Java: public void handle63Digging(Packet63Digging var1) {
	//     this.mc.field_6321_h.func_1191_a(var1.x, var1.y, var1.z, var1.face);
	//     ChunkCoordinates var2 = new ChunkCoordinates(var1.x, var1.y, var1.z);
	//     if(this.mc.field_9243_r.mpDigging.containsKey(var2)) {
	//         this.mc.field_9243_r.mpDigging.remove(var2, var1);
	//     }
	//     if(var1.progress != 0.0F) {
	//         this.mc.field_9243_r.mpDigging.put(var2, var1);
	//     }
	// }
	// Note: field_6321_h is EffectRenderer (particleEngine), func_1191_a shows block breaking particles
	// field_9243_r is EntityRenderer (gameRenderer in C++), mpDigging is a map for rendering block breaking progress
	if (mc == nullptr)
		return;
	
	// Java: this.mc.field_6321_h.func_1191_a(var1.x, var1.y, var1.z, var1.face);
	// EffectRenderer.func_1191_a() shows block breaking particles
	mc->particleEngine.crack(var1->x, var1->y, var1->z, static_cast<int_t>(var1->face));
	
	// Java: ChunkCoordinates var2 = new ChunkCoordinates(var1.x, var1.y, var1.z);
	ChunkCoordinates coord(var1->x, var1->y, var1->z);
	
	// Java: if(this.mc.field_9243_r.mpDigging.containsKey(var2)) {
	//     this.mc.field_9243_r.mpDigging.remove(var2, var1);
	// }
	auto it = mc->gameRenderer.mpDigging.find(coord);
	if (it != mc->gameRenderer.mpDigging.end())
	{
		// Note: In Java, HashMap.remove(key) removes the entry
		// We need to delete the old Packet63Digging pointer before removing
		delete it->second;
		mc->gameRenderer.mpDigging.erase(it);
	}
	
	// Java: if(var1.progress != 0.0F) {
	//     this.mc.field_9243_r.mpDigging.put(var2, var1);
	// }
	if (var1->progress != 0.0f)
	{
		// Store a copy of the packet for rendering progress overlay
		// Note: We need to store a pointer because Packet63Digging isn't copyable easily
		// The packet will be deleted when removed from map or when handler is destroyed
		Packet63Digging* packetCopy = new Packet63Digging();
		packetCopy->x = var1->x;
		packetCopy->y = var1->y;
		packetCopy->z = var1->z;
		packetCopy->face = var1->face;
		packetCopy->progress = var1->progress;
		packetCopy->timestamp = var1->timestamp;
		mc->gameRenderer.mpDigging[coord] = packetCopy;
	}
}

void NetClientHandler::func_28117_a(Packet* var1)
{
	// Java: if(!this.disconnected) {
	if (!disconnected)
	{
		// Java: this.netManager.addToSendQueue(var1);
		netManager->addToSendQueue(var1);
		
		// Java: this.netManager.func_28142_c();
		netManager->wakeWriterThread();
	}
}

void NetClientHandler::addToSendQueue(Packet* var1)
{
	// Java: if(!this.disconnected) {
	if (!disconnected)
	{
		// Java: this.netManager.addToSendQueue(var1);
		netManager->addToSendQueue(var1);
	}
}

void NetClientHandler::func_4114_b(Packet* var1)
{
	// Java: Default handler that does nothing
	// In Java Alpha 1.2.6, Packet0KeepAlive.processPacket is empty, so this default handler
	// is called and does nothing. Keep-alive packets are sent proactively by GuiDownloadTerrain,
	// not as responses to server keep-alive packets.
}

// Alpha 1.2.6: NetClientHandler.func_20090_a() - handles Packet105UpdateProgressbar
// Java: public void func_20090_a(Packet105UpdateProgressbar var1) {
//     this.func_4114_b(var1);
//     if(this.mc.thePlayer.craftingInventory != null && this.mc.thePlayer.craftingInventory.windowId == var1.windowId) {
//         this.mc.thePlayer.craftingInventory.func_20112_a(var1.progressBar, var1.progressBarValue);
//     }
// }
void NetClientHandler::func_20090_a(Packet105UpdateProgressbar* var1)
{
	if (mc == nullptr || mc->player == nullptr)
		return;
	
	// Java: this.func_4114_b(var1); (line 545) - default handler (no-op)
	
	// Java: if(this.mc.thePlayer.craftingInventory != null && this.mc.thePlayer.craftingInventory.windowId == var1.windowId) (line 546)
	// In Java, craftingInventory is the Container (ContainerFurnace, ContainerChest, etc.)
	// Since we don't have a Container system on the client, we update the FurnaceTileEntity directly through the FurnaceScreen
	FurnaceScreen* furnaceScreen = dynamic_cast<FurnaceScreen*>(mc->screen.get());
	if (furnaceScreen != nullptr && furnaceScreen->windowId == var1->windowId)
	{
		// Java: this.mc.thePlayer.craftingInventory.func_20112_a(var1.progressBar, var1.progressBarValue); (line 547)
		// ContainerFurnace.func_20112_a() maps progressBar to furnace fields:
		// progressBar 0 → furnaceCookTime (tickCount)
		// progressBar 1 → furnaceBurnTime (litTime)
		// progressBar 2 → currentItemBurnTime (litDuration)
		std::shared_ptr<FurnaceTileEntity> furnace = furnaceScreen->getFurnace();
		if (furnace != nullptr)
		{
			furnace->updateProgressBar(var1->progressBar, var1->progressBarValue);
		}
	}
}

Entity* NetClientHandler::func_12246_a(int entityId)
{
	// Java: return (Entity)(var1 == this.mc.thePlayer.field_620_ab ? this.mc.thePlayer : this.worldClient.func_709_b(var1));
	if (mc != nullptr && mc->player != nullptr && mc->player->entityId == entityId)
	{
		return mc->player.get();
	}
	
	if (worldClient != nullptr)
	{
		std::shared_ptr<Entity> entity = worldClient->getEntity(entityId);
		if (entity != nullptr)
		{
			return entity.get();
		}
	}
	
	return nullptr;
}
