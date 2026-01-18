#include "world/level/tile/FurnaceTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/entity/FurnaceTileEntity.h"
#include "world/entity/player/Player.h"
#include "world/entity/Mob.h"
#include "client/player/LocalPlayer.h"
#include "java/Random.h"
#include "util/Mth.h"

// Beta: FurnaceTile(int var1, boolean var2) (FurnaceTile.java:16-20)
FurnaceTile::FurnaceTile(int_t id, bool lit) : Tile(id, Material::stone), lit(lit)
{
	tex = 45;  // Beta: this.tex = 45 (FurnaceTile.java:19)
	// Beta: FurnaceTile extends EntityTile, so isEntityTile should be true
	Tile::isEntityTile[id] = true;
}

std::shared_ptr<TileEntity> FurnaceTile::newTileEntity()
{
	// Beta: FurnaceTile.newTileEntity() returns new FurnaceTileEntity() (FurnaceTile.java:134-137)
	return std::make_shared<FurnaceTileEntity>();
}

int_t FurnaceTile::getResource(int_t data, Random &random)
{
	// Beta: FurnaceTile.getResource() - always returns furnace (unlit) ID (FurnaceTile.java:22-25)
	return Tile::furnace.id;  // Beta: return Tile.furnace.id (FurnaceTile.java:24)
}

void FurnaceTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FurnaceTile.onPlace() - calls super and recalcLockDir (FurnaceTile.java:27-31)
	// Beta: EntityTile.onPlace() - creates TileEntity (EntityTile.java:19-22)
	Tile::onPlace(level, x, y, z);  // Beta: super.onPlace(var1, var2, var3, var4) (FurnaceTile.java:28)
	level.setTileEntity(x, y, z, newTileEntity());  // Beta: var1.setTileEntity(var2, var3, var4, this.newTileEntity()) (EntityTile.java:21)
	recalcLockDir(level, x, y, z);  // Beta: this.recalcLockDir(var1, var2, var3, var4) (FurnaceTile.java:30)
	// Note: setPlacedBy() will be called after onPlace() and will override recalcLockDir() when placed by a player
}

// Beta: EntityTile.onRemove() - removes TileEntity (EntityTile.java:24-28)
void FurnaceTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	Tile::onRemove(level, x, y, z);  // Beta: super.onRemove(var1, var2, var3, var4) (EntityTile.java:25)
	level.removeTileEntity(x, y, z);  // Beta: var1.removeTileEntity(var2, var3, var4) (EntityTile.java:26)
}

void FurnaceTile::recalcLockDir(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FurnaceTile.recalcLockDir() - calculates orientation based on adjacent solid blocks (FurnaceTile.java:33-56)
	int_t tileNorth = level.getTile(x, y, z - 1);  // Beta: int var5 = var1.getTile(var2, var3, var4 - 1) (FurnaceTile.java:34)
	int_t tileSouth = level.getTile(x, y, z + 1);  // Beta: int var6 = var1.getTile(var2, var3, var4 + 1) (FurnaceTile.java:35)
	int_t tileWest = level.getTile(x - 1, y, z);   // Beta: int var7 = var1.getTile(var2 - 1, var3, var4) (FurnaceTile.java:36)
	int_t tileEast = level.getTile(x + 1, y, z);   // Beta: int var8 = var1.getTile(var2 + 1, var3, var4) (FurnaceTile.java:37)
	
	byte_t data = 3;  // Beta: byte var9 = 3 (default) (FurnaceTile.java:38)
	
	if (Tile::solid[tileNorth] && !Tile::solid[tileSouth])  // Beta: if (Tile.solid[var5] && !Tile.solid[var6]) (FurnaceTile.java:39)
	{
		data = 3;  // Beta: var9 = 3 (FurnaceTile.java:40) - facing north
	}
	
	if (Tile::solid[tileSouth] && !Tile::solid[tileNorth])  // Beta: if (Tile.solid[var6] && !Tile.solid[var5]) (FurnaceTile.java:43)
	{
		data = 2;  // Beta: var9 = 2 (FurnaceTile.java:44) - facing south
	}
	
	if (Tile::solid[tileWest] && !Tile::solid[tileEast])  // Beta: if (Tile.solid[var7] && !Tile.solid[var8]) (FurnaceTile.java:47)
	{
		data = 5;  // Beta: var9 = 5 (FurnaceTile.java:48) - facing west
	}
	
	if (Tile::solid[tileEast] && !Tile::solid[tileWest])  // Beta: if (Tile.solid[var8] && !Tile.solid[var7]) (FurnaceTile.java:51)
	{
		data = 4;  // Beta: var9 = 4 (FurnaceTile.java:52) - facing east
	}
	
	level.setData(x, y, z, data);  // Beta: var1.setData(var2, var3, var4, var9) (FurnaceTile.java:55)
}

int_t FurnaceTile::getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Alpha 1.2.6: BlockFurnace.getBlockTexture() - top/bottom use stone texture (BlockFurnace.java:50-59)
	// Beta 1.2: FurnaceTile.getTexture() - different textures based on face and lit state (FurnaceTile.java:58-72)
	if (face == Facing::UP || face == Facing::DOWN)  // Alpha: if (var5 == 1) or (var5 == 0) (BlockFurnace.java:51, 53)
	{
		return Tile::rock.tex;  // Alpha: return Block.stone.blockIndexInTexture (BlockFurnace.java:52, 54) - stone texture
	}
	else
	{
		int_t data = level.getData(x, y, z);  // Beta: int var6 = var1.getData(var2, var3, var4) (FurnaceTile.java:65)
		if (face != static_cast<Facing>(data))  // Beta: if (var5 != var6) (FurnaceTile.java:66)
		{
			return tex;  // Beta: return this.tex (FurnaceTile.java:67) - side texture
		}
		else
		{
			return lit ? (tex + 16) : (tex - 1);  // Beta: return this.lit ? this.tex + 16 : this.tex - 1 (FurnaceTile.java:69) - front texture
		}
	}
}

int_t FurnaceTile::getTexture(Facing face, int_t data)
{
	// Alpha 1.2.6: BlockFurnace.getBlockTexture() - top/bottom use stone texture (BlockFurnace.java:50-59)
	// Beta: FurnaceTile.getTexture() - handles item rendering with data (similar to level-based version)
	if (face == Facing::UP || face == Facing::DOWN)  // Alpha: if (var5 == 1) or (var5 == 0) (BlockFurnace.java:51, 53)
	{
		return Tile::rock.tex;  // Alpha: return Block.stone.blockIndexInTexture (BlockFurnace.java:52, 54) - stone texture
	}
	else
	{
		// Beta: Use data to determine which face is the front (FurnaceTile.java:65-69)
		// For item rendering (data=0), default to NORTH (3) as front
		int_t frontFace = (data == 0) ? 3 : data;  // Default to NORTH (3) when data is 0
		if (face != static_cast<Facing>(frontFace))  // Beta: if (var5 != var6) (FurnaceTile.java:66)
		{
			return tex;  // Beta: return this.tex (FurnaceTile.java:67) - side texture
		}
		else
		{
			return lit ? (tex + 16) : (tex - 1);  // Beta: return this.lit ? this.tex + 16 : this.tex - 1 (FurnaceTile.java:69) - front texture
		}
	}
}

int_t FurnaceTile::getTexture(Facing face)
{
	// Alpha 1.2.6: BlockFurnace.getBlockTextureFromSide() - top/bottom use stone texture (BlockFurnace.java:86-88)
	// Beta 1.2: FurnaceTile.getTexture() - overload for Facing only (FurnaceTile.java:99-108)
	// When rendering as item without data, default to NORTH (face 3) as front
	if (face == Facing::UP || face == Facing::DOWN)  // Alpha: if (var1 == 1) or (var1 == 0) (BlockFurnace.java:87)
	{
		return Tile::rock.tex;  // Alpha: return Block.stone.blockID (BlockFurnace.java:87) - stone texture
	}
	else
	{
		return (face == Facing::NORTH) ? (tex - 1) : tex;  // Beta: return var1 == 3 ? this.tex - 1 : this.tex (FurnaceTile.java:106)
	}
}

void FurnaceTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Alpha 1.2.6: BlockFurnace.randomDisplayTick() - particles when lit (BlockFurnace.java:61-82)
	if (lit)  // Alpha: if (this.isActive) (BlockFurnace.java:62)
	{
		int_t data = level.getData(x, y, z);  // Alpha: int var6 = var1.getBlockMetadata(var2, var3, var4) (BlockFurnace.java:63)
		double fx = (double)x + 0.5;  // Alpha: float var7 = (float)var2 + 0.5F (BlockFurnace.java:64) - cast to double for addParticle
		double fy = (double)y + 0.0 + (double)(random.nextFloat() * 6.0f / 16.0f);  // Alpha: float var8 = (float)var3 + 0.0F + var5.nextFloat() * 6.0F / 16.0F (BlockFurnace.java:65)
		double fz = (double)z + 0.5;  // Alpha: float var9 = (float)var4 + 0.5F (BlockFurnace.java:66)
		double offset = 0.52;  // Alpha: float var10 = 0.52F (BlockFurnace.java:67)
		double randomOffset = (double)(random.nextFloat() * 0.6f - 0.3f);  // Alpha: float var11 = var5.nextFloat() * 0.6F - 0.3F (BlockFurnace.java:68)
		
		// Alpha: Particle positions based on facing direction (BlockFurnace.java:69-81)
		if (data == 4)  // Alpha: if (var6 == 4) - facing east (BlockFurnace.java:69)
		{
			level.addParticle(u"smoke", fx - offset, fy, fz + randomOffset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("smoke", (double)(var7 - var10), (double)var8, (double)(var9 + var11), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:70)
			level.addParticle(u"flame", fx - offset, fy, fz + randomOffset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("flame", (double)(var7 - var10), (double)var8, (double)(var9 + var11), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:71)
		}
		else if (data == 5)  // Alpha: else if (var6 == 5) - facing west (BlockFurnace.java:72)
		{
			level.addParticle(u"smoke", fx + offset, fy, fz + randomOffset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("smoke", (double)(var7 + var10), (double)var8, (double)(var9 + var11), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:73)
			level.addParticle(u"flame", fx + offset, fy, fz + randomOffset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("flame", (double)(var7 + var10), (double)var8, (double)(var9 + var11), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:74)
		}
		else if (data == 2)  // Alpha: else if (var6 == 2) - facing south (BlockFurnace.java:75)
		{
			level.addParticle(u"smoke", fx + randomOffset, fy, fz - offset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("smoke", (double)(var7 + var11), (double)var8, (double)(var9 - var10), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:76)
			level.addParticle(u"flame", fx + randomOffset, fy, fz - offset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("flame", (double)(var7 + var11), (double)var8, (double)(var9 - var10), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:77)
		}
		else if (data == 3)  // Alpha: else if (var6 == 3) - facing north (BlockFurnace.java:78)
		{
			level.addParticle(u"smoke", fx + randomOffset, fy, fz + offset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("smoke", (double)(var7 + var11), (double)var8, (double)(var9 + var10), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:79)
			level.addParticle(u"flame", fx + randomOffset, fy, fz + offset, 0.0, 0.0, 0.0);  // Alpha: var1.spawnParticle("flame", (double)(var7 + var11), (double)var8, (double)(var9 + var10), 0.0D, 0.0D, 0.0D) (BlockFurnace.java:80)
		}
	}
}

bool FurnaceTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: FurnaceTile.use() - opens furnace GUI (FurnaceTile.java:110-119)
	if (level.isOnline)  // Beta: if (var1.isOnline) (FurnaceTile.java:112)
	{
		return true;  // Beta: return true (FurnaceTile.java:113)
	}
	else
	{
		// Beta: Get TileEntity and open furnace (FurnaceTile.java:115-117)
		std::shared_ptr<TileEntity> te = level.getTileEntity(x, y, z);
		std::shared_ptr<FurnaceTileEntity> furnaceEntity = std::dynamic_pointer_cast<FurnaceTileEntity>(te);
		if (furnaceEntity != nullptr)
		{
			LocalPlayer *localPlayer = dynamic_cast<LocalPlayer*>(&player);
			if (localPlayer != nullptr)
			{
				localPlayer->openFurnace(furnaceEntity);
				return true;
			}
		}
		return false;
	}
}

void FurnaceTile::setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob)
{
	// Alpha 1.2.6: BlockFurnace.onBlockPlacedBy() - sets orientation based on player rotation (BlockFurnace.java:117-135)
	// Java: int var6 = MathHelper.floor_double((double)(var5.rotationYaw * 4.0F / 360.0F) + 0.5D) & 3;
	// Note: Alpha 1.2.6 uses MathHelper.floor_double (takes double), not Mth.floor (takes float)
	// Cast to double first, then floor, to match Java exactly
	int_t direction = Mth::floor((double)(mob.yRot * 4.0f / 360.0f) + 0.5) & 3;
	
	// Beta: Set orientation based on player rotation - this ALWAYS faces the player, overriding recalcLockDir()
	// Use setData (not setDataNoUpdate) to match Beta exactly
	if (direction == 0)  // Beta: if (var6 == 0) (FurnaceTile.java:142)
	{
		level.setData(x, y, z, 2);  // Beta: var1.setData(var2, var3, var4, 2) (FurnaceTile.java:143) - facing south
	}
	
	if (direction == 1)  // Beta: if (var6 == 1) (FurnaceTile.java:146)
	{
		level.setData(x, y, z, 5);  // Beta: var1.setData(var2, var3, var4, 5) (FurnaceTile.java:147) - facing west
	}
	
	if (direction == 2)  // Beta: if (var6 == 2) (FurnaceTile.java:150)
	{
		level.setData(x, y, z, 3);  // Beta: var1.setData(var2, var3, var4, 3) (FurnaceTile.java:151) - facing north
	}
	
	if (direction == 3)  // Beta: if (var6 == 3) (FurnaceTile.java:154)
	{
		level.setData(x, y, z, 4);  // Beta: var1.setData(var2, var3, var4, 4) (FurnaceTile.java:155) - facing east
	}
}

void FurnaceTile::setLit(bool lit, Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FurnaceTile.setLit() - static method to toggle between lit/unlit (FurnaceTile.java:121-132)
	int_t data = level.getData(x, y, z);  // Beta: int var5 = var1.getData(var2, var3, var4) (FurnaceTile.java:122)
	std::shared_ptr<TileEntity> tileEntity = level.getTileEntity(x, y, z);  // Beta: TileEntity var6 = var1.getTileEntity(var2, var3, var4) (FurnaceTile.java:123)
	
	if (lit)  // Beta: if (var0) (FurnaceTile.java:124)
	{
		level.setTile(x, y, z, 62);  // Beta: var1.setTile(var2, var3, var4, Tile.furnace_lit.id) (FurnaceTile.java:125) - furnace_lit ID 62
	}
	else
	{
		level.setTile(x, y, z, 61);  // Beta: var1.setTile(var2, var3, var4, Tile.furnace.id) (FurnaceTile.java:127) - furnace ID 61
	}
	
	level.setData(x, y, z, data);  // Beta: var1.setData(var2, var3, var4, var5) (FurnaceTile.java:130)
	level.setTileEntity(x, y, z, tileEntity);  // Beta: var1.setTileEntity(var2, var3, var4, var6) (FurnaceTile.java:131)
}
