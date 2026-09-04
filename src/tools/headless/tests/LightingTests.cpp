// Layer 4: chunk-edge lighting eligibility and drain.
//
// Alpha reference:
//   World.func_627_a schedules an update when blockExists(xm, 64, zm) holds
//   for the range midpoint column only; neighboring chunks may be absent and
//   the update still applies to whichever columns are loaded
//   (MetadataChunkBlock.func_4127_a checks blockExists per column).
//   World.func_6465_g processes 5000 records per call; the frame tick drains
//   the whole queue in singleplayer.
//
// D15: the port required hasChunksAt(x, 0, z, 1), a 3x3 chunk neighborhood,
// so a valid edge column was skipped whenever a neighboring chunk was absent.

#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/level/Level.h"
#include "world/level/LightLayer.h"
#include "world/level/dimension/Dimension.h"

HEADLESS_TEST(lighting, edge_column_queues_when_neighbors_absent)
{
	headless::initGameRegistries();
	Level level(u"lighting-edge", Dimension::Id_Normal, 201LL);
	level.getChunk(0, 0); // Neighbors deliberately absent.

	const std::size_t before = level.lightUpdates.size();
	// Single full-height column at the chunk's +X edge: midpoint column is
	// loaded, the x+1 neighbor column is not.
	level.updateLight(LightLayer::Sky, 15, 0, 8, 15, 127, 8);
	ctx.checkEqual(level.lightUpdates.size(), before + 1,
		"edge column queues although the neighboring chunk is absent");
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

HEADLESS_TEST(lighting, edge_update_drain_terminates)
{
	headless::initGameRegistries();
	Level level(u"lighting-drain", Dimension::Id_Normal, 203LL);
	level.getChunk(0, 0);
	level.getChunk(1, 0);

	// A range straddling the loaded boundary, drained the way the
	// singleplayer frame tick drains it.
	level.updateLight(LightLayer::Sky, 8, 0, 8, 23, 127, 8);
	int calls = 0;
	while (level.updateLights())
	{
		if (++calls > 1000)
			break;
	}
	ctx.check(calls <= 1000, "edge-spanning light queue drains without rescheduling forever");
	ctx.checkEqual(level.lightUpdates.size(), static_cast<std::size_t>(0),
		"edge-spanning light queue is empty after a full drain");
}
