#include "world/level/tile/StairTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/entity/Entity.h"
#include "world/entity/Mob.h"
#include "world/entity/player/Player.h"
#include "world/phys/AABB.h"
#include "Facing.h"
#include "java/Random.h"
#include "util/Mth.h"

// Beta: StairTile(int var1, Tile var2) (StairTile.java:17-23)
StairTile::StairTile(int_t id, Tile &baseTile) 
	: Tile(id, baseTile.tex, baseTile.material), base(&baseTile)
{
	// Beta: this.setDestroyTime(var2.destroySpeed) (StairTile.java:20)
	setDestroyTime(baseTile.getDestroySpeed());
	
	// Beta: this.setExplodeable(var2.explosionResistance / 3.0F) (StairTile.java:21)
	setExplodeable(baseTile.getExplosionResistance() / 3.0f);
	
	// Beta: this.setSoundType(var2.soundType) (StairTile.java:22)
	setSoundType(*baseTile.getSoundType());
	
	// Alpha: Stairs have exposed parts (they don't fill the entire block), so light should pass through
	// Similar to how slabs and farmland allow light through exposed parts
	// Since isSolidRender() returns false, we need to explicitly set lightBlock = 0
	setLightBlock(0);  // Allow light to propagate through exposed parts of stairs
}

void StairTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.updateShape() - resets shape to full block (StairTile.java:26-28)
	setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

AABB *StairTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.getAABB() - delegates to super (StairTile.java:31-33)
	return Tile::getAABB(level, x, y, z);
}

bool StairTile::isSolidRender()
{
	// Beta: StairTile.isSolidRender() - returns false (StairTile.java:36-38)
	return false;
}

bool StairTile::isCubeShaped()
{
	// Beta: StairTile.isCubeShaped() - returns false (StairTile.java:41-43)
	return false;
}

Tile::Shape StairTile::getRenderShape()
{
	// Beta: StairTile.getRenderShape() - returns 10 (SHAPE_STAIRS) (StairTile.java:46-48)
	return SHAPE_STAIRS;
}

void StairTile::addAABBs(Level &level, int_t x, int_t y, int_t z, AABB &bb, std::vector<AABB *> &aabbList)
{
	// Beta: StairTile.addAABBs() - creates two AABBs based on data (StairTile.java:56-81)
	int_t data = level.getData(x, y, z);
	
	if (data == 0)  // Beta: if (var7 == 0) (StairTile.java:58)
	{
		// Beta: this.setShape(0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 1.0F) (StairTile.java:59)
		setShape(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:60)
		
		// Beta: this.setShape(0.5F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F) (StairTile.java:61)
		setShape(0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:62)
	}
	else if (data == 1)  // Beta: else if (var7 == 1) (StairTile.java:63)
	{
		// Beta: this.setShape(0.0F, 0.0F, 0.0F, 0.5F, 1.0F, 1.0F) (StairTile.java:64)
		setShape(0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:65)
		
		// Beta: this.setShape(0.5F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F) (StairTile.java:66)
		setShape(0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:67)
	}
	else if (data == 2)  // Beta: else if (var7 == 2) (StairTile.java:68)
	{
		// Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 0.5F) (StairTile.java:69)
		setShape(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:70)
		
		// Beta: this.setShape(0.0F, 0.0F, 0.5F, 1.0F, 1.0F, 1.0F) (StairTile.java:71)
		setShape(0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:72)
	}
	else if (data == 3)  // Beta: else if (var7 == 3) (StairTile.java:73)
	{
		// Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.5F) (StairTile.java:74)
		setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:75)
		
		// Beta: this.setShape(0.0F, 0.0F, 0.5F, 1.0F, 0.5F, 1.0F) (StairTile.java:76)
		setShape(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
		Tile::addAABBs(level, x, y, z, bb, aabbList);  // Beta: super.addAABBs(...) (StairTile.java:77)
	}
	
	// Beta: Reset shape to full block (StairTile.java:80)
	setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void StairTile::addLights(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.addLights() - delegates to base (StairTile.java:84-86)
	base->addLights(level, x, y, z);
}

void StairTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: StairTile.animateTick() - delegates to base (StairTile.java:89-91)
	base->animateTick(level, x, y, z, random);
}

void StairTile::attack(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: StairTile.attack() - delegates to base (StairTile.java:94-96)
	base->attack(level, x, y, z, player);
}

void StairTile::destroy(Level &level, int_t x, int_t y, int_t z, int_t data)
{
	// Beta: StairTile.destroy() - delegates to base (StairTile.java:99-101)
	base->destroy(level, x, y, z, data);
}

float StairTile::getBrightness(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.getBrightness() - delegates to base (StairTile.java:104-106)
	return base->getBrightness(level, x, y, z);
}

int_t StairTile::getRenderLayer()
{
	// Beta: StairTile.getRenderLayer() - delegates to base (StairTile.java:114-116)
	return base->getRenderLayer();
}

int_t StairTile::getResource(int_t data, Random &random)
{
	// Beta: StairTile.getResource() - delegates to base (StairTile.java:119-121)
	return base->getResource(data, random);
}

int_t StairTile::getResourceCount(Random &random)
{
	// Beta: StairTile.getResourceCount() - delegates to base (StairTile.java:124-126)
	return base->getResourceCount(random);
}

int_t StairTile::getTexture(Facing face, int_t data)
{
	// Beta: StairTile.getTexture() - delegates to base (StairTile.java:129-131)
	return base->getTexture(face, data);
}

int_t StairTile::getTexture(Facing face)
{
	// Beta: StairTile.getTexture() - delegates to base (StairTile.java:134-136)
	return base->getTexture(face);
}

int_t StairTile::getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: StairTile.getTexture() - delegates to base (StairTile.java:139-141)
	return base->getTexture(level, x, y, z, face);
}

int_t StairTile::getTickDelay()
{
	// Beta: StairTile.getTickDelay() - delegates to base (StairTile.java:144-146)
	return base->getTickDelay();
}

AABB *StairTile::getTileAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.getTileAABB() - delegates to base (StairTile.java:149-151)
	return base->getTileAABB(level, x, y, z);
}

bool StairTile::mayPick()
{
	// Beta: StairTile.mayPick() - delegates to base (StairTile.java:159-161)
	return base->mayPick();
}

bool StairTile::mayPick(int_t data, bool canPickLiquid)
{
	// Beta: StairTile.mayPick() - delegates to base (StairTile.java:164-166)
	return base->mayPick(data, canPickLiquid);
}

bool StairTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.mayPlace() - delegates to base (StairTile.java:169-171)
	return base->mayPlace(level, x, y, z);
}

void StairTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.onPlace() - calls neighborChanged and base.onPlace (StairTile.java:174-177)
	neighborChanged(level, x, y, z, 0);  // Beta: this.neighborChanged(var1, var2, var3, var4, 0) (StairTile.java:175)
	base->onPlace(level, x, y, z);  // Beta: this.base.onPlace(var1, var2, var3, var4) (StairTile.java:176)
}

void StairTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.onRemove() - delegates to base (StairTile.java:180-182)
	base->onRemove(level, x, y, z);
}

void StairTile::prepareRender(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: StairTile.prepareRender() - delegates to base (StairTile.java:185-187)
	base->prepareRender(level, x, y, z);
}

// Note: spawnResources is not virtual in Tile, so we can't override it
// StairTile uses base tile's spawnResources directly (inherited)

void StairTile::stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	// Beta: StairTile.stepOn() - delegates to base (StairTile.java:200-202)
	base->stepOn(level, x, y, z, entity);
}

void StairTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: StairTile.tick() - delegates to base (StairTile.java:205-207)
	base->tick(level, x, y, z, random);
}

bool StairTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: StairTile.use() - delegates to base (StairTile.java:210-212)
	return base->use(level, x, y, z, player);
}

void StairTile::setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob)
{
	// Beta: StairTile.setPlacedBy() - sets orientation based on player rotation (StairTile.java:220-237)
	// Note: Java uses separate if statements (not else-if), so we match that exactly
	int_t direction = Mth::floor(mob.yRot * 4.0f / 360.0f + 0.5f) & 3;  // Beta: int var6 = Mth.floor(var5.yRot * 4.0F / 360.0F + 0.5) & 3 (StairTile.java:221)
	
	// Beta: Set orientation based on player rotation - different metadata values than FurnaceTile
	if (direction == 0)  // Beta: if (var6 == 0) (StairTile.java:222)
	{
		level.setData(x, y, z, 2);  // Beta: var1.setData(var2, var3, var4, 2) (StairTile.java:223)
	}
	
	if (direction == 1)  // Beta: if (var6 == 1) (StairTile.java:226)
	{
		level.setData(x, y, z, 1);  // Beta: var1.setData(var2, var3, var4, 1) (StairTile.java:227)
	}
	
	if (direction == 2)  // Beta: if (var6 == 2) (StairTile.java:230)
	{
		level.setData(x, y, z, 3);  // Beta: var1.setData(var2, var3, var4, 3) (StairTile.java:231)
	}
	
	if (direction == 3)  // Beta: if (var6 == 3) (StairTile.java:234)
	{
		level.setData(x, y, z, 0);  // Beta: var1.setData(var2, var3, var4, 0) (StairTile.java:235)
	}
}