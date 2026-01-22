#include "world/level/tile/PumpkinTile.h"
#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/entity/Mob.h"
#include "util/Mth.h"
#include "Facing.h"

// Beta: PumpkinTile.java
PumpkinTile::PumpkinTile(int_t id, int_t texture, bool lit) : Tile(id, Material::vegetable), lit(lit)
{
	tex = texture;  // Beta: this.tex = var2 (PumpkinTile.java:13)
	setTicking(true);  // Beta: this.setTicking(true) (PumpkinTile.java:14)
}

int_t PumpkinTile::getTexture(Facing face, int_t data)
{
	// Beta: PumpkinTile.getTexture(int var1, int var2) (PumpkinTile.java:19-40)
	if (face == Facing::UP)  // Face 1
	{
		return tex;
	}
	else if (face == Facing::DOWN)  // Face 0
	{
		return tex;
	}
	else
	{
		int_t var3 = tex + 1 + 16;  // Beta: int var3 = this.tex + 1 + 16
		if (lit)  // Beta: if (this.lit) var3++
		{
			var3++;
		}
		
		// Beta: Check orientation based on data (PumpkinTile.java:30-38)
		// data 0 = facing NORTH (face 2), data 1 = facing EAST (face 5), data 2 = facing SOUTH (face 3), data 3 = facing WEST (face 4)
		if (data == 0 && face == Facing::NORTH)  // var2 == 0 && var1 == 2
			return var3;
		else if (data == 1 && face == Facing::EAST)  // var2 == 1 && var1 == 5
			return var3;
		else if (data == 2 && face == Facing::SOUTH)  // var2 == 2 && var1 == 3
			return var3;
		else if (data == 3 && face == Facing::WEST)  // var2 == 3 && var1 == 4
			return var3;
		else
			return tex + 16;  // Beta: return this.tex + 16
	}
}

int_t PumpkinTile::getTexture(Facing face)
{
	// Beta: PumpkinTile.getTexture(int var1) (PumpkinTile.java:43-51)
	if (face == Facing::UP)  // Face 1
	{
		return tex;
	}
	else if (face == Facing::DOWN)  // Face 0
	{
		return tex;
	}
	else
	{
		// Beta: var1 == 3 ? this.tex + 1 + 16 : this.tex + 16
		// Face 3 = SOUTH, shows face texture
		return face == Facing::SOUTH ? tex + 1 + 16 : tex + 16;
	}
}

void PumpkinTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: PumpkinTile.onPlace() (PumpkinTile.java:54-56)
	Tile::onPlace(level, x, y, z);
}

bool PumpkinTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: PumpkinTile.mayPlace() (PumpkinTile.java:59-62)
	int_t var5 = level.getTile(x, y, z);
	return (var5 == 0 || Tile::tiles[var5]->material.isLiquid()) && level.isSolidTile(x, y - 1, z);
}

void PumpkinTile::setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob)
{
	// Beta: PumpkinTile.setPlacedBy() (PumpkinTile.java:65-68)
	// Calculate orientation from player rotation: Mth.floor(var5.yRot * 4.0F / 360.0F + 0.5) & 3
	int_t var6 = Mth::floor((double)(mob.yRot * 4.0F / 360.0F) + 0.5) & 3;
	level.setData(x, y, z, var6);
}
