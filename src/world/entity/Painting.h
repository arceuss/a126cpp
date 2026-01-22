#pragma once

#include "world/entity/Entity.h"
#include "nbt/CompoundTag.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/entity/item/EntityItem.h"
#include "world/level/material/Material.h"
#include <vector>
#include <string>

class Level;

// Alpha 1.2.6 Painting
// Reference: newb12/net/minecraft/world/entity/Painting.java
class Painting : public Entity
{
private:
	int_t checkInterval = 0;

public:
	enum class Motive
	{
		Kebab, Aztec, Alban, Aztec2, Bomb, Plant, Wasteland,
		Pool, Courbet, Sea, Sunset, Creebet,
		Wanderer, Graham,
		Match, Bust, Stage, Void, SkullAndRoses,
		Fighters,
		Pointer, Pigscene, BurningSkull,
		Skeleton, DonkeyKong
	};

public:
	int_t dir = 0;
	int_t xTile = 0;
	int_t yTile = 0;
	int_t zTile = 0;
	Motive motive = Motive::Kebab;

	struct MotiveData
	{
		jstring name;
		int_t w;
		int_t h;
		int_t uo;
		int_t vo;
	};

private:
	static MotiveData motiveData[];

public:
	Painting(Level &level);
	Painting(Level &level, int_t x, int_t y, int_t z, int_t dir);
	Painting(Level &level, int_t x, int_t y, int_t z, int_t dir, const jstring &motiveName);

protected:
	virtual void defineSynchedData();

public:
	void setDir(int_t dir);

private:
	float offs(int_t size);

public:
	virtual void tick() override;

public:
	bool survives();

public:
	virtual bool isPickable() override;

public:
	virtual bool hurt(Entity *source, int_t dmg) override;

public:
	virtual void addAdditionalSaveData(CompoundTag &tag) override;
	virtual void readAdditionalSaveData(CompoundTag &tag) override;

public:
	static Motive randomMotive();
	static MotiveData &getMotiveData(Motive motive);
};
