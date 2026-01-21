#include "world/level/tile/ButtonTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"
#include "world/entity/player/Player.h"
#include "java/Random.h"

// Beta: ButtonTile(int var1, int var2) (ButtonTile.java:11-14)
ButtonTile::ButtonTile(int_t id, int_t tex) : Tile(id, tex, Material::decoration)
{
	setTicking(true);  // Beta: this.setTicking(true) (ButtonTile.java:13)
	// Beta: ButtonTile.blocksLight() returns false (ButtonTile.java:26-28)
	// Explicitly set lightBlock to 0 to ensure buttons don't block light
	setLightBlock(0);
}

AABB *ButtonTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ButtonTile.getAABB() - returns null (no collision) (ButtonTile.java:17-19)
	return nullptr;
}

int_t ButtonTile::getTickDelay()
{
	// Beta: ButtonTile.getTickDelay() - returns 20 (ButtonTile.java:22-24)
	return 20;
}

bool ButtonTile::isSolidRender()
{
	// Beta: ButtonTile.isSolidRender() returns false (ButtonTile.java:31-33)
	return false;
}

bool ButtonTile::isCubeShaped()
{
	// Beta: ButtonTile.isCubeShaped() returns false (ButtonTile.java:36-38)
	return false;
}

bool ButtonTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ButtonTile.mayPlace() - checks if can be placed on solid block (ButtonTile.java:41-49)
	if (level.isSolidTile(x - 1, y, z))
	{
		return true;
	}
	else if (level.isSolidTile(x + 1, y, z))
	{
		return true;
	}
	else
	{
		return level.isSolidTile(x, y, z - 1) ? true : level.isSolidTile(x, y, z + 1);
	}
}

void ButtonTile::setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: ButtonTile.setPlacedOnFace() - sets orientation based on face (ButtonTile.java:52-73)
	int_t var6 = level.getData(x, y, z);
	int_t var7 = var6 & 8;
	var6 &= 7;
	
	// Beta: Java uses int var5 (face value 0-5), C++ uses Facing enum
	int_t faceVal = static_cast<int_t>(face);
	
	if (faceVal == 2 && level.isSolidTile(x, y, z + 1))  // Beta: if (var5 == 2 && ...) var6 = 4 (NORTH face)
	{
		var6 = 4;
	}
	
	if (faceVal == 3 && level.isSolidTile(x, y, z - 1))  // Beta: if (var5 == 3 && ...) var6 = 3 (SOUTH face)
	{
		var6 = 3;
	}
	
	if (faceVal == 4 && level.isSolidTile(x + 1, y, z))  // Beta: if (var5 == 4 && ...) var6 = 2 (WEST face)
	{
		var6 = 2;
	}
	
	if (faceVal == 5 && level.isSolidTile(x - 1, y, z))  // Beta: if (var5 == 5 && ...) var6 = 1 (EAST face)
	{
		var6 = 1;
	}
	
	level.setData(x, y, z, var6 + var7);
}

void ButtonTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ButtonTile.onPlace() - sets orientation based on adjacent solid blocks (ButtonTile.java:76-88)
	if (level.isSolidTile(x - 1, y, z))
	{
		level.setData(x, y, z, 1);
	}
	else if (level.isSolidTile(x + 1, y, z))
	{
		level.setData(x, y, z, 2);
	}
	else if (level.isSolidTile(x, y, z - 1))
	{
		level.setData(x, y, z, 3);
	}
	else if (level.isSolidTile(x, y, z + 1))
	{
		level.setData(x, y, z, 4);
	}
	
	checkCanSurvive(level, x, y, z);
}

void ButtonTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: ButtonTile.neighborChanged() - checks if button can survive (ButtonTile.java:91-116)
	if (checkCanSurvive(level, x, y, z))
	{
		int_t var6 = level.getData(x, y, z) & 7;
		bool var7 = false;
		if (!level.isSolidTile(x - 1, y, z) && var6 == 1)
		{
			var7 = true;
		}
		
		if (!level.isSolidTile(x + 1, y, z) && var6 == 2)
		{
			var7 = true;
		}
		
		if (!level.isSolidTile(x, y, z - 1) && var6 == 3)
		{
			var7 = true;
		}
		
		if (!level.isSolidTile(x, y, z + 1) && var6 == 4)
		{
			var7 = true;
		}
		
		if (var7)
		{
			spawnResources(level, x, y, z, level.getData(x, y, z));
			level.setTile(x, y, z, 0);
		}
	}
}

bool ButtonTile::checkCanSurvive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ButtonTile.checkCanSurvive() - checks if button can survive (ButtonTile.java:118-126)
	if (!mayPlace(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
		return false;
	}
	else
	{
		return true;
	}
}

void ButtonTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: ButtonTile.updateShape() - sets shape based on data (ButtonTile.java:129-150)
	int_t var5 = level.getData(x, y, z);
	int_t var6 = var5 & 7;
	bool var7 = (var5 & 8) > 0;
	float var8 = 0.375F;
	float var9 = 0.625F;
	float var10 = 0.1875F;
	float var11 = 0.125F;
	if (var7)
	{
		var11 = 0.0625F;
	}
	
	if (var6 == 1)
	{
		setShape(0.0F, var8, 0.5F - var10, var11, var9, 0.5F + var10);
	}
	else if (var6 == 2)
	{
		setShape(1.0F - var11, var8, 0.5F - var10, 1.0F, var9, 0.5F + var10);
	}
	else if (var6 == 3)
	{
		setShape(0.5F - var10, var8, 0.0F, 0.5F + var10, var9, var11);
	}
	else if (var6 == 4)
	{
		setShape(0.5F - var10, var8, 1.0F - var11, 0.5F + var10, var9, 1.0F);
	}
}

void ButtonTile::attack(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: ButtonTile.attack() - calls use() (ButtonTile.java:153-155)
	use(level, x, y, z, player);
}

bool ButtonTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: ButtonTile.use() - toggles button state (ButtonTile.java:158-188)
	if (level.isOnline)
	{
		return true;
	}
	else
	{
		int_t var6 = level.getData(x, y, z);
		int_t var7 = var6 & 7;
		int_t var8 = 8 - (var6 & 8);
		if (var8 == 0)
		{
			return true;
		}
		else
		{
			level.setData(x, y, z, var7 + var8);
			// Beta: Update shape immediately when button state changes (for animation)
			updateShape(level, x, y, z);
			level.setTilesDirty(x, y, z, x, y, z);
			level.playSound(x + 0.5, y + 0.5, z + 0.5, u"random.click", 0.3F, 0.6F);
			level.updateNeighborsAt(x, y, z, id);
			if (var7 == 1)
			{
				level.updateNeighborsAt(x - 1, y, z, id);
			}
			else if (var7 == 2)
			{
				level.updateNeighborsAt(x + 1, y, z, id);
			}
			else if (var7 == 3)
			{
				level.updateNeighborsAt(x, y, z - 1, id);
			}
			else if (var7 == 4)
			{
				level.updateNeighborsAt(x, y, z + 1, id);
			}
			else
			{
				level.updateNeighborsAt(x, y - 1, z, id);
			}
			
			level.scheduleBlockUpdate(x, y, z, id);  // Beta: var1.addToTickNextTick(var2, var3, var4, this.id)
			return true;
		}
	}
}

void ButtonTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: ButtonTile.onRemove() - updates neighbors when removed (ButtonTile.java:191-210)
	int_t var5 = level.getData(x, y, z);
	if ((var5 & 8) > 0)
	{
		level.updateNeighborsAt(x, y, z, id);
		int_t var6 = var5 & 7;
		if (var6 == 1)
		{
			level.updateNeighborsAt(x - 1, y, z, id);
		}
		else if (var6 == 2)
		{
			level.updateNeighborsAt(x + 1, y, z, id);
		}
		else if (var6 == 3)
		{
			level.updateNeighborsAt(x, y, z - 1, id);
		}
		else if (var6 == 4)
		{
			level.updateNeighborsAt(x, y, z + 1, id);
		}
		else
		{
			level.updateNeighborsAt(x, y - 1, z, id);
		}
	}
	
	Tile::onRemove(level, x, y, z);
}

bool ButtonTile::getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: ButtonTile.getSignal() - returns signal state (ButtonTile.java:213-215)
	return (level.getData(x, y, z) & 8) > 0;
}

bool ButtonTile::getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing)
{
	// Beta: ButtonTile.getDirectSignal() - returns direct signal (ButtonTile.java:218-234)
	int_t var6 = level.getData(x, y, z);
	if ((var6 & 8) == 0)
	{
		return false;
	}
	else
	{
		int_t var7 = var6 & 7;
		if (var7 == 5 && facing == 1)
		{
			return true;
		}
		else if (var7 == 4 && facing == 2)
		{
			return true;
		}
		else if (var7 == 3 && facing == 3)
		{
			return true;
		}
		else
		{
			return var7 == 2 && facing == 4 ? true : var7 == 1 && facing == 5;
		}
	}
}

bool ButtonTile::isSignalSource()
{
	// Beta: ButtonTile.isSignalSource() returns true (ButtonTile.java:237-239)
	return true;
}

void ButtonTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: ButtonTile.tick() - auto-releases button after delay (ButtonTile.java:242-265)
	if (!level.isOnline)
	{
		int_t var6 = level.getData(x, y, z);
		if ((var6 & 8) != 0)
		{
			level.setData(x, y, z, var6 & 7);
			// Beta: Update shape immediately when button releases (for animation)
			updateShape(level, x, y, z);
			level.updateNeighborsAt(x, y, z, id);
			int_t var7 = var6 & 7;
			if (var7 == 1)
			{
				level.updateNeighborsAt(x - 1, y, z, id);
			}
			else if (var7 == 2)
			{
				level.updateNeighborsAt(x + 1, y, z, id);
			}
			else if (var7 == 3)
			{
				level.updateNeighborsAt(x, y, z - 1, id);
			}
			else if (var7 == 4)
			{
				level.updateNeighborsAt(x, y, z + 1, id);
			}
			else
			{
				level.updateNeighborsAt(x, y - 1, z, id);
			}
			
			level.playSound(x + 0.5, y + 0.5, z + 0.5, u"random.click", 0.3F, 0.5F);
			level.setTilesDirty(x, y, z, x, y, z);
		}
	}
}

void ButtonTile::updateDefaultShape()
{
	// Beta: ButtonTile.updateDefaultShape() - sets default shape (ButtonTile.java:268-273)
	float var1 = 0.1875F;
	float var2 = 0.125F;
	float var3 = 0.125F;
	setShape(0.5F - var1, 0.5F - var2, 0.5F - var3, 0.5F + var1, 0.5F + var2, 0.5F + var3);
}
