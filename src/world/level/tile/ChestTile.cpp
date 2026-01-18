#include "world/level/tile/ChestTile.h"
#include "world/level/material/Material.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/LevelSource.h"
#include "world/level/Level.h"
#include "world/entity/player/Player.h"
#include "client/player/LocalPlayer.h"
#include "world/CompoundContainer.h"

// Beta: ChestTile(id) - texture 26, Material.wood, hardness 2.5F
ChestTile::ChestTile(int_t id) : Tile(id, 26, Material::wood)
{
	setDestroyTime(2.5f);
	// Beta: ChestTile extends EntityTile, so isEntityTile should be true
	Tile::isEntityTile[id] = true;
}

std::shared_ptr<TileEntity> ChestTile::newTileEntity()
{
	// Beta: ChestTile.newTileEntity() returns new ChestTileEntity()
	return std::make_shared<ChestTileEntity>();
}

// Beta: EntityTile.onPlace() - creates TileEntity (EntityTile.java:19-22)
void ChestTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	Tile::onPlace(level, x, y, z);  // Beta: super.onPlace(var1, var2, var3, var4) (EntityTile.java:20)
	level.setTileEntity(x, y, z, newTileEntity());  // Beta: var1.setTileEntity(var2, var3, var4, this.newTileEntity()) (EntityTile.java:21)
}

// Beta: EntityTile.onRemove() - removes TileEntity (EntityTile.java:24-28)
void ChestTile::onRemove(Level &level, int_t x, int_t y, int_t z)
{
	Tile::onRemove(level, x, y, z);  // Beta: super.onRemove(var1, var2, var3, var4) (EntityTile.java:25)
	level.removeTileEntity(x, y, z);  // Beta: var1.removeTileEntity(var2, var3, var4) (EntityTile.java:26)
}

int_t ChestTile::getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: ChestTile.getTexture() - complex texture logic for chest faces (ChestTile.java:23-105)
	if (face == Facing::UP)  // Beta: if (var5 == 1) (ChestTile.java:25)
	{
		return tex - 1;  // Beta: return this.tex - 1 (ChestTile.java:26) - top texture
	}
	else if (face == Facing::DOWN)  // Beta: else if (var5 == 0) (ChestTile.java:27)
	{
		return tex - 1;  // Beta: return this.tex - 1 (ChestTile.java:28) - bottom texture
	}
	else
	{
		int_t tileN = level.getTile(x, y, z - 1);  // Beta: int var6 = var1.getTile(var2, var3, var4 - 1) (ChestTile.java:30)
		int_t tileS = level.getTile(x, y, z + 1);  // Beta: int var7 = var1.getTile(var2, var3, var4 + 1) (ChestTile.java:31)
		int_t tileW = level.getTile(x - 1, y, z);  // Beta: int var8 = var1.getTile(var2 - 1, var3, var4) (ChestTile.java:32)
		int_t tileE = level.getTile(x + 1, y, z);  // Beta: int var9 = var1.getTile(var2 + 1, var3, var4) (ChestTile.java:33)
		
		if (tileN != id && tileS != id)  // Beta: if (var6 != this.id && var7 != this.id) (ChestTile.java:34)
		{
			if (tileW != id && tileE != id)  // Beta: if (var8 != this.id && var9 != this.id) (ChestTile.java:35)
			{
				// Single chest - determine facing based on adjacent solid blocks
				byte_t facing = 3;  // Beta: byte var15 = 3 (ChestTile.java:36) - default to north
				if (Tile::solid[tileN] && !Tile::solid[tileS])  // Beta: if (Tile.solid[var6] && !Tile.solid[var7]) (ChestTile.java:37)
				{
					facing = 3;  // Beta: var15 = 3 (ChestTile.java:38) - facing north
				}
				
				if (Tile::solid[tileS] && !Tile::solid[tileN])  // Beta: if (Tile.solid[var7] && !Tile.solid[var6]) (ChestTile.java:41)
				{
					facing = 2;  // Beta: var15 = 2 (ChestTile.java:42) - facing south
				}
				
				if (Tile::solid[tileW] && !Tile::solid[tileE])  // Beta: if (Tile.solid[var8] && !Tile.solid[var9]) (ChestTile.java:45)
				{
					facing = 5;  // Beta: var15 = 5 (ChestTile.java:46) - facing west
				}
				
				if (Tile::solid[tileE] && !Tile::solid[tileW])  // Beta: if (Tile.solid[var9] && !Tile.solid[var8]) (ChestTile.java:49)
				{
					facing = 4;  // Beta: var15 = 4 (ChestTile.java:50) - facing east
				}
				
				return (static_cast<int_t>(face) == facing) ? (tex + 1) : tex;  // Beta: return var5 == var15 ? this.tex + 1 : this.tex (ChestTile.java:53)
			}
			else if (face != Facing::EAST && face != Facing::WEST)  // Beta: else if (var5 != 4 && var5 != 5) (ChestTile.java:54)
			{
				// Double chest with chest on west or east side
				int_t offset = 0;  // Beta: int var14 = 0 (ChestTile.java:55)
				if (tileW == id)  // Beta: if (var8 == this.id) (ChestTile.java:56)
				{
					offset = -1;  // Beta: var14 = -1 (ChestTile.java:57)
				}
				
				int_t tileNW = level.getTile(tileW == id ? x - 1 : x + 1, y, z - 1);  // Beta: int var16 = var1.getTile(var8 == this.id ? var2 - 1 : var2 + 1, var3, var4 - 1) (ChestTile.java:60)
				int_t tileSW = level.getTile(tileW == id ? x - 1 : x + 1, y, z + 1);  // Beta: int var17 = var1.getTile(var8 == this.id ? var2 - 1 : var2 + 1, var3, var4 + 1) (ChestTile.java:61)
				if (face == Facing::SOUTH)  // Beta: if (var5 == 3) (ChestTile.java:62) - SOUTH = 3 in Java
				{
					offset = -1 - offset;  // Beta: var14 = -1 - var14 (ChestTile.java:63)
				}
				
				byte_t facing = 3;  // Beta: byte var18 = 3 (ChestTile.java:66)
				if ((Tile::solid[tileN] || Tile::solid[tileNW]) && !Tile::solid[tileS] && !Tile::solid[tileSW])  // Beta: if ((Tile.solid[var6] || Tile.solid[var16]) && !Tile.solid[var7] && !Tile.solid[var17]) (ChestTile.java:67)
				{
					facing = 3;  // Beta: var18 = 3 (ChestTile.java:68) - facing north
				}
				
				if ((Tile::solid[tileS] || Tile::solid[tileSW]) && !Tile::solid[tileN] && !Tile::solid[tileNW])  // Beta: if ((Tile.solid[var7] || Tile.solid[var17]) && !Tile.solid[var6] && !Tile.solid[var16]) (ChestTile.java:71)
				{
					facing = 2;  // Beta: var18 = 2 (ChestTile.java:72) - facing south
				}
				
				return (static_cast<int_t>(face) == facing ? (tex + 16) : (tex + 32)) + offset;  // Beta: return (var5 == var18 ? this.tex + 16 : this.tex + 32) + var14 (ChestTile.java:75)
			}
			else
			{
				return tex;  // Beta: return this.tex (ChestTile.java:77) - side texture
			}
		}
		else if (face != Facing::SOUTH && face != Facing::NORTH)  // Beta: else if (var5 != 2 && var5 != 3) (ChestTile.java:79)
		{
			// Double chest with chest on north or south side
			int_t offset = 0;  // Beta: int var10 = 0 (ChestTile.java:80)
			if (tileN == id)  // Beta: if (var6 == this.id) (ChestTile.java:81)
			{
				offset = -1;  // Beta: var10 = -1 (ChestTile.java:82)
			}
			
			int_t tileNE = level.getTile(x - 1, y, tileN == id ? z - 1 : z + 1);  // Beta: int var11 = var1.getTile(var2 - 1, var3, var6 == this.id ? var4 - 1 : var4 + 1) (ChestTile.java:85)
			int_t tileSE = level.getTile(x + 1, y, tileN == id ? z - 1 : z + 1);  // Beta: int var12 = var1.getTile(var2 + 1, var3, var6 == this.id ? var4 - 1 : var4 + 1) (ChestTile.java:86)
			if (face == Facing::WEST)  // Beta: if (var5 == 4) (ChestTile.java:87) - WEST = 4 in Java
			{
				offset = -1 - offset;  // Beta: var10 = -1 - var10 (ChestTile.java:88)
			}
			
			byte_t facing = 5;  // Beta: byte var13 = 5 (ChestTile.java:91)
			if ((Tile::solid[tileW] || Tile::solid[tileNE]) && !Tile::solid[tileE] && !Tile::solid[tileSE])  // Beta: if ((Tile.solid[var8] || Tile.solid[var11]) && !Tile.solid[var9] && !Tile.solid[var12]) (ChestTile.java:92)
			{
				facing = 5;  // Beta: var13 = 5 (ChestTile.java:93) - facing west
			}
			
			if ((Tile::solid[tileE] || Tile::solid[tileSE]) && !Tile::solid[tileW] && !Tile::solid[tileNE])  // Beta: if ((Tile.solid[var9] || Tile.solid[var12]) && !Tile.solid[var8] && !Tile.solid[var11]) (ChestTile.java:96)
			{
				facing = 4;  // Beta: var13 = 4 (ChestTile.java:97) - facing east
			}
			
			return (static_cast<int_t>(face) == facing ? (tex + 16) : (tex + 32)) + offset;  // Beta: return (var5 == var13 ? this.tex + 16 : this.tex + 32) + var10 (ChestTile.java:100)
		}
		else
		{
			return tex;  // Beta: return this.tex (ChestTile.java:102) - side texture
		}
	}
}

int_t ChestTile::getTexture(Facing face)
{
	// Beta: ChestTile.getTexture() - overload for Facing only (ChestTile.java:107-116)
	if (face == Facing::UP)  // Beta: if (var1 == 1) (ChestTile.java:109)
	{
		return tex - 1;  // Beta: return this.tex - 1 (ChestTile.java:110) - top texture
	}
	else if (face == Facing::DOWN)  // Beta: else if (var1 == 0) (ChestTile.java:111)
	{
		return tex - 1;  // Beta: return this.tex - 1 (ChestTile.java:112) - bottom texture
	}
	else
	{
		return (face == Facing::NORTH) ? (tex + 1) : tex;  // Beta: return var1 == 3 ? this.tex + 1 : this.tex (ChestTile.java:114) - front (north) or side
	}
}

// Beta: ChestTile.use() - opens chest interface (ChestTile.java:192-228)
bool ChestTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: Check if chest is blocked above (ChestTile.java:194-203)
	if (level.isSolidTile(x, y + 1, z))
		return true;
	if (level.getTile(x - 1, y, z) == id && level.isSolidTile(x - 1, y + 1, z))
		return true;
	if (level.getTile(x + 1, y, z) == id && level.isSolidTile(x + 1, y + 1, z))
		return true;
	if (level.getTile(x, y, z - 1) == id && level.isSolidTile(x, y + 1, z - 1))
		return true;
	if (level.getTile(x, y, z + 1) == id && level.isSolidTile(x, y + 1, z + 1))
		return true;
	
	// Beta: Get chest tile entity (ChestTile.java:193)
	std::shared_ptr<TileEntity> te = level.getTileEntity(x, y, z);
	std::shared_ptr<ChestTileEntity> chestEntity = std::dynamic_pointer_cast<ChestTileEntity>(te);
	
	// Beta: Handle double chests with CompoundContainer (ChestTile.java:205-219)
	std::shared_ptr<CompoundContainer> compoundContainer = nullptr;
	
	if (chestEntity != nullptr)
	{
		// Beta: Check for adjacent chests and combine them (ChestTile.java:205-219)
		if (level.getTile(x - 1, y, z) == id)
		{
			std::shared_ptr<TileEntity> te2 = level.getTileEntity(x - 1, y, z);
			std::shared_ptr<ChestTileEntity> chest2 = std::dynamic_pointer_cast<ChestTileEntity>(te2);
			if (chest2)
			{
				// Beta: var6 = new CompoundContainer("Large chest", (ChestTileEntity)var1.getTileEntity(var2 - 1, var3, var4), (Container)var6) (ChestTile.java:206)
				compoundContainer = std::make_shared<CompoundContainer>(u"Large chest", chest2, chestEntity);
				chestEntity = nullptr;  // Use compoundContainer instead
			}
		}
		
		if (level.getTile(x + 1, y, z) == id && compoundContainer == nullptr)
		{
			std::shared_ptr<TileEntity> te2 = level.getTileEntity(x + 1, y, z);
			std::shared_ptr<ChestTileEntity> chest2 = std::dynamic_pointer_cast<ChestTileEntity>(te2);
			if (chest2)
			{
				// Beta: var6 = new CompoundContainer("Large chest", (Container)var6, (ChestTileEntity)var1.getTileEntity(var2 + 1, var3, var4)) (ChestTile.java:209)
				compoundContainer = std::make_shared<CompoundContainer>(u"Large chest", chestEntity, chest2);
				chestEntity = nullptr;  // Use compoundContainer instead
			}
		}
		
		if (level.getTile(x, y, z - 1) == id && compoundContainer == nullptr)
		{
			std::shared_ptr<TileEntity> te2 = level.getTileEntity(x, y, z - 1);
			std::shared_ptr<ChestTileEntity> chest2 = std::dynamic_pointer_cast<ChestTileEntity>(te2);
			if (chest2)
			{
				// Beta: var6 = new CompoundContainer("Large chest", (ChestTileEntity)var1.getTileEntity(var2, var3, var4 - 1), (Container)var6) (ChestTile.java:213)
				compoundContainer = std::make_shared<CompoundContainer>(u"Large chest", chest2, chestEntity);
				chestEntity = nullptr;  // Use compoundContainer instead
			}
		}
		
		if (level.getTile(x, y, z + 1) == id && compoundContainer == nullptr)
		{
			std::shared_ptr<TileEntity> te2 = level.getTileEntity(x, y, z + 1);
			std::shared_ptr<ChestTileEntity> chest2 = std::dynamic_pointer_cast<ChestTileEntity>(te2);
			if (chest2)
			{
				// Beta: var6 = new CompoundContainer("Large chest", (Container)var6, (ChestTileEntity)var1.getTileEntity(var2, var3, var4 + 1)) (ChestTile.java:217)
				compoundContainer = std::make_shared<CompoundContainer>(u"Large chest", chestEntity, chest2);
				chestEntity = nullptr;  // Use compoundContainer instead
			}
		}
	}
	
	if (level.isOnline)
	{
		return true;  // Beta: if (var1.isOnline) return true (ChestTile.java:221)
	}
	else
	{
		// Beta: var5.openContainer((Container)var6) (ChestTile.java:224)
		LocalPlayer *localPlayer = dynamic_cast<LocalPlayer*>(&player);
		if (localPlayer != nullptr)
		{
			if (compoundContainer != nullptr)
			{
				// Open double chest
				localPlayer->openContainer(compoundContainer);
				return true;
			}
			else if (chestEntity != nullptr)
			{
				// Open single chest
				localPlayer->openContainer(chestEntity);
				return true;
			}
		}
		return false;
	}
}
