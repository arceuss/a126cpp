#include "world/level/tile/FarmTile.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/DirtTile.h"
#include "world/phys/AABB.h"
#include "Facing.h"
#include "java/Random.h"

// Beta: FarmTile(int var1) (FarmTile.java:10-16)
// Alpha 1.2.6: Farmland should not block light (crops need light to grow)
FarmTile::FarmTile(int_t id) : Tile(id, Material::dirt)
{
	tex = 87;  // Beta: this.tex = 87 (FarmTile.java:12)
	setTicking(true);  // Beta: this.setTicking(true) (FarmTile.java:13)
	setShape(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);  // Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.9375F, 1.0F) (FarmTile.java:14)
	// Alpha: Farmland should not block light - crops need light to grow
	// Note: Beta sets setLightBlock(255) but Alpha behavior is different
}

AABB *FarmTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FarmTile.getAABB() - returns full block AABB (FarmTile.java:19-21)
	return AABB::newTemp(x + 0, y + 0, z + 0, x + 1, y + 1, z + 1);
}

bool FarmTile::isSolidRender()
{
	// Beta: FarmTile.isSolidRender() - returns false (FarmTile.java:24-26)
	return false;
}

bool FarmTile::isCubeShaped()
{
	// Beta: FarmTile.isCubeShaped() - returns false (FarmTile.java:29-31)
	return false;
}

int_t FarmTile::getTexture(Facing face, int_t data)
{
	// Beta: FarmTile.getTexture() - different textures based on face and data (FarmTile.java:34-40)
	if (face == Facing::UP && data > 0)  // Beta: if (var1 == 1 && var2 > 0) (FarmTile.java:35)
	{
		return tex - 1;  // Beta: return this.tex - 1 (texture 86) (FarmTile.java:36)
	}
	else if (face == Facing::UP)  // Beta: else if (var1 == 1) (FarmTile.java:38)
	{
		return tex;  // Beta: return this.tex (texture 87) (FarmTile.java:38)
	}
	else
	{
		return 2;  // Beta: return 2 (dirt texture) (FarmTile.java:39)
	}
}

void FarmTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: FarmTile.tick() - hydration logic (FarmTile.java:43-56)
	if (random.nextInt(5) == 0)  // Beta: if (var5.nextInt(5) == 0) (FarmTile.java:44)
	{
		if (isNearWater(level, x, y, z))  // Beta: if (this.isNearWater(var1, var2, var3, var4)) (FarmTile.java:45)
		{
			level.setData(x, y, z, 7);  // Beta: var1.setData(var2, var3, var4, 7) (FarmTile.java:46)
		}
		else
		{
			int_t data = level.getData(x, y, z);  // Beta: int var6 = var1.getData(var2, var3, var4) (FarmTile.java:48)
			if (data > 0)  // Beta: if (var6 > 0) (FarmTile.java:49)
			{
				level.setData(x, y, z, data - 1);  // Beta: var1.setData(var2, var3, var4, var6 - 1) (FarmTile.java:50)
			}
			else if (!isUnderCrops(level, x, y, z))  // Beta: else if (!this.isUnderCrops(var1, var2, var3, var4)) (FarmTile.java:51)
			{
				level.setTile(x, y, z, Tile::dirt.id);  // Beta: var1.setTile(var2, var3, var4, Tile.dirt.id) (FarmTile.java:52)
			}
		}
	}
}

void FarmTile::stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	// Beta: FarmTile.stepOn() - converts to dirt if stepped on (FarmTile.java:59-63)
	if (level.random.nextInt(4) == 0)  // Beta: if (var1.random.nextInt(4) == 0) (FarmTile.java:60)
	{
		level.setTile(x, y, z, Tile::dirt.id);  // Beta: var1.setTile(var2, var3, var4, Tile.dirt.id) (FarmTile.java:61)
	}
}

void FarmTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: FarmTile.neighborChanged() - converts to dirt if solid block above (FarmTile.java:94-100)
	Tile::neighborChanged(level, x, y, z, tile);  // Beta: super.neighborChanged(var1, var2, var3, var4, var5) (FarmTile.java:95)
	const Material &mat = level.getMaterial(x, y + 1, z);  // Beta: Material var6 = var1.getMaterial(var2, var3 + 1, var4) (FarmTile.java:96)
	if (mat.isSolid())  // Beta: if (var6.isSolid()) (FarmTile.java:97)
	{
		level.setTile(x, y, z, Tile::dirt.id);  // Beta: var1.setTile(var2, var3, var4, Tile.dirt.id) (FarmTile.java:98)
	}
}

int_t FarmTile::getResource(int_t data, Random &random)
{
	// Alpha: BlockSoil.idDropped() returns Block.dirt.idDropped(0, var2) (BlockSoil.java:96-97)
	// Beta: FarmTile.getResource() - drops dirt (FarmTile.java:107-109)
	return Tile::dirt.getResource(0, random);  // Alpha/Beta: return Tile.dirt.getResource(0, var2)
}

bool FarmTile::isUnderCrops(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FarmTile.isUnderCrops() - checks if crops are above (FarmTile.java:65-77)
	byte_t range = 0;  // Beta: byte var5 = 0 (FarmTile.java:66)
	
	for (int_t dx = x - range; dx <= x + range; dx++)  // Beta: for (int var6 = var2 - var5; var6 <= var2 + var5; var6++) (FarmTile.java:68)
	{
		for (int_t dz = z - range; dz <= z + range; dz++)  // Beta: for (int var7 = var4 - var5; var7 <= var4 + var5; var7++) (FarmTile.java:69)
		{
			// Beta: if (var1.getTile(var6, var3 + 1, var7) == Tile.crops.id) (FarmTile.java:70)
			int_t aboveTile = level.getTile(dx, y + 1, dz);
			// TODO: Replace with Tile::crops.id once CropTile is ported (ID 59)
			if (aboveTile == 59)  // crops ID (temporary - will use Tile::crops.id)
			{
				return true;  // Beta: return true (FarmTile.java:71)
			}
		}
	}
	
	return false;  // Beta: return false (FarmTile.java:76)
}

bool FarmTile::isNearWater(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FarmTile.isNearWater() - checks if water is nearby (FarmTile.java:79-91)
	for (int_t dx = x - 4; dx <= x + 4; dx++)  // Beta: for (int var5 = var2 - 4; var5 <= var2 + 4; var5++) (FarmTile.java:80)
	{
		for (int_t dy = y; dy <= y + 1; dy++)  // Beta: for (int var6 = var3; var6 <= var3 + 1; var6++) (FarmTile.java:81)
		{
			for (int_t dz = z - 4; dz <= z + 4; dz++)  // Beta: for (int var7 = var4 - 4; var7 <= var4 + 4; var7++) (FarmTile.java:82)
			{
				const Material &mat = level.getMaterial(dx, dy, dz);  // Beta: if (var1.getMaterial(var5, var6, var7) == Material.water) (FarmTile.java:83)
				if (mat == Material::water)  // Beta: Check if material is water (uses operator== from LiquidMaterial.h) (FarmTile.java:83)
				{
					return true;  // Beta: return true (FarmTile.java:84)
				}
			}
		}
	}
	
	return false;  // Beta: return false (FarmTile.java:90)
}
