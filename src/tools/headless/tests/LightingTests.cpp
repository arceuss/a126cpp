// Layer 4: chunk-edge lighting eligibility and drain.
//
// Alpha reference:
//   World.func_627_a schedules an update when blockExists(xm, 64, zm) holds
//   for the range midpoint column only; neighboring chunks may be absent and
//   the update still applies to whichever columns are loaded
//   (MetadataChunkBlock.func_4127_a checks blockExists(x, 0, z) per column).
//   World.func_6465_g processes 5000 records per call; the frame tick drains
//   the whole queue in singleplayer.
//
// D15: the port required hasChunksAt(x, 0, z, 1), a 3x3 chunk neighborhood,
// so a valid edge column was skipped whenever a neighboring chunk was absent.
// The tests here drain and read light back, so the old predicate fails them:
// with only chunk (0,0) loaded, the x=15 column's +X neighbor chunk is absent.

#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/level/Level.h"
#include "world/level/LightLayer.h"
#include "world/level/dimension/Dimension.h"

static void drain(Level &level)
{
	int calls = 0;
	while (level.updateLights())
	{
		if (++calls > 1000)
			break;
	}
}

// Highest y in the column that is open to the sky, or -1 if none.
static int_t skyOpenY(Level &level, int_t x, int_t z)
{
	for (int_t y = Level::DEPTH - 1; y >= 0; --y)
	{
		if (level.getTile(x, y, z) == 0 && level.isSkyLit(x, y, z))
			return y;
	}
	return -1;
}

HEADLESS_TEST(lighting, edge_column_relights_when_neighbor_chunk_absent)
{
	headless::initGameRegistries();
	Level level(u"lighting-edge", Dimension::Id_Normal, 201LL);
	level.getChunk(0, 0); // Chunk (1,0) deliberately absent.
	drain(level);

	const int_t y = skyOpenY(level, 15, 8);
	ctx.check(y >= 0, "edge column has a sky-open cell");
	ctx.checkEqual(level.getBrightness(LightLayer::Sky, 15, y, 8), 15, "sky-open cell starts at full sky light");

	// Corrupt the stored value, then ask the lighting engine to repair just
	// this column. Alpha applies it because the column's own chunk exists.
	level.setBrightness(LightLayer::Sky, 15, y, 8, 0);
	level.updateLight(LightLayer::Sky, 15, y, 8, 15, y, 8);
	ctx.check(!level.lightUpdates.empty(), "edge column queues with the neighbor chunk absent");
	drain(level);
	ctx.checkEqual(level.getBrightness(LightLayer::Sky, 15, y, 8), 15,
		"edge column is relit although the +X neighbor chunk is absent");
	ctx.checkEqual(level.lightUpdates.size(), static_cast<std::size_t>(0), "queue drains fully");
}

HEADLESS_TEST(lighting, unloaded_center_drops_update)
{
	headless::initGameRegistries();
	Level level(u"lighting-unloaded", Dimension::Id_Normal, 202LL);

	const std::size_t before = level.lightUpdates.size();
	level.updateLight(LightLayer::Sky, 15, 0, 8, 15, 127, 8);
	ctx.checkEqual(level.lightUpdates.size(), before,
		"update centered in an unloaded chunk is dropped");
}

HEADLESS_TEST(lighting, range_straddling_unloaded_chunk_applies_loaded_part)
{
	headless::initGameRegistries();
	Level level(u"lighting-straddle", Dimension::Id_Normal, 203LL);
	level.getChunk(0, 0); // Chunk (1,0) absent; range x 8..23 crosses into it.
	drain(level);

	const int_t y = skyOpenY(level, 12, 8);
	ctx.check(y >= 0, "interior column has a sky-open cell");
	level.setBrightness(LightLayer::Sky, 12, y, 8, 0);
	// Midpoint column x=15 is in the loaded chunk, so Alpha queues the range.
	level.updateLight(LightLayer::Sky, 8, y, 8, 23, y, 8);
	ctx.check(!level.lightUpdates.empty(), "range with loaded midpoint queues");
	drain(level);
	ctx.checkEqual(level.getBrightness(LightLayer::Sky, 12, y, 8), 15,
		"loaded columns of a straddling range are applied");
	ctx.checkEqual(level.lightUpdates.size(), static_cast<std::size_t>(0),
		"straddling range drains without rescheduling forever");
}
