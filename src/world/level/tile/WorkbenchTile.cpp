#include "world/level/tile/WorkbenchTile.h"
#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/WoodTile.h"
#include "world/entity/player/Player.h"
#include "client/player/LocalPlayer.h"
#include "Facing.h"

// Beta: Tile.workBench = new WorkbenchTile(58).setDestroyTime(2.5F).setSoundType(SOUND_WOOD)
WorkbenchTile::WorkbenchTile(int_t id) : Tile(id, Material::wood)
{
	tex = 59;  // Beta: this.tex = 59 (WorkbenchTile.java:10)
	setDestroyTime(2.5f);  // Beta: setDestroyTime(2.5F)
	// Sound type is handled by Material::wood
}

int_t WorkbenchTile::getTexture(Facing face, int_t data)
{
	// Beta: WorkbenchTile.getTexture() - different textures for different faces (WorkbenchTile.java:14-22)
	if (face == Facing::UP)  // Face 1
	{
		return tex - 16;  // Beta: return this.tex - 16 (texture 43)
	}
	else if (face == Facing::DOWN)  // Face 0
	{
		return Tile::wood.getTexture(Facing::DOWN);  // Beta: return Tile.wood.getTexture(0)
	}
	else if (face == Facing::NORTH || face == Facing::SOUTH)  // Face 2 or 4
	{
		return tex + 1;  // Beta: return this.tex + 1 (texture 60)
	}
	else  // Face 3 or 5 (EAST or WEST)
	{
		return tex;  // Beta: return this.tex (texture 59)
	}
}

bool WorkbenchTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: WorkbenchTile.use() - opens crafting interface (WorkbenchTile.java:25-32)
	if (level.isOnline)
	{
		return true;  // Beta: if (var1.isOnline) return true
	}
	else
	{
		// Beta: var5.startCrafting(var2, var3, var4) (WorkbenchTile.java:31)
		LocalPlayer *localPlayer = dynamic_cast<LocalPlayer*>(&player);
		if (localPlayer != nullptr)
		{
			localPlayer->startCrafting(x, y, z);
			return true;
		}
		return false;
	}
}
