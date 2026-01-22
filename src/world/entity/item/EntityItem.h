#pragma once

#include "world/entity/Entity.h"
#include "world/item/ItemStack.h"
#include "java/Type.h"

class Player;

// Alpha 1.2.6 EntityItem - item pickup entity
// Reference: apclient/minecraft/src/net/minecraft/src/EntityItem.java
class EntityItem : public Entity
{
public:
	ItemStack item;
	
	int_t age = 0;  // Age counter, despawns at 6000 (public for rendering)
	float bobOffs = 0.0f;  // Beta: EntityItem.bobOffs - Random rotation/bob offset (field_804_d in Alpha) (public for rendering)
	int_t throwTime = 10;  // Beta: throwTime - pickup delay (10 ticks = 0.5 seconds) (ItemEntity.java:17, 47-48) - public for drop() access
	
private:
	int_t health = 5;  // Item health (damageable by explosions, etc.)
	
public:
	// Alpha 1.2.6: EntityItem constructor with full parameters
	EntityItem(Level &level, double x, double y, double z, ItemStack stack);
	
	// Alpha 1.2.6: EntityItem constructor for EntityIO (takes only Level, sets defaults)
	// Used when creating from EntityList.createEntity() - position and stack set separately
	EntityItem(Level &level);
	
	virtual void tick() override;
	
	// Beta: EntityItem.playerTouch() - pickup logic (ItemEntity.java:199-207)
	virtual void playerTouch(Player &player) override;
	
	// Entity callbacks
	virtual bool hurt(Entity *source, int_t dmg) override;
	
	// Override render distance - EntityItem needs larger render distance than default
	virtual bool shouldRenderAtSqrDistance(double distance) override;
	
protected:
	// Helper: handle collision detection with blocks
	bool func_466_g(double x, double y, double z);
};
