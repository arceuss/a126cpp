#include "world/entity/item/EntityItem.h"
#include "world/entity/player/Player.h"
#include "world/entity/player/InventoryPlayer.h"
#include "world/item/ItemStack.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/material/Material.h"
#include "java/Math.h"
#include <iostream>

// Ensure Player is fully defined for playerTouch override

// Alpha 1.2.6: EntityItem constructor for EntityIO (takes only Level)
// Used when creating from EntityList.createEntity() - position and stack should be set via load() or separately
EntityItem::EntityItem(Level &level)
	: Entity(level)
{
	// Default empty ItemStack
	this->item = ItemStack();
	setSize(0.25f, 0.25f);  // Alpha: setSize(0.25F, 0.25F) (EntityItem.java:13)
	this->heightOffset = this->bbHeight / 2.0f;  // Alpha: yOffset = height / 2.0F (EntityItem.java:14)
	
	// Position will be set via load() or setPos() after construction
	setPos(0.0, 0.0, 0.0);
	
	// Alpha: Random rotation and initial velocity (EntityItem.java:17-20)
	this->yRot = (float)(Math::random() * 360.0);
	this->xd = 0.0;
	this->yd = 0.2;  // Alpha: motionY = 0.2F
	this->zd = 0.0;
	
	this->bobOffs = (float)(Math::random() * 3.14159265358979323846 * 2.0);  // Beta: bobOffs (ItemEntity.java:19) - Math.PI * 2.0
	
	// Beta: throwTime is set after construction, but we set it in constructor for convenience (ItemEntity.java:17)
	this->throwTime = 10;  // Beta: throwTime = 10 (default delay before pickup)
}

EntityItem::EntityItem(Level &level, double x, double y, double z, ItemStack stack)
	: Entity(level)
{
	this->item = stack;
	setSize(0.25f, 0.25f);  // Alpha: setSize(0.25F, 0.25F) (EntityItem.java:13) - use setSize() to properly update bounding box
	this->heightOffset = this->bbHeight / 2.0f;  // Alpha: yOffset = height / 2.0F (EntityItem.java:14)
	
	setPos(x, y, z);  // Alpha: setPosition (EntityItem.java:15)
	
	// Alpha: Random rotation and initial velocity (EntityItem.java:17-20)
	this->yRot = (float)(Math::random() * 360.0);
	this->xd = (double)((float)(Math::random() * 0.2f - 0.1f));
	this->yd = 0.2;  // Alpha: motionY = 0.2F
	this->zd = (double)((float)(Math::random() * 0.2f - 0.1f));
	
	this->bobOffs = (float)(Math::random() * 3.14159265358979323846 * 2.0);  // Beta: bobOffs (ItemEntity.java:19) - Math.PI * 2.0
	
	// Beta: throwTime is set after construction, but we set it in constructor for convenience (ItemEntity.java:17)
	this->throwTime = 10;  // Beta: throwTime = 10 (default delay before pickup)
}

void EntityItem::tick()
{
	Entity::tick();
	
	// Beta: Decrement throwTime (ItemEntity.java:47-49)
	if (throwTime > 0)
		--throwTime;
	
	// Beta: Save previous position (ItemEntity.java:51-53)
	xo = x;
	yo = y;
	zo = z;
	
	// Alpha: Apply gravity (EntityItem.java:42)
	yd -= 0.04;  // motionY -= 0.04F
	
	// Alpha: Lava interaction (EntityItem.java:43-50)
	// TODO: Check if in lava material and apply bounce/particle effects
	
	// Alpha: Block collision check (EntityItem.java:52)
	func_466_g(x, y, z);
	
	// Alpha: Water movement (EntityItem.java:53)
	// TODO: handleWaterMovement() - check if in water
	
	// Alpha: Move entity (EntityItem.java:54)
	move(xd, yd, zd);  // Entity::move() instead of moveEntity()
	
	// Alpha: Friction calculation (EntityItem.java:55-62)
	float friction = 0.98f;  // Default air friction
	if (onGround)
	{
		friction = 0.1f * 0.1f * 58.8f;  // Alpha: 0.1F * 0.1F * 58.8F (EntityItem.java:57)
		// TODO: Get block slipperiness from tile at position
		// int_t blockId = level.getTile((int_t)x, (int_t)(bb.minY - 1.0), (int_t)z);
		// if (blockId > 0) friction = Tile::tiles[blockId]->slipperiness * 0.98f;
	}
	
	// Alpha: Apply friction (EntityItem.java:64-66)
	xd *= (double)friction;
	yd *= 0.98;  // motionY *= 0.98F
	zd *= (double)friction;
	
	// Alpha: Ground bounce (EntityItem.java:67-69)
	if (onGround)
		yd *= -0.5;  // motionY *= -0.5D
	
	// Alpha: Age increment and despawn (EntityItem.java:71-75)
	++age;
	if (age >= 6000)  // Alpha: despawn at 6000 ticks (5 minutes)
	{
		removed = true;  // Alpha: setEntityDead()
	}
	
	// Alpha: Check for player collision for pickup
	// This is handled in Level::tickEntities() via Entity::interact() or similar
}

bool EntityItem::func_466_g(double x, double y, double z)
{
	// Alpha: Block collision push-out logic (EntityItem.java:83-156)
	// This pushes items away from solid blocks to prevent clipping
	// Simplified version - full implementation would check all 6 faces
	// For now, return false (no collision detected)
	return false;
}

void EntityItem::playerTouch(Player &player)
{
	// Beta: ItemEntity.playerTouch() - pickup logic (ItemEntity.java:199-207)
	// Beta: Only handle pickup in single-player; server controls pickup in multiplayer
	// Beta: if (!this.world.isStatic) - only in single-player (ItemEntity.java:200)
	// In multiplayer, the server handles collision detection and pickup, sending Packet103SetSlot updates
	if (!level.isOnline)  // Beta: Only in single-player (ItemEntity.java:200)
	{
		int_t originalCount = item.stackSize;  // Beta: var2 = this.item.count (ItemEntity.java:201)
		
		// Beta: Check throwTime == 0 and try to add to inventory (ItemEntity.java:202)
		// Beta: var1.inventory.add(this.item) (ItemEntity.java:202)
		if (throwTime == 0 && player.inventory.add(item))
		{
			// Beta: Play pickup sound (ItemEntity.java:203)
			// Beta: level.playSound(this, "random.pop", 0.2F, ((this.random.nextFloat() - this.random.nextFloat()) * 0.7F + 1.0F) * 2.0F)
			level.playSound(this, u"random.pop", 0.2f, ((random.nextFloat() - random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
			
			// Beta: Call player.take() (ItemEntity.java:204)
			player.take(*this, originalCount);
			
			// Beta: Remove entity if pickup successful (ItemEntity.java:205)
			remove();  // Beta: this.remove() - removes entity immediately
		}
	}
	// Note: In multiplayer, the server handles item pickup via entity collision detection
	// and sends Packet103SetSlot updates to the client. The client should not do optimistic
	// pickup because it conflicts with server authority and causes inventory desyncs.
}

bool EntityItem::hurt(Entity *source, int_t dmg)
{
	// Alpha: Item damage handling (EntityItem.java:162-170 via canAttackEntity)
	// func_9281_M() call is skipped (seems to be empty in Alpha)
	health -= dmg;
	if (health <= 0)
	{
		removed = true;  // Alpha: setEntityDead()
	}
	return false;
}

bool EntityItem::shouldRenderAtSqrDistance(double distance)
{
	// Beta/Alpha: EntityItem uses a fixed render distance (typically 64 blocks = 4096 squared)
	// The default shouldRenderAtSqrDistance uses bb.getSize() which is too small for items (0.25x0.25x0.25)
	// Override to use a fixed distance like other small entities in Beta
	constexpr double MAX_RENDER_DISTANCE_SQR = 64.0 * 64.0;  // 64 blocks = 4096 squared
	return distance < MAX_RENDER_DISTANCE_SQR;
}
