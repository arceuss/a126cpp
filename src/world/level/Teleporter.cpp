#include "world/level/Teleporter.h"

#include <iostream>

#include "util/Mth.h"
#include "world/entity/Entity.h"
#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/ObsidianTile.h"
#include "world/level/tile/PortalTile.h"

// Alpha: Teleporter.func_4107_a (Teleporter.java:15-21)
void Teleporter::teleport(Level &level, Entity &entity)
{
	if (findPortal(level, entity))
		return;
	createPortal(level, entity);
	findPortal(level, entity);
}

// Alpha: Teleporter.func_4106_b (Teleporter.java:23-79)
bool Teleporter::findPortal(Level &level, Entity &entity)
{
	short_t range = 128;
	double bestDistance = -1.0;
	int_t bestX = 0;
	int_t bestY = 0;
	int_t bestZ = 0;
	int_t originX = Mth::floor(entity.x);
	int_t originZ = Mth::floor(entity.z);

	for (int_t x = originX - range; x <= originX + range; x++)
	{
		double dx = x + 0.5 - entity.x;
		for (int_t z = originZ - range; z <= originZ + range; z++)
		{
			double dz = z + 0.5 - entity.z;
			for (int_t y = 127; y >= 0; y--)
			{
				if (level.getTile(x, y, z) != Tile::portalTile.id)
					continue;
				while (level.getTile(x, y - 1, z) == Tile::portalTile.id)
					y--;

				double dy = y + 0.5 - entity.y;
				double distance = dx * dx + dy * dy + dz * dz;
				if (bestDistance >= 0.0 && distance >= bestDistance)
					continue;
				bestDistance = distance;
				bestX = x;
				bestY = y;
				bestZ = z;
			}
		}
	}

	if (bestDistance < 0.0)
		return false;

	double x = bestX + 0.5;
	double y = bestY + 0.5;
	double z = bestZ + 0.5;

	if (level.getTile(bestX - 1, bestY, bestZ) == Tile::portalTile.id)
		x -= 0.5;
	if (level.getTile(bestX + 1, bestY, bestZ) == Tile::portalTile.id)
		x += 0.5;
	if (level.getTile(bestX, bestY, bestZ - 1) == Tile::portalTile.id)
		z -= 0.5;
	if (level.getTile(bestX, bestY, bestZ + 1) == Tile::portalTile.id)
		z += 0.5;

	std::cout << "Teleporting to " << x << ", " << y << ", " << z << std::endl;
	// Alpha: setLocationAndAngles(), so the entity's feet land on the bottom
	// portal block's mid-height (Teleporter.java:72, Entity.java:538-545).
	entity.absMoveTo(x, y, z, entity.yRot, 0.0f);
	entity.zd = 0.0;
	entity.yd = 0.0;
	entity.xd = 0.0;
	return true;
}

// Alpha: the frame fits when the five-high column at every offset is air and
// the row below it is solid (Teleporter.java:121-130,156-163). A rejected site
// aborts the remaining rotations for that column, exactly like Alpha's
// `continue block2` / `continue block10`.
static bool frameFits(Level &level, int_t x, int_t y, int_t z, int_t xStep, int_t zStep, int_t depth)
{
	for (int_t across = 0; across < 4; across++)
	{
		for (int_t up = -1; up < 4; up++)
		{
			int_t tx = x + (across - 1) * xStep + depth * zStep;
			int_t ty = y + up;
			int_t tz = z + (across - 1) * zStep - depth * xStep;
			if (up < 0 && !level.getMaterial(tx, ty, tz).isSolid())
				return false;
			if (up >= 0 && level.getTile(tx, ty, tz) != 0)
				return false;
		}
	}
	return true;
}

// Alpha: Teleporter.func_4108_c (Teleporter.java:81-229)
bool Teleporter::createPortal(Level &level, Entity &entity)
{
	int_t range = 16;
	double bestDistance = -1.0;
	int_t originX = Mth::floor(entity.x);
	int_t originY = Mth::floor(entity.y);
	int_t originZ = Mth::floor(entity.z);
	int_t placeX = originX;
	int_t placeY = originY;
	int_t placeZ = originZ;
	int_t rotation = 0;
	int_t firstRotation = random.nextInt(4);

	// Alpha's first pass needs three block depth of clearance and considers all
	// four rotations (Teleporter.java:105-142).
	for (int_t x = originX - range; x <= originX + range; x++)
	{
		double dx = x + 0.5 - entity.x;
		for (int_t z = originZ - range; z <= originZ + range; z++)
		{
			double dz = z + 0.5 - entity.z;
			for (int_t y = 127; y >= 0; y--)
			{
				if (level.getTile(x, y, z) != 0)
					continue;
				while (y > 0 && level.getTile(x, y - 1, z) == 0)
					y--;

				for (int_t candidate = firstRotation; candidate < firstRotation + 4; candidate++)
				{
					int_t xStep = candidate % 2;
					int_t zStep = 1 - xStep;
					if (candidate % 4 >= 2)
					{
						xStep = -xStep;
						zStep = -zStep;
					}

					bool fits = true;
					for (int_t depth = 0; depth < 3 && fits; depth++)
						fits = frameFits(level, x, y, z, xStep, zStep, depth);
					if (!fits)
						break;

					double dy = y + 0.5 - entity.y;
					double distance = dx * dx + dy * dy + dz * dz;
					if (bestDistance >= 0.0 && distance >= bestDistance)
						continue;
					bestDistance = distance;
					placeX = x;
					placeY = y;
					placeZ = z;
					rotation = candidate % 4;
				}
			}
		}
	}

	// Alpha's second pass drops the depth requirement and only tries two
	// rotations (Teleporter.java:143-176).
	if (bestDistance < 0.0)
	{
		for (int_t x = originX - range; x <= originX + range; x++)
		{
			double dx = x + 0.5 - entity.x;
			for (int_t z = originZ - range; z <= originZ + range; z++)
			{
				double dz = z + 0.5 - entity.z;
				for (int_t y = 127; y >= 0; y--)
				{
					if (level.getTile(x, y, z) != 0)
						continue;
					// Alpha omits the floor guard here (Teleporter.java:150),
					// which walks below the world forever when the column is
					// open to y=0. The guard only prevents that hang.
					while (y > 0 && level.getTile(x, y - 1, z) == 0)
						y--;

					for (int_t candidate = firstRotation; candidate < firstRotation + 2; candidate++)
					{
						int_t xStep = candidate % 2;
						int_t zStep = 1 - xStep;

						if (!frameFits(level, x, y, z, xStep, zStep, 0))
							break;

						double dy = y + 0.5 - entity.y;
						double distance = dx * dx + dy * dy + dz * dz;
						if (bestDistance >= 0.0 && distance >= bestDistance)
							continue;
						bestDistance = distance;
						placeX = x;
						placeY = y;
						placeZ = z;
						rotation = candidate % 2;
					}
				}
			}
		}
	}

	int_t frameX = placeX;
	int_t frameY = placeY;
	int_t frameZ = placeZ;
	int_t xStep = rotation % 2;
	int_t zStep = 1 - xStep;
	if (rotation % 4 >= 2)
	{
		xStep = -xStep;
		zStep = -zStep;
	}

	// No site fits: Alpha clamps the height and carves an obsidian platform
	// (Teleporter.java:187-206).
	if (bestDistance < 0.0)
	{
		if (placeY < 70)
			placeY = 70;
		if (placeY > 118)
			placeY = 118;
		frameY = placeY;

		for (int_t depth = -1; depth <= 1; depth++)
		{
			for (int_t across = 1; across < 3; across++)
			{
				for (int_t up = -1; up < 3; up++)
				{
					int_t tx = frameX + (across - 1) * xStep + depth * zStep;
					int_t ty = frameY + up;
					int_t tz = frameZ + (across - 1) * zStep - depth * xStep;
					level.setTile(tx, ty, tz, (up < 0) ? Tile::obsidian.id : 0);
				}
			}
		}
	}

	// Alpha builds the frame four times, suppressing neighbour updates while
	// placing and replaying them afterwards (Teleporter.java:207-227).
	for (int_t pass = 0; pass < 4; pass++)
	{
		level.noNeighborUpdate = true;

		for (int_t across = 0; across < 4; across++)
		{
			for (int_t up = -1; up < 4; up++)
			{
				int_t tx = frameX + (across - 1) * xStep;
				int_t ty = frameY + up;
				int_t tz = frameZ + (across - 1) * zStep;
				bool frame = (across == 0 || across == 3 || up == -1 || up == 3);
				level.setTile(tx, ty, tz, frame ? Tile::obsidian.id : Tile::portalTile.id);
			}
		}

		level.noNeighborUpdate = false;

		for (int_t across = 0; across < 4; across++)
		{
			for (int_t up = -1; up < 4; up++)
			{
				int_t tx = frameX + (across - 1) * xStep;
				int_t ty = frameY + up;
				int_t tz = frameZ + (across - 1) * zStep;
				level.updateNeighborsAt(tx, ty, tz, level.getTile(tx, ty, tz));
			}
		}
	}

	return true;
}
