#include "world/level/tile/TorchTile.h"
#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/phys/AABB.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "java/Random.h"
#include "util/Mth.h"

// Beta: Tile.torch = new TorchTile(50, 80).setDestroyTime(0.0F).setLightEmission(0.9375F).setSoundType(SOUND_WOOD)
TorchTile::TorchTile(int_t id, int_t tex) : Tile(id, tex, Material::decoration)
{
	setDestroyTime(0.0f);
	setLightEmission((int_t)(0.9375f * 15.0f));  // Beta: setLightEmission(0.9375F) -> 0.9375 * 15 = 14
	setTicking(true);
}

AABB *TorchTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: TorchTile.getAABB() returns null (no collision) (TorchTile.java:17-19)
	return nullptr;
}

bool TorchTile::isSolidRender()
{
	// Beta: TorchTile.isSolidRender() returns false (TorchTile.java:26-28)
	return false;
}

bool TorchTile::isCubeShaped()
{
	// Beta: TorchTile.isCubeShaped() returns false (TorchTile.java:31-33)
	return false;
}

Tile::Shape TorchTile::getRenderShape()
{
	// Beta: TorchTile.getRenderShape() returns 2 (TorchTile.java:36-38)
	// Shape 2 = SHAPE_TORCH (torch rendering)
	return SHAPE_TORCH;
}

bool TorchTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: TorchTile.mayPlace() - checks if can be placed on solid block (TorchTile.java:41-51)
	if (level.isSolidTile(x - 1, y, z))
		return true;
	if (level.isSolidTile(x + 1, y, z))
		return true;
	if (level.isSolidTile(x, y, z - 1))
		return true;
	if (level.isSolidTile(x, y, z + 1))
		return true;
	return level.isSolidTile(x, y - 1, z);
}

void TorchTile::setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Alpha 1.2.6: BlockTorch.onBlockPlaced() - sets orientation based on face (BlockTorch.java:31-54)
	// When placing on a face, check if the block on the OPPOSITE side is solid
	int_t data = level.getData(x, y, z);
	if (face == Facing::UP && level.isSolidTile(x, y - 1, z))
	{
		data = 5;  // Alpha: var6 = 5 (on top of block)
	}
	else if (face == Facing::NORTH && level.isSolidTile(x, y, z + 1))  // Alpha: var5 == 2 && isBlockOpaqueCube(var2, var3, var4 + 1)
	{
		data = 4;  // Alpha: var6 = 4 (torch on north face, facing south)
	}
	else if (face == Facing::SOUTH && level.isSolidTile(x, y, z - 1))  // Alpha: var5 == 3 && isBlockOpaqueCube(var2, var3, var4 - 1)
	{
		data = 3;  // Alpha: var6 = 3 (torch on south face, facing north)
	}
	else if (face == Facing::WEST && level.isSolidTile(x + 1, y, z))  // Alpha: var5 == 4 && isBlockOpaqueCube(var2 + 1, var3, var4)
	{
		data = 2;  // Alpha: var6 = 2 (torch on west face, facing east)
	}
	else if (face == Facing::EAST && level.isSolidTile(x - 1, y, z))  // Alpha: var5 == 5 && isBlockOpaqueCube(var2 - 1, var3, var4)
	{
		data = 1;  // Alpha: var6 = 1 (torch on east face, facing west)
	}
	level.setData(x, y, z, data);
}

void TorchTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: TorchTile.tick() - calls onPlace if data is 0 (TorchTile.java:80-85)
	Tile::tick(level, x, y, z, random);
	if (level.getData(x, y, z) == 0)
	{
		onPlace(level, x, y, z);
	}
}

void TorchTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: TorchTile.onPlace() - sets orientation based on adjacent solid blocks (TorchTile.java:88-102)
	if (level.isSolidTile(x - 1, y, z))
	{
		level.setData(x, y, z, 1);  // Beta: var1.setData(var2, var3, var4, 1)
	}
	else if (level.isSolidTile(x + 1, y, z))
	{
		level.setData(x, y, z, 2);  // Beta: var1.setData(var2, var3, var4, 2)
	}
	else if (level.isSolidTile(x, y, z - 1))
	{
		level.setData(x, y, z, 3);  // Beta: var1.setData(var2, var3, var4, 3)
	}
	else if (level.isSolidTile(x, y, z + 1))
	{
		level.setData(x, y, z, 4);  // Beta: var1.setData(var2, var3, var4, 4)
	}
	else if (level.isSolidTile(x, y - 1, z))
	{
		level.setData(x, y, z, 5);  // Beta: var1.setData(var2, var3, var4, 5)
	}
	
	checkCanSurvive(level, x, y, z);
}

void TorchTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: TorchTile.neighborChanged() - checks if can survive, drops if not (TorchTile.java:105-134)
	if (checkCanSurvive(level, x, y, z))
	{
		int_t data = level.getData(x, y, z);
		bool shouldDrop = false;
		
		if (!level.isSolidTile(x - 1, y, z) && data == 1)
			shouldDrop = true;
		if (!level.isSolidTile(x + 1, y, z) && data == 2)
			shouldDrop = true;
		if (!level.isSolidTile(x, y, z - 1) && data == 3)
			shouldDrop = true;
		if (!level.isSolidTile(x, y, z + 1) && data == 4)
			shouldDrop = true;
		if (!level.isSolidTile(x, y - 1, z) && data == 5)
			shouldDrop = true;
		
		if (shouldDrop)
		{
			spawnResources(level, x, y, z, level.getData(x, y, z));
			level.setTile(x, y, z, 0);
		}
	}
}

bool TorchTile::checkCanSurvive(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: TorchTile.checkCanSurvive() - checks if can survive, drops if not (TorchTile.java:136-144)
	if (!mayPlace(level, x, y, z))
	{
		spawnResources(level, x, y, z, level.getData(x, y, z));
		level.setTile(x, y, z, 0);
		return false;
	}
	return true;
}

HitResult TorchTile::clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to)
{
	// Beta: TorchTile.clip() - custom hitbox based on orientation (TorchTile.java:147-164)
	int_t data = level.getData(x, y, z) & 7;  // Beta: var7 = var1.getData(var2, var3, var4) & 7
	float size = 0.15f;  // Beta: var8 = 0.15F
	
	if (data == 1)  // West face
	{
		setShape(0.0f, 0.2f, 0.5f - size, size * 2.0f, 0.8f, 0.5f + size);
	}
	else if (data == 2)  // East face
	{
		setShape(1.0f - size * 2.0f, 0.2f, 0.5f - size, 1.0f, 0.8f, 0.5f + size);
	}
	else if (data == 3)  // North face
	{
		setShape(0.5f - size, 0.2f, 0.0f, 0.5f + size, 0.8f, size * 2.0f);
	}
	else if (data == 4)  // South face
	{
		setShape(0.5f - size, 0.2f, 1.0f - size * 2.0f, 0.5f + size, 0.8f, 1.0f);
	}
	else  // On top (data == 5 or 0)
	{
		size = 0.1f;  // Beta: var8 = 0.1F
		setShape(0.5f - size, 0.0f, 0.5f - size, 0.5f + size, 0.6f, 0.5f + size);
	}
	
	return Tile::clip(level, x, y, z, from, to);
}

void TorchTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// newb12: TorchTile.animateTick() - adds smoke and flame particles (TorchTile.java:167-190)
	// Ported 1:1 from newb12/net/minecraft/world/level/tile/TorchTile.java
	int_t data = level.getData(x, y, z);  // newb12: int var6 = var1.getData(var2, var3, var4)
	double px = (double)x + 0.5;  // newb12: double var7 = var2 + 0.5F
	double py = (double)y + 0.7;  // newb12: double var9 = var3 + 0.7F
	double pz = (double)z + 0.5;  // newb12: double var11 = var4 + 0.5F
	double var13 = 0.22;  // newb12: double var13 = 0.22F
	double var15 = 0.27;  // newb12: double var15 = 0.27F
	
	if (data == 1)  // newb12: if (var6 == 1) - West face
	{
		level.addParticle(u"smoke", px - var15, py + var13, pz, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("smoke", var7 - var15, var9 + var13, var11, 0.0, 0.0, 0.0)
		level.addParticle(u"flame", px - var15, py + var13, pz, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("flame", var7 - var15, var9 + var13, var11, 0.0, 0.0, 0.0)
	}
	else if (data == 2)  // newb12: else if (var6 == 2) - East face
	{
		level.addParticle(u"smoke", px + var15, py + var13, pz, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("smoke", var7 + var15, var9 + var13, var11, 0.0, 0.0, 0.0)
		level.addParticle(u"flame", px + var15, py + var13, pz, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("flame", var7 + var15, var9 + var13, var11, 0.0, 0.0, 0.0)
	}
	else if (data == 3)  // newb12: else if (var6 == 3) - North face
	{
		level.addParticle(u"smoke", px, py + var13, pz - var15, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("smoke", var7, var9 + var13, var11 - var15, 0.0, 0.0, 0.0)
		level.addParticle(u"flame", px, py + var13, pz - var15, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("flame", var7, var9 + var13, var11 - var15, 0.0, 0.0, 0.0)
	}
	else if (data == 4)  // newb12: else if (var6 == 4) - South face
	{
		level.addParticle(u"smoke", px, py + var13, pz + var15, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("smoke", var7, var9 + var13, var11 + var15, 0.0, 0.0, 0.0)
		level.addParticle(u"flame", px, py + var13, pz + var15, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("flame", var7, var9 + var13, var11 + var15, 0.0, 0.0, 0.0)
	}
	else  // newb12: else - On top (data == 5 or 0)
	{
		level.addParticle(u"smoke", px, py, pz, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("smoke", var7, var9, var11, 0.0, 0.0, 0.0)
		level.addParticle(u"flame", px, py, pz, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("flame", var7, var9, var11, 0.0, 0.0, 0.0)
	}
}
