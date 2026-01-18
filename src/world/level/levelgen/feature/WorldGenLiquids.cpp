#include "world/level/levelgen/feature/WorldGenLiquids.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "java/Random.h"
#include <iostream>

// Beta 1.2: SpringFeature.java - ported 1:1
WorldGenLiquids::WorldGenLiquids(int_t blockId) : liquidBlockId(blockId)
{
}

bool WorldGenLiquids::generate(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Beta: SpringFeature.java:15-66
	// Beta: line 16 - Must have stone above
	if (level.getTile(x, y + 1, z) != Tile::rock.id)
	{
		return false;
	}
	
	// Beta: line 18 - Must have stone below
	if (level.getTile(x, y - 1, z) != Tile::rock.id)
	{
		return false;
	}
	
	// Beta: line 20 - Center must be air or stone
	int_t centerTile = level.getTile(x, y, z);
	if (centerTile != 0 && centerTile != Tile::rock.id)
	{
		return false;
	}
	
	// Beta: lines 23-38 - Count stone neighbors (must be exactly 3)
	int_t stoneCount = 0;
	if (level.getTile(x - 1, y, z) == Tile::rock.id) ++stoneCount;
	if (level.getTile(x + 1, y, z) == Tile::rock.id) ++stoneCount;
	if (level.getTile(x, y, z - 1) == Tile::rock.id) ++stoneCount;
	if (level.getTile(x, y, z + 1) == Tile::rock.id) ++stoneCount;
	
	// Beta: lines 40-55 - Count air neighbors (must be exactly 1)
	int_t airCount = 0;
	if (level.isEmptyTile(x - 1, y, z)) ++airCount;
	if (level.isEmptyTile(x + 1, y, z)) ++airCount;
	if (level.isEmptyTile(x, y, z - 1)) ++airCount;
	if (level.isEmptyTile(x, y, z + 1)) ++airCount;
	
	// Beta: line 57 - Only place if exactly 3 stone neighbors and 1 air neighbor
	if (stoneCount == 3 && airCount == 1)
	{
		// Beta: line 58 - Place liquid block
		level.setTile(x, y, z, liquidBlockId);
		
		// Beta: lines 59-61 - Trigger fluid update tick directly
		// Beta: var1.instaTick = true; Tile.tiles[this.tile].tick(...); var1.instaTick = false;
		// This is equivalent to our noNeighborUpdate flag
		bool oldNoNeighborUpdate = level.noNeighborUpdate;
		level.noNeighborUpdate = true;
		Tile *tile = Tile::tiles[liquidBlockId];
		if (tile != nullptr)
		{
			tile->tick(level, x, y, z, random);
		}
		level.noNeighborUpdate = oldNoNeighborUpdate;
		
		return true;
	}
	
	return false;
}
