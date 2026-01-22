#include "world/level/tile/PressurePlateTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/phys/AABB.h"
#include "world/entity/Entity.h"
#include "world/entity/Mob.h"
#include "world/entity/player/Player.h"
#include "java/Random.h"

// Beta: PressurePlateTile(int var1, int var2, PressurePlateTile.Sensitivity var3) (PressurePlateTile.java:16-22)
PressurePlateTile::PressurePlateTile(int_t id, int_t tex, Sensitivity sensitivity) : Tile(id, tex, Material::stone), sensitivity(sensitivity)
{
	// Beta: PressurePlateTile uses Material.stone (PressurePlateTile.java:17)
	// Beta: this.sensitivity = var3 (PressurePlateTile.java:18)
	// Beta: this.setTicking(true) (PressurePlateTile.java:19)
	setTicking(true);
	// Beta: float var4 = 0.0625F (PressurePlateTile.java:20)
	float offset = 0.0625F;
	// Beta: this.setShape(var4, 0.0F, var4, 1.0F - var4, 0.03125F, 1.0F - var4) (PressurePlateTile.java:21)
	Tile::setShape(offset, 0.0F, offset, 1.0F - offset, 0.03125F, 1.0F - offset);
	// Beta: PressurePlateTile.blocksLight() returns false (PressurePlateTile.java:39-41)
	// Explicitly set lightBlock to 0 to ensure pressure plates don't block light
	setLightBlock(0);
}

// Beta: PressurePlateTile.getTickDelay() - returns 20 (PressurePlateTile.java:24-27)
int_t PressurePlateTile::getTickDelay()
{
	// Beta: return 20 (PressurePlateTile.java:26)
	return 20;
}

// Beta: PressurePlateTile.getAABB() - returns null (no collision) (PressurePlateTile.java:29-32)
AABB *PressurePlateTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	return nullptr;  // Beta: return null (PressurePlateTile.java:31)
}

// Beta: PressurePlateTile.isSolidRender() - returns false (PressurePlateTile.java:34-37)
bool PressurePlateTile::isSolidRender()
{
	return false;  // Beta: return false (PressurePlateTile.java:36)
}

// Beta: PressurePlateTile.blocksLight() - returns false (PressurePlateTile.java:39-41)
bool PressurePlateTile::blocksLight()
{
	return false;  // Beta: return false (PressurePlateTile.java:40)
}

// Beta: PressurePlateTile.isCubeShaped() - returns false (PressurePlateTile.java:43-46)
bool PressurePlateTile::isCubeShaped()
{
	return false;  // Beta: return false (PressurePlateTile.java:45)
}

// Beta: PressurePlateTile.mayPlace() - checks if can be placed on solid block (PressurePlateTile.java:48-51)
bool PressurePlateTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: return var1.isSolidTile(var2, var3 - 1, var4) (PressurePlateTile.java:50)
	return level.isSolidTile(x, y - 1, z);
}

// Beta: PressurePlateTile.onPlace() - empty (PressurePlateTile.java:53-55)
void PressurePlateTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: Empty method (PressurePlateTile.java:54)
}

// Beta: PressurePlateTile.neighborChanged() - breaks if support removed (PressurePlateTile.java:57-68)
void PressurePlateTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: boolean var6 = false (PressurePlateTile.java:59)
	bool shouldBreak = false;
	// Beta: if (!var1.isSolidTile(var2, var3 - 1, var4)) { var6 = true; } (PressurePlateTile.java:60-62)
	if (!level.isSolidTile(x, y - 1, z))
	{
		shouldBreak = true;
	}

	// Beta: if (var6) (PressurePlateTile.java:64)
	if (shouldBreak)
	{
		// Beta: this.spawnResources(var1, var2, var3, var4, var1.getData(var2, var3, var4)) (PressurePlateTile.java:65)
		spawnResources(level, x, y, z, level.getData(x, y, z));
		// Beta: var1.setTile(var2, var3, var4, 0) (PressurePlateTile.java:66)
		level.setTile(x, y, z, 0);
	}
}

// Beta: PressurePlateTile.tick() - checks if pressed (PressurePlateTile.java:70-77)
void PressurePlateTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: if (!var1.isOnline) (PressurePlateTile.java:72)
	if (!level.isOnline)
	{
		// Beta: if (var1.getData(var2, var3, var4) != 0) (PressurePlateTile.java:73)
		if (level.getData(x, y, z) != 0)
		{
			// Beta: this.checkPressed(var1, var2, var3, var4) (PressurePlateTile.java:74)
			checkPressed(level, x, y, z);
		}
	}
}

// Beta: PressurePlateTile.entityInside() - checks if pressed (PressurePlateTile.java:79-86)
void PressurePlateTile::entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	// Beta: if (!var1.isOnline) (PressurePlateTile.java:81)
	if (!level.isOnline)
	{
		// Beta: if (var1.getData(var2, var3, var4) != 1) (PressurePlateTile.java:82)
		if (level.getData(x, y, z) != 1)
		{
			// Beta: this.checkPressed(var1, var2, var3, var4) (PressurePlateTile.java:83)
			checkPressed(level, x, y, z);
		}
	}
}

// Beta: PressurePlateTile.checkPressed() - checks if entities are pressing (PressurePlateTile.java:88-128)
void PressurePlateTile::checkPressed(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: boolean var5 = var1.getData(var2, var3, var4) == 1 (PressurePlateTile.java:89)
	bool isPressed = level.getData(x, y, z) == 1;
	// Beta: boolean var6 = false (PressurePlateTile.java:90)
	bool hasEntity = false;
	// Beta: float var7 = 0.125F (PressurePlateTile.java:91)
	float offset = 0.125F;
	// Beta: List var8 = null (PressurePlateTile.java:92)
	std::vector<std::shared_ptr<Entity>> entities;
	
	// Beta: if (this.sensitivity == PressurePlateTile.Sensitivity.everything) (PressurePlateTile.java:93)
	if (sensitivity == Sensitivity::everything)
	{
		// Beta: var8 = var1.getEntities(null, AABB.newTemp(var2 + var7, var3, var4 + var7, var2 + 1 - var7, var3 + 0.25, var4 + 1 - var7)) (PressurePlateTile.java:94)
		AABB *aabb = AABB::newTemp((double)x + offset, (double)y, (double)z + offset, (double)x + 1 - offset, (double)y + 0.25, (double)z + 1 - offset);
		entities = level.getEntities(nullptr, *aabb);
	}
	// Beta: if (this.sensitivity == PressurePlateTile.Sensitivity.mobs) (PressurePlateTile.java:97)
	else if (sensitivity == Sensitivity::mobs)
	{
		// Beta: var8 = var1.getEntitiesOfClass(Mob.class, AABB.newTemp(var2 + var7, var3, var4 + var7, var2 + 1 - var7, var3 + 0.25, var4 + 1 - var7)) (PressurePlateTile.java:98)
		AABB *aabb = AABB::newTemp((double)x + offset, (double)y, (double)z + offset, (double)x + 1 - offset, (double)y + 0.25, (double)z + 1 - offset);
		const std::vector<std::shared_ptr<Entity>> &allEntities = level.getEntities(nullptr, *aabb);
		for (auto &entity : allEntities)
		{
			// Filter for Mob entities
			if (dynamic_cast<Mob*>(entity.get()) != nullptr)
				entities.push_back(entity);
		}
	}
	// Beta: if (this.sensitivity == PressurePlateTile.Sensitivity.players) (PressurePlateTile.java:101)
	else if (sensitivity == Sensitivity::players)
	{
		// Beta: var8 = var1.getEntitiesOfClass(Player.class, AABB.newTemp(var2 + var7, var3, var4 + var7, var2 + 1 - var7, var3 + 0.25, var4 + 1 - var7)) (PressurePlateTile.java:102)
		AABB *aabb = AABB::newTemp((double)x + offset, (double)y, (double)z + offset, (double)x + 1 - offset, (double)y + 0.25, (double)z + 1 - offset);
		const std::vector<std::shared_ptr<Entity>> &allEntities = level.getEntities(nullptr, *aabb);
		for (auto &entity : allEntities)
		{
			// Filter for Player entities
			if (dynamic_cast<Player*>(entity.get()) != nullptr)
				entities.push_back(entity);
		}
	}

	// Beta: if (var8.size() > 0) { var6 = true; } (PressurePlateTile.java:105-107)
	if (entities.size() > 0)
	{
		hasEntity = true;
	}

	// Beta: if (var6 && !var5) (PressurePlateTile.java:109)
	if (hasEntity && !isPressed)
	{
		// Beta: var1.setData(var2, var3, var4, 1) (PressurePlateTile.java:110)
		level.setData(x, y, z, 1);
		// Beta: Update shape immediately when pressure plate is pressed (for animation)
		updateShape(level, x, y, z);
		// Beta: var1.updateNeighborsAt(var2, var3, var4, this.id) (PressurePlateTile.java:111)
		level.updateNeighborsAt(x, y, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (PressurePlateTile.java:112)
		level.updateNeighborsAt(x, y - 1, z, id);
		// Beta: var1.setTilesDirty(var2, var3, var4, var2, var3, var4) (PressurePlateTile.java:113)
		level.setTilesDirty(x, y, z, x, y, z);
		// Beta: var1.playSound(var2 + 0.5, var3 + 0.1, var4 + 0.5, "random.click", 0.3F, 0.6F) (PressurePlateTile.java:114)
		level.playSound((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, u"random.click", 0.3F, 0.6F);
	}

	// Beta: if (!var6 && var5) (PressurePlateTile.java:117)
	if (!hasEntity && isPressed)
	{
		// Beta: var1.setData(var2, var3, var4, 0) (PressurePlateTile.java:118)
		level.setData(x, y, z, 0);
		// Beta: Update shape immediately when pressure plate is released (for animation)
		updateShape(level, x, y, z);
		// Beta: var1.updateNeighborsAt(var2, var3, var4, this.id) (PressurePlateTile.java:119)
		level.updateNeighborsAt(x, y, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (PressurePlateTile.java:120)
		level.updateNeighborsAt(x, y - 1, z, id);
		// Beta: var1.setTilesDirty(var2, var3, var4, var2, var3, var4) (PressurePlateTile.java:121)
		level.setTilesDirty(x, y, z, x, y, z);
		// Beta: var1.playSound(var2 + 0.5, var3 + 0.1, var4 + 0.5, "random.click", 0.3F, 0.5F) (PressurePlateTile.java:122)
		level.playSound((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, u"random.click", 0.3F, 0.5F);
	}

	// Beta: if (var6) { var1.addToTickNextTick(var2, var3, var4, this.id); } (PressurePlateTile.java:125-127)
	if (hasEntity)
	{
		level.scheduleBlockUpdate(x, y, z, id);
	}
}

// Beta: PressurePlateTile.onRemove() - updates neighbors if pressed (PressurePlateTile.java:130-139)
void PressurePlateTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: int var5 = var1.getData(var2, var3, var4) (PressurePlateTile.java:132)
	int_t data = level.getData(x, y, z);
	// Beta: if (var5 > 0) (PressurePlateTile.java:133)
	if (data > 0)
	{
		// Beta: var1.updateNeighborsAt(var2, var3, var4, this.id) (PressurePlateTile.java:134)
		level.updateNeighborsAt(x, y, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (PressurePlateTile.java:135)
		level.updateNeighborsAt(x, y - 1, z, id);
	}

	// Beta: super.onRemove(var1, var2, var3, var4) (PressurePlateTile.java:138)
	Tile::onRemove(level, x, y, z);
}

// Beta: PressurePlateTile.updateShape() - updates shape based on pressed state (PressurePlateTile.java:141-150)
void PressurePlateTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: boolean var5 = var1.getData(var2, var3, var4) == 1 (PressurePlateTile.java:143)
	bool isPressed = level.getData(x, y, z) == 1;
	// Beta: float var6 = 0.0625F (PressurePlateTile.java:144)
	float offset = 0.0625F;
	// Beta: if (var5) (PressurePlateTile.java:145)
	if (isPressed)
	{
		// Beta: this.setShape(var6, 0.0F, var6, 1.0F - var6, 0.03125F, 1.0F - var6) (PressurePlateTile.java:146)
		Tile::setShape(offset, 0.0F, offset, 1.0F - offset, 0.03125F, 1.0F - offset);
	}
	else
	{
		// Beta: this.setShape(var6, 0.0F, var6, 1.0F - var6, 0.0625F, 1.0F - var6) (PressurePlateTile.java:148)
		Tile::setShape(offset, 0.0F, offset, 1.0F - offset, 0.0625F, 1.0F - offset);
	}
}

// Beta: PressurePlateTile.getSignal() - returns true if pressed (PressurePlateTile.java:152-155)
bool PressurePlateTile::getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: return var1.getData(var2, var3, var4) > 0 (PressurePlateTile.java:154)
	return level.getData(x, y, z) > 0;
}

// Beta: PressurePlateTile.getDirectSignal() - returns direct signal (PressurePlateTile.java:157-160)
bool PressurePlateTile::getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: return var1.getData(var2, var3, var4) == 0 ? false : var5 == 1 (PressurePlateTile.java:159)
	return level.getData(x, y, z) == 0 ? false : facing == 1;
}

// Beta: PressurePlateTile.isSignalSource() - returns true (PressurePlateTile.java:162-165)
bool PressurePlateTile::isSignalSource()
{
	// Beta: return true (PressurePlateTile.java:164)
	return true;
}

// Beta: PressurePlateTile.updateDefaultShape() - updates default shape (PressurePlateTile.java:167-173)
void PressurePlateTile::updateDefaultShape()
{
	// Beta: float var1 = 0.5F (PressurePlateTile.java:169)
	float sizeX = 0.5F;
	// Beta: float var2 = 0.125F (PressurePlateTile.java:170)
	float sizeY = 0.125F;
	// Beta: float var3 = 0.5F (PressurePlateTile.java:171)
	float sizeZ = 0.5F;
	// Beta: this.setShape(0.5F - var1, 0.5F - var2, 0.5F - var3, 0.5F + var1, 0.5F + var2, 0.5F + var3) (PressurePlateTile.java:172)
	Tile::setShape(0.5F - sizeX, 0.5F - sizeY, 0.5F - sizeZ, 0.5F + sizeX, 0.5F + sizeY, 0.5F + sizeZ);
}
