#pragma once

#include "world/entity/Mob.h"

#include "world/level/tile/Tile.h"
#include "world/entity/player/InventoryPlayer.h"

#include "java/Type.h"
#include "java/String.h"
#include <memory>

class EntityItem;
class FishingHook;

class Player : public Mob
{
public:
	static constexpr int_t MAX_HEALTH = 20;
	static constexpr int_t SWING_DURATION = 8;

	byte_t userType = 0;

	int_t score = 0;

	float oBob = 0.0f;
	float bob = 0.0f;

	bool swinging = false;
	int_t swingTime = 0;

	jstring name;

	int_t dimension = 0;

	// Beta: Player.fishing - reference to active fishing hook (Player.java:52)
	FishingHook *fishing = nullptr;

	int_t dmgSpill = 0;  // Beta: dmgSpill - damage spillover for armor (Player.java:47)
	
	InventoryPlayer inventory = InventoryPlayer(this);

	Player(Level &level);

	void tick() override;

protected:
	virtual void closeContainer();

public:
	virtual void rideTick() override;

	virtual void resetPos() override;

protected:
	virtual void updateAi() override;

public:
	virtual void aiStep() override;

private:
	void touch(Entity &e);

public:
	virtual void swing();
	virtual void attack(const std::shared_ptr<Entity> &entity);

	virtual void respawn();

public:
	float getDestroySpeed(Tile &tile);
	bool canDestroy(Tile &tile);

	void readAdditionalSaveData(CompoundTag &tag) override;
	void addAdditionalSaveData(CompoundTag &tag) override;

	float getHeadHeight() override;

	jstring getTexture() override;  // Testing: override getTexture in Player

	bool hurt(Entity *source, int_t dmg) override;

protected:
	void actuallyHurt(int_t dmg) override;

public:
	void interact(const std::shared_ptr<Entity> &entity);

	// Beta: Player helper methods (Player.java:190-196, 312-313, 376-387)
	bool addResource(int_t itemID);  // Beta: addResource(int var1) - adds item to inventory (Player.java:190-192)
	int_t getScore() const;  // Beta: getScore() - returns score (Player.java:194-196)
	virtual void take(Entity &entity, int_t count);  // Beta: take(Entity var1, int var2) - empty method (Player.java:312-313), overridden in LocalPlayer
	ItemStack *getSelectedItem();  // Beta: getSelectedItem() - returns inventory.getSelected() (Player.java:376-378)
	void removeSelectedItem();  // Beta: removeSelectedItem() - sets selected slot to null (Player.java:380-382)
	double getRidingHeight() override;  // Beta: getRidingHeight() - returns heightOffset - 0.5F (Player.java:385-387)

	// Beta: Player drop methods (Player.java:234-272)
	void drop();  // Beta: drop() - drops selected item (Player.java:234-236)
	void drop(ItemStack &stack);  // Beta: drop(ItemInstance var1) - drops item with default spread (Player.java:238-240)
	void drop(ItemStack &stack, bool randomSpread);  // Beta: drop(ItemInstance var1, boolean var2) - drops item (Player.java:242-268)
	
protected:
	void reallyDrop(std::shared_ptr<EntityItem> itemEntity);  // Beta: reallyDrop(ItemEntity var1) - adds entity to level (Player.java:270-272)
	
public:
	// Beta: Player die() override (Player.java:199-217)
	// Note: Mob::die() is not virtual, so this shadows it (same behavior as Beta 1.2)
	void die(Entity *source);  // Beta: die(Entity var1) - drops inventory and changes size (Player.java:199-217)

	// Beta: Player.remove() - container cleanup (Player.java:420-425)
	// Note: Entity::remove() is not virtual, so this shadows it (same behavior as Beta 1.2)
	void remove();  // Beta: remove() - cleans up containers and calls super.remove()

	// Beta: Player helper methods (Player.java:220-232)
	void awardKillScore(Entity &source, int_t dmg) override;  // Beta: awardKillScore(Entity var1, int var2) - adds to score (Player.java:220-222)
	bool isShootable() override;  // Beta: isShootable() - returns true (Player.java:225-227)
	bool isCreativeModeAllowed() override;  // Beta: isCreativeModeAllowed() - returns true (Player.java:230-232)

	bool isPlayer() override { return true; }
	
	// Beta: Player.openTextEdit() - empty method (Player.java:360-361), overridden in LocalPlayer
	virtual void openTextEdit(std::shared_ptr<class SignTileEntity> signEntity) {}
};
