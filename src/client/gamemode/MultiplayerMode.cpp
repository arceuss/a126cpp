#include "client/gamemode/MultiplayerMode.h"
#include "client/gui/InventoryScreen.h"
#include "client/gui/WorkbenchScreen.h"
#include <unordered_map>
#include "network/NetClientHandler.h"
#include "network/Packet14BlockDig.h"
#include "network/Packet15Place.h"
#include "network/Packet16BlockItemSwitch.h"
#include "network/Packet7.h"
#include "network/Packet102WindowClick.h"
#include "network/Packet18ArmAnimation.h"
#include "client/player/EntityClientPlayerMP.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "Facing.h"
#include "client/Minecraft.h"
#include "world/entity/Entity.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "util/Memory.h"
#include <cmath>

MultiplayerMode::MultiplayerMode(Minecraft &minecraft, NetClientHandler* netHandler)
	: GameMode(minecraft)
	, netHandler(netHandler)
{
}

// Alpha 1.2.6: PlayerControllerMP.func_4087_b - creates EntityClientPlayerMP
// Java: return new EntityClientPlayerMP(this.mc, var1, this.mc.field_6320_i, this.field_9438_k);
std::shared_ptr<Player> MultiplayerMode::createPlayer(Level &level)
{
	return Util::make_shared<EntityClientPlayerMP>(minecraft, level, minecraft.user.get(), netHandler, level.dimension->id);
}

void MultiplayerMode::initPlayer(std::shared_ptr<Player> player)
{
	// Alpha 1.2.6: PlayerControllerMP.flipPlayer() - set rotation to -180 (north)
	player->yRot = -180.0f;
	
	// Alpha 1.2.6: Set userType to 100 to allow block breaking
	// In Alpha 1.2.6, userType >= 100 is required for block breaking (Minecraft.java:755)
	player->userType = 100;
}

void MultiplayerMode::startDestroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	// Alpha 1.2.6: PlayerControllerMP.clickBlock() - start breaking block
	clickBlock(x, y, z, face);
}

bool MultiplayerMode::destroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	// Alpha 1.2.6: PlayerControllerMP.sendBlockRemoved() - send block removal packet
	// Java: public boolean sendBlockRemoved(int var1, int var2, int var3, int var4) {
	//     int var5 = this.mc.theWorld.getBlockId(var1, var2, var3);
	//     boolean var6 = super.sendBlockRemoved(var1, var2, var3, var4);
	Level &level = *minecraft.level;
	int_t blockId = level.getTile(x, y, z);
	
	// Java: boolean var6 = super.sendBlockRemoved(var1, var2, var3, var4);
	// Call base implementation to actually remove the block (client-side prediction)
	bool changed = GameMode::destroyBlock(x, y, z, face);
	
	// Java: ItemStack var7 = this.mc.thePlayer.getCurrentEquippedItem();
	//       if(var7 != null) {
	//           var7.hitBlock(var5, var1, var2, var3);
	//           if(var7.stackSize == 0) {
	//               var7.func_1097_a(this.mc.thePlayer);
	//               this.mc.thePlayer.destroyCurrentEquippedItem();
	//           }
	//       }
	// Alpha 1.2.6: Handle item durability (PlayerControllerMP.java:29-36)
	ItemStack *held = minecraft.player->inventory.getCurrentItem();
	if (held != nullptr && changed)
	{
		held->hitBlock(blockId, x, y, z);
		if (held->isEmpty())
		{
			// Java: this.mc.thePlayer.destroyCurrentEquippedItem();
			// Clear inventory slot when item breaks
			minecraft.player->inventory.mainInventory[minecraft.player->inventory.currentItem] = ItemStack();
		}
	}
	
	// Note: Packet14BlockDig(2) is sent in continueDestroyBlock when damage >= 1.0
	// For instantly breakable blocks called from clickBlock, we rely on the server
	// to send Packet53BlockChange to confirm the removal
	
	return changed;
}

void MultiplayerMode::continueDestroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	// Alpha 1.2.6: PlayerControllerMP.sendBlockRemoving() - continue breaking block
	if (isHittingBlock)
	{
		syncCurrentPlayItem();
		
		if (blockHitDelay > 0)
		{
			--blockHitDelay;
		}
		else if (x == currentBlockX && y == currentBlockY && z == currentBlockZ)
		{
			Level &level = *minecraft.level;
			int_t blockId = level.getTile(x, y, z);
			if (blockId == 0)
			{
				isHittingBlock = false;
				return;
			}
			
			Tile *tile = Tile::tiles[blockId];
			if (tile != nullptr)
			{
				curBlockDamageMP += tile->getDestroyProgress(*minecraft.player);
				
				// Alpha 1.2.6: Play mining sound every 4 ticks (PlayerControllerMP.java:83-85)
				if (std::fmod(field_9441_h, 4.0f) == 0.0f)
				{
					const Tile::SoundType *soundType = tile->getSoundType();
					if (soundType != nullptr)
					{
						jstring stepSound = soundType->getStepSound();
						float volume = (soundType->getVolume() + 1.0f) / 8.0f;
						float pitch = soundType->getPitch() * 0.5f;
						minecraft.soundEngine.play(stepSound, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, volume, pitch);
					}
				}
				
				++field_9441_h;
				
				if (curBlockDamageMP >= 1.0f)
				{
					isHittingBlock = false;
					// Alpha 1.2.6: Send block dig packet with status 2 (finished)
					// Use the stored face from when we started breaking, not the current hit result face
					netHandler->addToSendQueue(new Packet14BlockDig(2, x, y, z, static_cast<int>(currentBlockFace)));
					destroyBlock(x, y, z, currentBlockFace);
					curBlockDamageMP = 0.0f;
					prevBlockDamageMP = 0.0f;
					field_9441_h = 0.0f;
					blockHitDelay = 5;
					currentBlockFace = Facing::NONE;
				}
			}
		}
		else
		{
			clickBlock(x, y, z, face);
		}
	}
}

void MultiplayerMode::stopDestroyBlock()
{
	// Alpha 1.2.6: PlayerControllerMP.func_6468_a() - stop breaking block
	// Java: public void func_6468_a() {
	//     this.curBlockDamageMP = 0.0F;
	//     this.isHittingBlock = false;
	// }
	// Note: Java doesn't send a packet here - just resets state
	// The server will cancel breaking when it doesn't receive continue packets
	curBlockDamageMP = 0.0f;
	isHittingBlock = false;
}

void MultiplayerMode::render(float a)
{
	// Alpha 1.2.6: PlayerControllerMP.func_6467_a() - update destroy progress for rendering
	if (curBlockDamageMP <= 0.0f)
	{
		minecraft.gui.progress = 0.0f;
		minecraft.levelRenderer.destroyProgress = 0.0f;
	}
	else
	{
		float dp = prevBlockDamageMP + (curBlockDamageMP - prevBlockDamageMP) * a;
		minecraft.gui.progress = dp;
		minecraft.levelRenderer.destroyProgress = dp;
	}
}

float MultiplayerMode::getPickRange()
{
	// Alpha 1.2.6: PlayerControllerMP.getBlockReachDistance() - return 4.0
	return 4.0f;
}

bool MultiplayerMode::useItem(std::shared_ptr<Player> &player, std::shared_ptr<Level> &level, ItemStack *item)
{
	// Alpha 1.2.6: PlayerControllerMP.sendUseItem() - send use item packet
	syncCurrentPlayItem();
	
	// Alpha 1.2.6: Packet15Place(-1, -1, -1, 255, itemStack) for item use
	std::unique_ptr<ItemStack> itemCopy = item != nullptr ? std::make_unique<ItemStack>(*item) : nullptr;
	netHandler->addToSendQueue(new Packet15Place(-1, -1, -1, 255, std::move(itemCopy)));
	
	// Call base implementation to actually use the item
	return GameMode::useItem(player, level, item);
}

bool MultiplayerMode::useItemOn(std::shared_ptr<Player> &player, std::shared_ptr<Level> &level, ItemStack *item, int_t x, int_t y, int_t z, Facing face)
{
	// Alpha 1.2.6: PlayerControllerMP.sendPlaceBlock() - send place block packet
	syncCurrentPlayItem();
	
	// Alpha 1.2.6: Packet15Place(x, y, z, face, itemStack) for block placement
	std::unique_ptr<ItemStack> itemCopy = item != nullptr ? std::make_unique<ItemStack>(*item) : nullptr;
	netHandler->addToSendQueue(new Packet15Place(x, y, z, static_cast<int>(face), std::move(itemCopy)));
	
	// Call base implementation to actually place the block
	return GameMode::useItemOn(player, level, item, x, y, z, face);
}

void MultiplayerMode::interact(std::shared_ptr<Player> &player, std::shared_ptr<Entity> &entity)
{
	// Alpha 1.2.6: PlayerControllerMP.func_6475_a() - interact with entity
	syncCurrentPlayItem();
	
	// Alpha 1.2.6: Packet7(player.entityId, entity.entityId, 0) for interaction
	netHandler->addToSendQueue(new Packet7(player->entityId, entity->entityId, 0));
	
	// Call base implementation
	GameMode::interact(player, entity);
}

void MultiplayerMode::attack(std::shared_ptr<Player> &player, std::shared_ptr<Entity> &entity)
{
	// Alpha 1.2.6: PlayerControllerMP.func_6472_b() - attack entity
	syncCurrentPlayItem();
	
	// Alpha 1.2.6: Packet7(player.entityId, entity.entityId, 1) for attack
	netHandler->addToSendQueue(new Packet7(player->entityId, entity->entityId, 1));
	
	// Call base implementation
	GameMode::attack(player, entity);
}

void MultiplayerMode::tick()
{
	// Alpha 1.2.6: PlayerControllerMP.func_6474_c() - update destroy progress
	syncCurrentPlayItem();
	prevBlockDamageMP = curBlockDamageMP;
	// Note: Sound stopping (func_4033_c) is handled elsewhere
}

void MultiplayerMode::initLevel(std::shared_ptr<Level> level)
{
	// Alpha 1.2.6: PlayerControllerMP.func_717_a() - initialize level
	GameMode::initLevel(level);
}

ItemStack MultiplayerMode::handleWindowClick(int windowId, int slot, int mouseClick, bool shiftClick, std::shared_ptr<Player> player, std::unique_ptr<ItemStack> itemBefore)
{
	// Alpha 1.2.6: PlayerControllerMP.func_27174_a() - handle window click
	// Java: short var6 = var5.craftingInventory.func_20111_a(var5.inventory);
	//       ItemStack var7 = super.func_27174_a(var1, var2, var3, var4, var5);
	//       this.field_9438_k.addToSendQueue(new Packet102WindowClick(var1, var2, var3, var4, var7, var6));
	
	// Get action counter - increment per window (Container.field_20917_a)
	// For now, we'll use a simple incrementing counter per windowId
	static std::unordered_map<int_t, short_t> actionCounters;
	short_t &counter = actionCounters[windowId];
	counter++;
	short_t action = counter;
	
	// Get the item that's currently in the slot BEFORE the click (if not provided)
	// For ContainerPlayer (windowId 0), we need to map the slot correctly
	if (itemBefore == nullptr)
	{
		if (windowId == 0)
		{
			// ContainerPlayer slot mapping:
			// Slot 0: Result (from InventoryScreen.resultSlot)
			// Slots 1-4: Crafting matrix (from InventoryScreen.craftSlots)
			// Slots 5-8: Armor (from player->inventory.armorInventory)
			// Slots 9-35: Main inventory (from player->inventory.mainInventory[9-35])
			// Slots 36-44: Hotbar (from player->inventory.mainInventory[0-8])
			
			if (slot == 0)
			{
				// Result slot - get from InventoryScreen if open
				InventoryScreen* invScreen = dynamic_cast<InventoryScreen*>(minecraft.screen.get());
				if (invScreen != nullptr)
				{
					const ItemStack& result = invScreen->getResultSlot();
					if (!result.isEmpty())
					{
						itemBefore = std::make_unique<ItemStack>(result);
					}
				}
			}
			else if (slot >= 1 && slot <= 4)
			{
				// Crafting matrix slots 1-4 -> craftSlots[0-3]
				InventoryScreen* invScreen = dynamic_cast<InventoryScreen*>(minecraft.screen.get());
				if (invScreen != nullptr)
				{
					int_t craftIndex = slot - 1;  // Map 1->0, 2->1, 3->2, 4->3
					const ItemStack& craftSlot = invScreen->getCraftSlot(craftIndex);
					if (!craftSlot.isEmpty())
					{
						itemBefore = std::make_unique<ItemStack>(craftSlot);
					}
				}
			}
			else if (slot >= 5 && slot <= 8)
			{
				// Armor slots 5-8 -> armorInventory[3,2,1,0]
				int_t armorIndex = 8 - slot;  // Maps 5->3, 6->2, 7->1, 8->0
				if (!player->inventory.armorInventory[armorIndex].isEmpty())
				{
					itemBefore = std::make_unique<ItemStack>(player->inventory.armorInventory[armorIndex]);
				}
			}
			else if (slot >= 9 && slot <= 35)
			{
				// Main inventory slots 9-35
				// Capture the item state - send nullptr if empty (matches Java behavior)
				const ItemStack& slotStack = player->inventory.mainInventory[slot];
				if (!slotStack.isEmpty())
				{
					itemBefore = std::make_unique<ItemStack>(slotStack);
				}
			}
			else if (slot >= 36 && slot <= 44)
			{
				// Hotbar slots 36-44 -> mainInventory[0-8]
				int_t hotbarSlot = slot - 36;
				const ItemStack& slotStack = player->inventory.mainInventory[hotbarSlot];
				if (!slotStack.isEmpty())
				{
					itemBefore = std::make_unique<ItemStack>(slotStack);
				}
			}
		}
		else
		{
			// Other windows (workbench, chest, furnace) - get from the screen
			WorkbenchScreen* workbenchScreen = dynamic_cast<WorkbenchScreen*>(minecraft.screen.get());
			if (workbenchScreen != nullptr && workbenchScreen->windowId == windowId)
			{
				if (slot == 0)
				{
					// Result slot
					const ItemStack& result = workbenchScreen->getResultSlot();
					if (!result.isEmpty())
					{
						itemBefore = std::make_unique<ItemStack>(result);
					}
				}
				else if (slot >= 1 && slot <= 9)
				{
					// Crafting grid slots 1-9 -> craftSlots[0-8]
					int_t craftIndex = slot - 1;
					const ItemStack& craftSlot = workbenchScreen->getCraftSlot(craftIndex);
					if (!craftSlot.isEmpty())
					{
						itemBefore = std::make_unique<ItemStack>(craftSlot);
					}
				}
			}
			
			// If we couldn't get from screen, try to get from slot if it's a player inventory slot
			// (containers include player inventory slots after their own slots)
			if (itemBefore == nullptr)
			{
				// For workbench: slots 10-36 = main inventory (9-35), slots 37-45 = hotbar (0-8)
				if (slot >= 10 && slot <= 36)
				{
					int_t playerSlot = 9 + (slot - 10);  // Map 10->9, 11->10, ..., 36->35
					if (playerSlot >= 9 && playerSlot <= 35 && !player->inventory.mainInventory[playerSlot].isEmpty())
					{
						itemBefore = std::make_unique<ItemStack>(player->inventory.mainInventory[playerSlot]);
					}
				}
				else if (slot >= 37 && slot <= 45)
				{
					int_t playerSlot = slot - 37;  // Map 37->0, 38->1, ..., 45->8
					if (playerSlot >= 0 && playerSlot < 9 && !player->inventory.mainInventory[playerSlot].isEmpty())
					{
						itemBefore = std::make_unique<ItemStack>(player->inventory.mainInventory[playerSlot]);
					}
				}
			}
		}
	}
	
	// Send the packet (itemBefore is what's in the slot before the click)
	// If itemBefore is still nullptr, create an empty one (slot was empty before click)
	netHandler->addToSendQueue(new Packet102WindowClick(windowId, slot, mouseClick, shiftClick, std::move(itemBefore), action));
	
	// Return empty stack - server will send back the result via Packet103SetSlot/Packet104WindowItems
	return ItemStack();
}

void MultiplayerMode::syncCurrentPlayItem()
{
	// Alpha 1.2.6: PlayerControllerMP.syncCurrentPlayItem() - sync current item to server
	int_t currentItem = minecraft.player->inventory.currentItem;
	if (currentItem != field_1075_l)
	{
		field_1075_l = currentItem;
		netHandler->addToSendQueue(new Packet16BlockItemSwitch(field_1075_l));
	}
}

void MultiplayerMode::clickBlock(int_t x, int_t y, int_t z, Facing face)
{
	// Alpha 1.2.6: PlayerControllerMP.clickBlock() - start clicking on block
	// NOTE: Arm swing is called BEFORE clickBlock in Minecraft.java (line 702)
	// So we should NOT call func_457_w() here - it's already called in handleMouseClick
	if (!isHittingBlock || x != currentBlockX || y != currentBlockY || z != currentBlockZ)
	{
		// Alpha 1.2.6: Send block dig packet with status 0 (start)
		if (netHandler == nullptr)
		{
			return;
		}
		if (netHandler->isDisconnected())
		{
			return;
		}
		netHandler->addToSendQueue(new Packet14BlockDig(0, x, y, z, static_cast<int>(face)));
		
		Level &level = *minecraft.level;
		int_t blockId = level.getTile(x, y, z);
		
		if (blockId > 0 && curBlockDamageMP == 0.0f)
		{
			Tile *tile = Tile::tiles[blockId];
			if (tile != nullptr)
			{
				tile->attack(level, x, y, z, *minecraft.player);
			}
		}
		
		if (blockId > 0)
		{
			Tile *tile = Tile::tiles[blockId];
			if (tile != nullptr && tile->getDestroyProgress(*minecraft.player) >= 1.0f)
			{
				// Block can be instantly destroyed
				// Alpha 1.2.6: Send status 2 (finished) before removing block
				// Java: In sendBlockRemoving, sends Packet14BlockDig(2) then sendBlockRemoved
				netHandler->addToSendQueue(new Packet14BlockDig(2, x, y, z, static_cast<int>(face)));
				destroyBlock(x, y, z, face);
			}
			else
			{
				// Start breaking animation
				isHittingBlock = true;
				currentBlockX = x;
				currentBlockY = y;
				currentBlockZ = z;
				currentBlockFace = face;  // Alpha 1.2.6: Store face when starting to break
				curBlockDamageMP = 0.0f;
				prevBlockDamageMP = 0.0f;
				field_9441_h = 0.0f;
			}
		}
	}
}
