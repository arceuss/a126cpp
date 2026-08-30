#include "tools/SignBench.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "client/Minecraft.h"
#include "client/Timer.h"
#include "client/renderer/Chunk.h"
#include "java/File.h"
#include "java/String.h"
#include "java/System.h"
#include "lwjgl/Display.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SignTile.h"
#include "OpenGL.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/phys/AABB.h"
#include "world/phys/Vec3.h"

// Developer fixture: renders a dense wall of signs in a hidden window through the
// production client path and writes frame timings to a log.
//
// It exists because sign-heavy views are the case where this port has been much
// slower than the reference client, and because the wall shows every sign
// variation (16 post rotations, four wall facings, colour codes, empty and
// full-length lines) in one view for visual comparison.

namespace signbench
{

bool g_blankText = false;

void fillWallText(SignTileEntity &sign, int_t index)
{
	if (g_blankText)
		return;

	const jstring colors = u"0123456789abcdef";
	char16_t color = static_cast<char16_t>(colors[static_cast<size_t>(index) % colors.size()]);

	sign.messages[0] = u"Sign " + String::toString(index);
	sign.messages[1] = jstring(u"\u00a7") + color + u"colored";
	sign.messages[2] = (index % 3 == 0) ? jstring() : jstring(u"WWWWWWWWWWWWWWW");
	sign.messages[3] = jstring(u"\u00a7") + color + u"tail";
}

// Signs only survive with a support block, so the fixture builds real geometry:
// stone walls carrying wall signs on both faces, and stone floors carrying rows
// of sign posts. `SignTile::onPlace` creates the tile entity, so the text is
// written into the entity the world made.
bool writeSign(Level &level, int_t x, int_t y, int_t z, int_t tileId, int_t data, int_t index)
{
	if (!level.setTileAndData(x, y, z, tileId, data))
		return false;
	if (level.getTile(x, y, z) != tileId)
		return false;

	std::shared_ptr<SignTileEntity> sign =
		std::dynamic_pointer_cast<SignTileEntity>(level.getTileEntity(x, y, z));
	if (sign == nullptr)
		return false;

	fillWallText(*sign, index);
	return true;
}

// `targetSigns` of zero is the control case: identical stone geometry, no signs,
// so the measurement isolates what the signs themselves cost.
int_t buildSignWall(Level &level, int_t targetSigns, int_t baseX, int_t baseY, int_t baseZ)
{
	int_t placed = 0;
	const int_t wallWidth = 32;
	const int_t wallHeight = 24;

	// Stacked walls, each carrying signs on its two z faces.
	for (int_t slab = 0; slab < 8; slab++)
	{
		int_t wallZ = baseZ + slab * 4;

		for (int_t x = baseX; x < baseX + wallWidth; x++)
			for (int_t y = baseY; y < baseY + wallHeight; y++)
				level.setTile(x, y, wallZ, Tile::rock.id);

		for (int_t face = 0; face < 2 && placed < targetSigns; face++)
		{
			// data 2 hangs on the block at z + 1, data 3 on the block at z - 1
			// (SignTile.java:103-109).
			int_t z = (face == 0) ? wallZ - 1 : wallZ + 1;
			int_t data = (face == 0) ? 2 : 3;
			for (int_t x = baseX; x < baseX + wallWidth && placed < targetSigns; x++)
			{
				for (int_t y = baseY; y < baseY + wallHeight && placed < targetSigns; y++)
				{
					if (writeSign(level, x, y, z, Tile::wallSign.id, data, placed))
						placed++;
				}
			}
		}
	}

	// Sign posts standing on their own stone floor, cycling all sixteen
	// rotations so every post angle is on screen.
	for (int_t row = 0; row < 8; row++)
	{
		int_t z = baseZ - 4 - row * 2;
		for (int_t x = baseX; x < baseX + wallWidth; x++)
			level.setTile(x, baseY - 1, z, Tile::rock.id);

		for (int_t x = baseX; x < baseX + wallWidth && placed < targetSigns; x++)
		{
			if (writeSign(level, x, baseY, z, Tile::sign.id, placed % 16, placed))
				placed++;
		}
	}

	return placed;
}

double percentile(std::vector<double> sorted, double fraction)
{
	if (sorted.empty())
		return 0.0;
	size_t index = static_cast<size_t>(fraction * (sorted.size() - 1));
	return sorted[index];
}

}

int runSignBench(int frames, int signCount, bool blankText, bool finishEachFrame,
	const std::string &worldName)
try
{
	if (frames <= 0)
		frames = 600;
	if (signCount < 0)
		signCount = 4096;
	signbench::g_blankText = blankText;

	Minecraft minecraft(854, 480, false);
	minecraft.unattended = true;
	std::cerr << "sign-bench: client initialised" << std::endl;
	minecraft.init();
	if (lwjgl::Display::isVisible())
	{
		std::cerr << "sign-bench: unattended window became visible" << std::endl;
		return 1;
	}

	std::shared_ptr<Level> level;
	const int_t baseX = 0;
	const int_t baseY = 70;
	const int_t baseZ = 0;
	int_t placed = 0;
	if (worldName.empty())
	{
		// Alpha's in-memory level constructor: no save directory, generated chunks.
		level = std::make_shared<Level>(u"sign-bench", Dimension::Id_Normal, 1234567LL);
		std::cerr << "sign-bench: level constructed" << std::endl;
		placed = signbench::buildSignWall(*level, static_cast<int_t>(signCount), baseX, baseY, baseZ);
		std::cerr << "sign-bench: signs placed " << placed << std::endl;
		minecraft.setLevel(level, u"Sign bench");
	}
	else
	{
		std::cerr << "sign-bench: loading world " << worldName << std::endl;
		minecraft.selectLevel(String::fromUTF8(worldName));
		level = minecraft.level;
	}
	std::cerr << "sign-bench: level bound" << std::endl;
	if (minecraft.player == nullptr || level == nullptr)
	{
		std::cerr << "sign-bench: the world produced no player" << std::endl;
		return 1;
	}

	if (worldName.empty())
	{
		// Stand in front of the wall looking at it.
		minecraft.player->moveTo(baseX + 16.0, baseY + 12.0, baseZ - 40.0, 0.0f, 0.0f);
	}
	const double fixedPlayerX = minecraft.player->x;
	const double fixedPlayerY = minecraft.player->y;
	const double fixedPlayerZ = minecraft.player->z;
	const float fixedPlayerYRot = minecraft.player->yRot;
	const float fixedPlayerXRot = minecraft.player->xRot;
	minecraft.setScreen(nullptr);
	minecraft.options.showDebugInfo = true;

	std::vector<double> frameTimes;
	std::vector<double> tickTimes;
	std::vector<double> lightTimes;
	std::vector<double> renderTimes;
	std::vector<double> finishTimes;
	frameTimes.reserve(static_cast<size_t>(frames));

	// Same loop shape as the singleplayer client: the timer decides how many
	// twenty-per-second ticks each frame owes, and the render uses the leftover
	// partial tick (Minecraft.cpp:467-520).
	Timer timer(20.0f);
	const int warmupFrames = worldName.empty() ? 60 : 2000;
	int_t warmupChunkUpdates = 0;
	int_t measuredChunkUpdates = 0;
	long_t gameTicks = 0;
	long_t loopStartMs = System::currentTimeMillis();
	for (int frame = 0; frame < frames; frame++)
	{
		AABB::resetPool();
		Vec3::resetPool();

		if (lwjgl::Display::isCloseRequested())
			break;

		auto start = std::chrono::steady_clock::now();

		timer.advanceTime();
		for (int_t i = 0; i < timer.ticks; i++)
		{
			minecraft.tick();
			if (!worldName.empty())
			{
				minecraft.player->moveTo(fixedPlayerX, fixedPlayerY, fixedPlayerZ,
					fixedPlayerYRot, fixedPlayerXRot);
			}
			gameTicks++;
		}
		auto afterTick = std::chrono::steady_clock::now();

		level->updateLights();
		auto afterLight = std::chrono::steady_clock::now();
		lwjgl::Display::update();
		minecraft.gameRenderer.render(timer.a);
		auto afterRender = std::chrono::steady_clock::now();

		if (finishEachFrame)
		{
			// Attribution: everything still queued in the driver is charged to the
			// frame that produced it instead of leaking into the next tick.
			glFinish();
		}
		auto afterFinish = std::chrono::steady_clock::now();

		// Skip the warm-up frames that compile chunk display lists.
		if (frame >= warmupFrames)
		{
			frameTimes.push_back(std::chrono::duration<double, std::milli>(afterFinish - start).count());
			tickTimes.push_back(std::chrono::duration<double, std::milli>(afterTick - start).count());
			lightTimes.push_back(std::chrono::duration<double, std::milli>(afterLight - afterTick).count());
			renderTimes.push_back(std::chrono::duration<double, std::milli>(afterRender - afterLight).count());
			finishTimes.push_back(std::chrono::duration<double, std::milli>(afterFinish - afterRender).count());
		}
		if (frame >= warmupFrames)
			measuredChunkUpdates += Chunk::updates;
		else
			warmupChunkUpdates += Chunk::updates;
		Chunk::updates = 0;
	}

	double loopSeconds = static_cast<double>(System::currentTimeMillis() - loopStartMs) / 1000.0;

	auto meanOf = [](const std::vector<double> &samples)
	{
		if (samples.empty())
			return 0.0;
		double total = 0.0;
		for (double sample : samples)
			total += sample;
		return total / samples.size();
	};

	std::vector<double> sorted = frameTimes;
	std::sort(sorted.begin(), sorted.end());
	double mean = meanOf(frameTimes);

	std::unique_ptr<File> logFile(File::open(u"sign-bench.log"));
	std::unique_ptr<std::ostream> out(logFile->toStreamOut());

	auto emit = [&](const std::string &line)
	{
		std::cout << line << '\n';
		if (out != nullptr)
			*out << line << '\n';
	};

	emit("sign-bench");
	emit("window_visible " + std::to_string(lwjgl::Display::isVisible() ? 1 : 0));
	emit("finish_each_frame " + std::to_string(finishEachFrame ? 1 : 0));
	emit("world " + (worldName.empty() ? std::string("generated") : worldName));
	emit("signs " + std::to_string(placed));
	emit("renderable_tile_entities "
		+ std::to_string(minecraft.levelRenderer.renderableTileEntities.size()));
	emit("warmup_frames " + std::to_string(frames < warmupFrames ? frames : warmupFrames));
	emit("measured_frames " + std::to_string(frameTimes.size()));
	emit("warmup_chunk_updates " + std::to_string(warmupChunkUpdates));
	emit("chunk_updates " + std::to_string(measuredChunkUpdates));
	emit("mean_ms " + std::to_string(mean));
	emit("min_ms " + std::to_string(sorted.empty() ? 0.0 : sorted.front()));
	emit("p50_ms " + std::to_string(signbench::percentile(sorted, 0.5)));
	emit("p95_ms " + std::to_string(signbench::percentile(sorted, 0.95)));
	emit("game_ticks " + std::to_string(gameTicks));
	emit("ticks_per_second " + std::to_string(loopSeconds > 0.0 ? gameTicks / loopSeconds : 0.0));
	emit("max_ms " + std::to_string(sorted.empty() ? 0.0 : sorted.back()));
	emit("mean_light_ms " + std::to_string(meanOf(lightTimes)));
	emit("mean_fps " + std::to_string(mean > 0.0 ? 1000.0 / mean : 0.0));
	emit("mean_tick_ms " + std::to_string(meanOf(tickTimes)));
	emit("mean_finish_ms " + std::to_string(meanOf(finishTimes)));
	emit("mean_render_ms " + std::to_string(meanOf(renderTimes)));

	minecraft.stop();
	return 0;
}
catch (const std::exception &e)
{
	std::cerr << "sign-bench failed: " << e.what() << std::endl;
	return 1;
}

// Runs the timer exactly as `Minecraft::run` does (advance once per frame, then
// run the reported ticks) and reports the resulting tick rate. Alpha's client
// must produce twenty ticks per wall-clock second.
int runTimerProbe(int seconds)
try
{
	if (seconds <= 0)
		seconds = 5;

	Timer timer(20.0f);
	long_t start = System::currentTimeMillis();
	long_t deadline = start + seconds * 1000LL;

	long_t frames = 0;
	long_t ticks = 0;
	double partialSum = 0.0;
	while (System::currentTimeMillis() < deadline)
	{
		timer.advanceTime();
		ticks += timer.ticks;
		partialSum += timer.a;
		frames++;
		// Roughly a 60 Hz client without a renderer attached.
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	double elapsedSeconds = static_cast<double>(System::currentTimeMillis() - start) / 1000.0;

	std::unique_ptr<File> logFile(File::open(u"timer-probe.log"));
	std::unique_ptr<std::ostream> out(logFile->toStreamOut());
	auto emit = [&](const std::string &line)
	{
		std::cout << line << '\n';
		if (out != nullptr)
			*out << line << '\n';
	};

	emit("timer-probe");
	emit("elapsed_s " + std::to_string(elapsedSeconds));
	emit("frames " + std::to_string(frames));
	emit("ticks " + std::to_string(ticks));
	emit("ticks_per_second " + std::to_string(elapsedSeconds > 0.0 ? ticks / elapsedSeconds : 0.0));
	emit("adjust_time " + std::to_string(timer.getAdjustTime()));
	return 0;
}
catch (const std::exception &e)
{
	std::cerr << "timer-probe failed: " << e.what() << std::endl;
	return 1;
}
