#include "world/level/tile/NotGateTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/tile/Tile.h"
#include "java/Random.h"

// Static member initialization
std::vector<NotGateTile::Toggle> NotGateTile::recentToggles;

// Beta: NotGateTile(int var1, int var2, boolean var3) (NotGateTile.java:39-43)
NotGateTile::NotGateTile(int_t id, int_t tex, bool on) : TorchTile(id, tex), on(on)
{
	// Beta: super(var1, var2) (NotGateTile.java:40)
	// Beta: this.on = var3 (NotGateTile.java:41)
	// Beta: this.setTicking(true) (NotGateTile.java:42)
	setTicking(true);
	
	// Alpha 1.2.6: Redstone torches should not block light (like normal torches before fix)
	// They also dim the block they're placed on (old torch behavior)
	setLightBlock(0);  // Don't block light
}

// Beta: NotGateTile.getTexture() - returns texture based on face (NotGateTile.java:15-18)
int_t NotGateTile::getTexture(Facing face, int_t data)
{
	// Beta: return var1 == 1 ? Tile.redStoneDust.getTexture(var1, var2) : super.getTexture(var1, var2) (NotGateTile.java:17)
	// Use Tile::tiles[55] to avoid circular dependency
	if (face == Facing::DOWN && Tile::tiles[55] != nullptr)
		return Tile::tiles[55]->getTexture(face, data);
	return TorchTile::getTexture(face, data);
}

// Beta: NotGateTile.getTickDelay() - returns 2 (NotGateTile.java:45-48)
int_t NotGateTile::getTickDelay()
{
	// Beta: return 2 (NotGateTile.java:47)
	return 2;
}

// Beta: NotGateTile.onPlace() - updates neighbors if on (NotGateTile.java:50-64)
void NotGateTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: if (var1.getData(var2, var3, var4) == 0) { super.onPlace(var1, var2, var3, var4); } (NotGateTile.java:52-54)
	if (level.getData(x, y, z) == 0)
	{
		TorchTile::onPlace(level, x, y, z);
	}

	// Beta: if (this.on) (NotGateTile.java:56)
	if (on)
	{
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (NotGateTile.java:57)
		level.updateNeighborsAt(x, y - 1, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 + 1, var4, this.id) (NotGateTile.java:58)
		level.updateNeighborsAt(x, y + 1, z, id);
		// Beta: var1.updateNeighborsAt(var2 - 1, var3, var4, this.id) (NotGateTile.java:59)
		level.updateNeighborsAt(x - 1, y, z, id);
		// Beta: var1.updateNeighborsAt(var2 + 1, var3, var4, this.id) (NotGateTile.java:60)
		level.updateNeighborsAt(x + 1, y, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3, var4 - 1, this.id) (NotGateTile.java:61)
		level.updateNeighborsAt(x, y, z - 1, id);
		// Beta: var1.updateNeighborsAt(var2, var3, var4 + 1, this.id) (NotGateTile.java:62)
		level.updateNeighborsAt(x, y, z + 1, id);
	}
}

// Beta: NotGateTile.onRemove() - updates neighbors if on (NotGateTile.java:66-76)
void NotGateTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: if (this.on) (NotGateTile.java:68)
	if (on)
	{
		// Beta: var1.updateNeighborsAt(var2, var3 - 1, var4, this.id) (NotGateTile.java:69)
		level.updateNeighborsAt(x, y - 1, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3 + 1, var4, this.id) (NotGateTile.java:70)
		level.updateNeighborsAt(x, y + 1, z, id);
		// Beta: var1.updateNeighborsAt(var2 - 1, var3, var4, this.id) (NotGateTile.java:71)
		level.updateNeighborsAt(x - 1, y, z, id);
		// Beta: var1.updateNeighborsAt(var2 + 1, var3, var4, this.id) (NotGateTile.java:72)
		level.updateNeighborsAt(x + 1, y, z, id);
		// Beta: var1.updateNeighborsAt(var2, var3, var4 - 1, this.id) (NotGateTile.java:73)
		level.updateNeighborsAt(x, y, z - 1, id);
		// Beta: var1.updateNeighborsAt(var2, var3, var4 + 1, this.id) (NotGateTile.java:74)
		level.updateNeighborsAt(x, y, z + 1, id);
	}
}

// Beta: NotGateTile.getSignal() - returns signal based on orientation (NotGateTile.java:78-94)
bool NotGateTile::getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: if (!this.on) { return false; } (NotGateTile.java:80-82)
	if (!on)
		return false;
	else
	{
		// Beta: int var6 = var1.getData(var2, var3, var4) (NotGateTile.java:83)
		int_t data = level.getData(x, y, z);
		// Beta: if (var6 == 5 && var5 == 1) { return false; } (NotGateTile.java:84-86)
		if (data == 5 && facing == 1)
			return false;
		// Beta: else if (var6 == 3 && var5 == 3) { return false; } (NotGateTile.java:86-88)
		else if (data == 3 && facing == 3)
			return false;
		// Beta: else if (var6 == 4 && var5 == 2) { return false; } (NotGateTile.java:88-90)
		else if (data == 4 && facing == 2)
			return false;
		else
		{
			// Beta: return var6 == 1 && var5 == 5 ? false : var6 != 2 || var5 != 4 (NotGateTile.java:91)
			return (data == 1 && facing == 5) ? false : (data != 2 || facing != 4);
		}
	}
}

// Beta: NotGateTile.hasNeighborSignal() - checks if neighbor has signal (NotGateTile.java:96-107)
bool NotGateTile::hasNeighborSignal(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: int var5 = var1.getData(var2, var3, var4) (NotGateTile.java:97)
	int_t data = level.getData(x, y, z);
	// Beta: if (var5 == 5 && var1.getSignal(var2, var3 - 1, var4, 0)) { return true; } (NotGateTile.java:98-100)
	if (data == 5 && level.getSignal(x, y - 1, z, 0))
		return true;
	// Beta: else if (var5 == 3 && var1.getSignal(var2, var3, var4 - 1, 2)) { return true; } (NotGateTile.java:100-102)
	else if (data == 3 && level.getSignal(x, y, z - 1, 2))
		return true;
	// Beta: else if (var5 == 4 && var1.getSignal(var2, var3, var4 + 1, 3)) { return true; } (NotGateTile.java:102-104)
	else if (data == 4 && level.getSignal(x, y, z + 1, 3))
		return true;
	else
	{
		// Beta: return var5 == 1 && var1.getSignal(var2 - 1, var3, var4, 4) ? true : var5 == 2 && var1.getSignal(var2 + 1, var3, var4, 5) (NotGateTile.java:105)
		return (data == 1 && level.getSignal(x - 1, y, z, 4)) ? true : (data == 2 && level.getSignal(x + 1, y, z, 5));
	}
}

// Beta: NotGateTile.isToggledTooFrequently() - checks toggle frequency (NotGateTile.java:20-37)
bool NotGateTile::isToggledTooFrequently(Level &level, int_t x, int_t y, int_t z, bool addToggle)
{
	// Beta: if (var5) { recentToggles.add(new NotGateTile.Toggle(var2, var3, var4, var1.time)); } (NotGateTile.java:21-23)
	if (addToggle)
	{
		recentToggles.push_back(Toggle(x, y, z, level.time));
	}

	// Beta: int var6 = 0 (NotGateTile.java:25)
	int_t count = 0;
	// Beta: for (int var7 = 0; var7 < recentToggles.size(); var7++) (NotGateTile.java:27)
	for (size_t i = 0; i < recentToggles.size(); i++)
	{
		Toggle &toggle = recentToggles[i];  // Beta: NotGateTile.Toggle var8 = recentToggles.get(var7) (NotGateTile.java:28)
		// Beta: if (var8.x == var2 && var8.y == var3 && var8.z == var4) (NotGateTile.java:29)
		if (toggle.x == x && toggle.y == y && toggle.z == z)
		{
			// Beta: if (++var6 >= 8) { return true; } (NotGateTile.java:30-32)
			if (++count >= 8)
				return true;
		}
	}

	// Beta: return false (NotGateTile.java:36)
	return false;
}

// Beta: NotGateTile.tick() - toggles based on neighbor signal (NotGateTile.java:109-134)
void NotGateTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: boolean var6 = this.hasNeighborSignal(var1, var2, var3, var4) (NotGateTile.java:111)
	bool hasSignal = hasNeighborSignal(level, x, y, z);

	// Beta: while (recentToggles.size() > 0 && var1.time - recentToggles.get(0).when > 100L) { recentToggles.remove(0); } (NotGateTile.java:113-115)
	while (recentToggles.size() > 0 && level.time - recentToggles[0].when > 100L)
	{
		recentToggles.erase(recentToggles.begin());
	}

	// Beta: if (this.on) (NotGateTile.java:117)
	if (on)
	{
		// Beta: if (var6) (NotGateTile.java:118)
		if (hasSignal)
		{
		// Beta: var1.setTileAndData(var2, var3, var4, Tile.notGate_off.id, var1.getData(var2, var3, var4)) (NotGateTile.java:119)
		// Use ID 75 directly to avoid circular dependency
		level.setTileAndData(x, y, z, 75, level.getData(x, y, z));
			// Beta: if (this.isToggledTooFrequently(var1, var2, var3, var4, true)) (NotGateTile.java:120)
			if (isToggledTooFrequently(level, x, y, z, true))
			{
			// Beta: var1.playSound(var2 + 0.5F, var3 + 0.5F, var4 + 0.5F, "random.fizz", 0.5F, 2.6F + (var1.random.nextFloat() - var1.random.nextFloat()) * 0.8F) (NotGateTile.java:121)
			float pitch = 2.6F + (random.nextFloat() - random.nextFloat()) * 0.8F;
			level.playSound((double)x + 0.5F, (double)y + 0.5F, (double)z + 0.5F, u"random.fizz", 0.5F, pitch);
				
				// Beta: for (int var7 = 0; var7 < 5; var7++) (NotGateTile.java:123)
				for (int_t i = 0; i < 5; i++)
				{
					// Beta: double var8 = var2 + var5.nextDouble() * 0.6 + 0.2 (NotGateTile.java:124)
					double px = (double)x + random.nextDouble() * 0.6 + 0.2;
					// Beta: double var10 = var3 + var5.nextDouble() * 0.6 + 0.2 (NotGateTile.java:125)
					double py = (double)y + random.nextDouble() * 0.6 + 0.2;
					// Beta: double var12 = var4 + var5.nextDouble() * 0.6 + 0.2 (NotGateTile.java:126)
					double pz = (double)z + random.nextDouble() * 0.6 + 0.2;
					// Beta: var1.addParticle("smoke", var8, var10, var12, 0.0, 0.0, 0.0) (NotGateTile.java:127)
					level.addParticle(u"smoke", px, py, pz, 0.0, 0.0, 0.0);
				}
			}
		}
	}
	// Beta: else if (!var6 && !this.isToggledTooFrequently(var1, var2, var3, var4, false)) (NotGateTile.java:131)
	else if (!hasSignal && !isToggledTooFrequently(level, x, y, z, false))
	{
		// Beta: var1.setTileAndData(var2, var3, var4, Tile.notGate_on.id, var1.getData(var2, var3, var4)) (NotGateTile.java:132)
		// Use ID 76 directly to avoid circular dependency
		level.setTileAndData(x, y, z, 76, level.getData(x, y, z));
	}
}

// Beta: NotGateTile.neighborChanged() - schedules tick (NotGateTile.java:136-140)
void NotGateTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: super.neighborChanged(var1, var2, var3, var4, var5) (NotGateTile.java:138)
	TorchTile::neighborChanged(level, x, y, z, tile);
	// Beta: var1.addToTickNextTick(var2, var3, var4, this.id) (NotGateTile.java:139)
	level.scheduleBlockUpdate(x, y, z, id);
}

// Beta: NotGateTile.getDirectSignal() - returns direct signal (NotGateTile.java:142-145)
bool NotGateTile::getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: return var5 == 0 ? this.getSignal(var1, var2, var3, var4, var5) : false (NotGateTile.java:144)
	return facing == 0 ? getSignal(level, x, y, z, facing) : false;
}

// Beta: NotGateTile.getResource() - returns notGate_on (NotGateTile.java:147-150)
int_t NotGateTile::getResource(int_t data, Random &random)
{
	// Beta: return Tile.notGate_on.id (NotGateTile.java:149)
	// Use ID 76 directly to avoid circular dependency
	return 76;
}

// Beta: NotGateTile.isSignalSource() - returns true (NotGateTile.java:152-155)
bool NotGateTile::isSignalSource()
{
	// Beta: return true (NotGateTile.java:154)
	return true;
}

// Beta: NotGateTile.animateTick() - adds reddust particles if on (NotGateTile.java:157-178)
void NotGateTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: if (this.on) (NotGateTile.java:159)
	if (on)
	{
		// Beta: int var6 = var1.getData(var2, var3, var4) (NotGateTile.java:160)
		int_t data = level.getData(x, y, z);
		// Beta: double var7 = var2 + 0.5F + (var5.nextFloat() - 0.5F) * 0.2 (NotGateTile.java:161)
		double px = (double)x + 0.5F + (random.nextFloat() - 0.5F) * 0.2;
		// Beta: double var9 = var3 + 0.7F + (var5.nextFloat() - 0.5F) * 0.2 (NotGateTile.java:162)
		double py = (double)y + 0.7F + (random.nextFloat() - 0.5F) * 0.2;
		// Beta: double var11 = var4 + 0.5F + (var5.nextFloat() - 0.5F) * 0.2 (NotGateTile.java:163)
		double pz = (double)z + 0.5F + (random.nextFloat() - 0.5F) * 0.2;
		// Beta: double var13 = 0.22F (NotGateTile.java:164)
		double offsetY = 0.22F;
		// Beta: double var15 = 0.27F (NotGateTile.java:165)
		double offsetXZ = 0.27F;
		
		// Beta: if (var6 == 1) { var1.addParticle("reddust", var7 - var15, var9 + var13, var11, 0.0, 0.0, 0.0); } (NotGateTile.java:166-168)
		if (data == 1)
		{
			level.addParticle(u"reddust", px - offsetXZ, py + offsetY, pz, 0.0, 0.0, 0.0);
		}
		// Beta: else if (var6 == 2) { var1.addParticle("reddust", var7 + var15, var9 + var13, var11, 0.0, 0.0, 0.0); } (NotGateTile.java:168-170)
		else if (data == 2)
		{
			level.addParticle(u"reddust", px + offsetXZ, py + offsetY, pz, 0.0, 0.0, 0.0);
		}
		// Beta: else if (var6 == 3) { var1.addParticle("reddust", var7, var9 + var13, var11 - var15, 0.0, 0.0, 0.0); } (NotGateTile.java:170-172)
		else if (data == 3)
		{
			level.addParticle(u"reddust", px, py + offsetY, pz - offsetXZ, 0.0, 0.0, 0.0);
		}
		// Beta: else if (var6 == 4) { var1.addParticle("reddust", var7, var9 + var13, var11 + var15, 0.0, 0.0, 0.0); } (NotGateTile.java:172-174)
		else if (data == 4)
		{
			level.addParticle(u"reddust", px, py + offsetY, pz + offsetXZ, 0.0, 0.0, 0.0);
		}
		// Beta: else { var1.addParticle("reddust", var7, var9, var11, 0.0, 0.0, 0.0); } (NotGateTile.java:175-176)
		else
		{
			level.addParticle(u"reddust", px, py, pz, 0.0, 0.0, 0.0);
		}
	}
}
