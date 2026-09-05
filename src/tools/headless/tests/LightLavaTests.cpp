#include "tools/headless/TestFramework.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidFlowingTile.h"

HEADLESS_TEST(lighting, lava_initialization_stays_in_its_column)
{
	Level level(u"lava-column-boundary", Dimension::Id_Normal, 201LL);
	LevelChunk chunk(level, 0, 0);
	for (int y = 31; y < Level::DEPTH; ++y)
		chunk.blocks[y] = Tile::lava.id;
	chunk.lightLava();
	ctx.checkEqual(chunk.blockLight.get(0, 127, 0), 15, "top lava retains emission");
	ctx.checkEqual(chunk.blockLight.get(0, 0, 1), 0, "upward light never spills into next column");
	ctx.checkEqual(chunk.blockLight.get(0, 31, 1), 14, "air column retains Alpha initialization");
	ctx.checkEqual(chunk.blockLight.get(0, 44, 1), 1, "upward attenuation retains final nonzero cell");
	ctx.checkEqual(chunk.blockLight.get(0, 45, 1), 0, "upward attenuation terminates at zero");
}

HEADLESS_TEST(lighting, lava_initialization_bounds_final_column)
{
	Level level(u"lava-final-column", Dimension::Id_Normal, 202LL);
	LevelChunk chunk(level, 0, 0);
	chunk.blocks.fill(Tile::lava.id);
	chunk.lightLava();
	for (int y = 0; y < Level::DEPTH; ++y)
		if (!ctx.checkEqual(chunk.blockLight.get(15, y, 15), 15, "final column emission"))
			return;
}
