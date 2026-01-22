#include "world/level/tile/GrassTile.h"

#include "world/level/tile/DirtTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/GrassColor.h"
#include "java/Random.h"
#include "Facing.h"

GrassTile::GrassTile(int_t id) : Tile(id, Material::dirt)  // Beta: Material.dirt (GrassTile.java:13)
{
	tex = 3;  // Beta: this.tex = 3 (GrassTile.java:14)
	setTicking(true);  // Beta: this.setTicking(true) (GrassTile.java:15)
}

int_t GrassTile::getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: GrassTile.getTexture() (GrassTile.java:19-28)
	if (face == Facing::UP)  // Face 1
		return 0;
	else if (face == Facing::DOWN)  // Face 0
		return 2;
	else
	{
		// Check if block above is snow/topSnow
		const Material &aboveMat = level.getMaterial(x, y + 1, z);
		if (aboveMat == Material::topSnow || aboveMat == Material::snow)
			return 68;  // Snow-covered grass texture
		return 3;  // Normal grass side texture
	}
}

int_t GrassTile::getColor(LevelSource &level, int_t x, int_t y, int_t z)
{
	level.getBiomeSource().getBiomeBlock(x, z, 1, 1);
	double temperature = level.getBiomeSource().temperatures[0];
	double downfall = level.getBiomeSource().downfalls[0];
	return GrassColor::get(temperature, downfall);
}

void GrassTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: GrassTile.tick() - grass spread/decay logic (GrassTile.java:39-58)
	if (!level.isOnline)
	{
		// Check if grass should decay (low light above)
		if (level.getRawBrightness(x, y + 1, z) < MIN_BRIGHTNESS && level.getMaterial(x, y + 1, z).blocksLight())
		{
			// 3/4 chance to not decay this tick
			if (random.nextInt(4) != 0)
				return;
			level.setTile(x, y, z, Tile::dirt.id);
		}
		// Check if grass should spread (high light above)
		else if (level.getRawBrightness(x, y + 1, z) >= 9)
		{
			// Try to spread to nearby dirt
			int_t spreadX = x + random.nextInt(3) - 1;
			int_t spreadY = y + random.nextInt(5) - 3;
			int_t spreadZ = z + random.nextInt(3) - 1;
			
			if (level.getTile(spreadX, spreadY, spreadZ) == Tile::dirt.id &&
			    level.getRawBrightness(spreadX, spreadY + 1, spreadZ) >= MIN_BRIGHTNESS &&
			    !level.getMaterial(spreadX, spreadY + 1, spreadZ).blocksLight())
			{
				level.setTile(spreadX, spreadY, spreadZ, Tile::grass.id);
			}
		}
	}
}

int_t GrassTile::getResource(int_t data, Random &random)
{
	// Beta: GrassTile.getResource() returns Tile.dirt.getResource(0, var2) (GrassTile.java:61-63)
	return Tile::dirt.getResource(0, random);
}
