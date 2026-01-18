#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "world/item/ItemStack.h"
#include "world/phys/AABB.h"
#include "world/phys/Vec3.h"
#include <vector>
#include <memory>

class Level;
class Player;

// Alpha 1.2.6 Minecart
// Reference: newb12/net/minecraft/world/entity/item/Minecart.java
class Minecart : public Entity
{
public:
	static constexpr int_t RIDEABLE = 0;
	static constexpr int_t CHEST = 1;
	static constexpr int_t FURNACE = 2;

private:
	std::vector<std::shared_ptr<ItemStack>> items;  // 36 items for chest minecart

public:
	int_t damage = 0;
	int_t hurtTime = 0;
	int_t hurtDir = 1;

private:
	bool flipped = false;

public:
	int_t type = 0;
	int_t fuel = 0;
	double xPush = 0.0;
	double zPush = 0.0;

private:
	static constexpr int EXITS[10][2][3] = {
		{{0, 0, -1}, {0, 0, 1}},
		{{-1, 0, 0}, {1, 0, 0}},
		{{-1, -1, 0}, {1, 0, 0}},
		{{-1, 0, 0}, {1, -1, 0}},
		{{0, 0, -1}, {0, -1, 1}},
		{{0, -1, -1}, {0, 0, 1}},
		{{0, 0, 1}, {1, 0, 0}},
		{{0, 0, 1}, {-1, 0, 0}},
		{{0, 0, -1}, {-1, 0, 0}},
		{{0, 0, -1}, {1, 0, 0}}
	};

	int_t lSteps = 0;
	double lx = 0.0;
	double ly = 0.0;
	double lz = 0.0;
	double lyr = 0.0;
	double lxr = 0.0;
	double lxd = 0.0;
	double lyd = 0.0;
	double lzd = 0.0;

public:
	Minecart(Level &level);
	Minecart(Level &level, double x, double y, double z, int_t type);

protected:
	virtual void defineSynchedData();

public:
	virtual AABB *getCollideAgainstBox(Entity &entity) override;
	virtual AABB *getCollideBox() override;
	virtual bool isPushable() override;

public:
	virtual double getRideHeight() override;

public:
	virtual bool hurt(Entity *source, int_t dmg) override;
	virtual void animateHurt() override;
	virtual bool isPickable() override;

public:
	virtual void remove();

public:
	virtual void tick() override;

public:
	Vec3 *getPosOffs(double x, double y, double z, double offset);
	Vec3 *getPos(double x, double y, double z);

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	virtual float getShadowHeightOffs();

public:
	virtual void push(Entity &entity);
	using Entity::push;  // Make the 3-arg push available

	// Container interface methods
	int_t getContainerSize();
	std::shared_ptr<ItemStack> getItem(int_t slot);
	std::shared_ptr<ItemStack> removeItem(int_t slot, int_t count);
	void setItem(int_t slot, std::shared_ptr<ItemStack> itemStack);
	jstring getName();
	int_t getMaxStackSize();
	void setChanged();
	bool stillValid(Player &player);

public:
	float getLootContent();

public:
	virtual void lerpTo(double x, double y, double z, float yRot, float xRot, int_t steps) override;
	virtual void lerpMotion(double x, double y, double z);

public:
	virtual bool interact(Player &player) override;
};
