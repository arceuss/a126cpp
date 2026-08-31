#include "tools/SignBench.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#include "client/Minecraft.h"
#include "client/Timer.h"
#include "client/renderer/Chunk.h"
#include "backends/Backend.h"
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

struct ProcessMemory
{
	std::uint64_t privateBytes = 0;
	std::uint64_t workingSetBytes = 0;
};

ProcessMemory processMemory()
{
	ProcessMemory result;
#if defined(_WIN32)
	PROCESS_MEMORY_COUNTERS_EX counters = {};
	counters.cb = sizeof(counters);
	if (GetProcessMemoryInfo(GetCurrentProcess(),
		reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters), sizeof(counters)))
	{
		result.privateBytes = static_cast<std::uint64_t>(counters.PrivateUsage);
		result.workingSetBytes = static_cast<std::uint64_t>(counters.WorkingSetSize);
	}
#endif
	return result;
}

struct TravelPosition
{
	int_t x = 0;
	int_t z = 0;
};

void appendTravelSegment(std::vector<TravelPosition> &path, int_t &x, int_t &z,
	int_t targetX, int_t targetZ)
{
	while (x != targetX)
	{
		x += targetX > x ? 1 : -1;
		path.push_back({ x, z });
	}
	while (z != targetZ)
	{
		z += targetZ > z ? 1 : -1;
		path.push_back({ x, z });
	}
}

std::vector<TravelPosition> buildTravelPath(int_t radius)
{
	std::vector<TravelPosition> path;
	int_t x = 0;
	int_t z = 0;
	appendTravelSegment(path, x, z, radius, 0);
	appendTravelSegment(path, x, z, radius, radius);
	appendTravelSegment(path, x, z, -radius, radius);
	appendTravelSegment(path, x, z, -radius, -radius);
	appendTravelSegment(path, x, z, radius, -radius);
	appendTravelSegment(path, x, z, 0, 0);
	return path;
}

void placeTravelPlayer(Player &player, const TravelPosition &position, double y)
{
	const double x = static_cast<double>(position.x * 16 + 8);
	const double z = static_cast<double>(position.z * 16 + 8);
	player.moveTo(x, y, z, 0.0f, 0.0f);
	player.xo = player.xOld = x;
	player.yo = player.yOld = y;
	player.zo = player.zOld = z;
	player.yRotO = player.yRot;
	player.xRotO = player.xRot;
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

	const int measuredFrameTarget = frames;
	const int frameWidth = 854;
	const int frameHeight = 480;
	Minecraft minecraft(frameWidth, frameHeight, false);
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
	frameTimes.reserve(static_cast<size_t>(measuredFrameTarget));

	// A deterministic tick cadence makes every backend render the same world
	// state sequence. Real-time Timer scheduling made a slower backend perform
	// more ticks during the same frame count, invalidating workload comparisons.
	const int benchmarkTickInterval = 10;
	const int warmupFrames = worldName.empty() ? 60 : 2000;
	const int totalFrames = warmupFrames + measuredFrameTarget;
	int_t warmupChunkUpdates = 0;
	int_t measuredChunkUpdates = 0;
	long_t gameTicks = 0;
	long_t loopStartMs = System::currentTimeMillis();
	for (int frame = 0; frame < totalFrames; frame++)
	{
		AABB::resetPool();
		Vec3::resetPool();

		if (lwjgl::Display::isCloseRequested())
			break;

		auto start = std::chrono::steady_clock::now();
		const int scheduledTicks = frame % benchmarkTickInterval == 0 ? 1 : 0;
		for (int_t i = 0; i < scheduledTicks; i++)
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
		const float partialTick = static_cast<float>(frame % benchmarkTickInterval) /
			static_cast<float>(benchmarkTickInterval);
		minecraft.gameRenderer.render(partialTick);
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
	emit("backend " + std::string(renderbackend::configuration().recordName));
	emit("width " + std::to_string(frameWidth));
	emit("height " + std::to_string(frameHeight));
	emit("blank_text " + std::to_string(blankText ? 1 : 0));
	const char *renderDiagnostics = std::getenv("A126_RENDER_DIAGNOSTICS");
	const char *legacyValidation = std::getenv("A126_LEGACYGL_VALIDATE");
	emit("render_diagnostics " + std::to_string(renderDiagnostics != nullptr &&
		std::string(renderDiagnostics) == "1" ? 1 : 0));
	emit("legacy_validation " + std::to_string(legacyValidation != nullptr &&
		std::string(legacyValidation) == "1" ? 1 : 0));
	emit("window_visible " + std::to_string(lwjgl::Display::isVisible() ? 1 : 0));
	emit("finish_each_frame " + std::to_string(finishEachFrame ? 1 : 0));
	emit("world " + (worldName.empty() ? std::string("generated") : worldName));
	emit("signs " + std::to_string(placed));
	emit("renderable_tile_entities "
		+ std::to_string(minecraft.levelRenderer.renderableTileEntities.size()));
	emit("warmup_frames " + std::to_string(warmupFrames));
	emit("measured_frames " + std::to_string(frameTimes.size()));
	emit("requested_measured_frames " + std::to_string(measuredFrameTarget));
	emit("tick_interval_frames " + std::to_string(benchmarkTickInterval));
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


int runChunkTravelBench(int cycles, int radiusChunks, int framesPerChunk,
	int settleFrames, int viewDistance, const std::string &worldName)
try
{
	if (cycles <= 0)
		cycles = 3;
	if (radiusChunks <= 0)
		radiusChunks = 24;
	if (framesPerChunk <= 0)
		framesPerChunk = 2;
	if (settleFrames <= 0)
		settleFrames = 120;
	// Alpha's options value: 0 is FAR (400 blocks), 3 is TINY. Walking with a
	// large view distance is the case the renderer has to survive, so that is
	// the default here.
	if (viewDistance < 0 || viewDistance > 3)
		viewDistance = 0;

	const int frameWidth = 854;
	const int frameHeight = 480;
	Minecraft minecraft(frameWidth, frameHeight, false);
	minecraft.unattended = true;
	minecraft.init();
	if (lwjgl::Display::isVisible())
	{
		std::cerr << "chunk-travel-bench: unattended window became visible" << std::endl;
		return 1;
	}

	minecraft.options.viewDistance = static_cast<int_t>(viewDistance);
	std::shared_ptr<Level> level;
	if (worldName.empty())
	{
		level = std::make_shared<Level>(u"chunk-travel-bench",
			Dimension::Id_Normal, 1234567LL);
		minecraft.setLevel(level, u"Chunk travel bench");
	}
	else
	{
		minecraft.selectLevel(String::fromUTF8(worldName));
		level = minecraft.level;
	}
	if (minecraft.player == nullptr || level == nullptr)
	{
		std::cerr << "chunk-travel-bench: the world produced no player" << std::endl;
		return 1;
	}

	minecraft.setScreen(nullptr);
	minecraft.options.showDebugInfo = true;
	const double playerY = minecraft.player->y;
	const std::vector<signbench::TravelPosition> path =
		signbench::buildTravelPath(static_cast<int_t>(radiusChunks));
	const signbench::TravelPosition origin = {};
	int frameNumber = 0;

	auto renderFrames = [&](const signbench::TravelPosition &position, int count,
		std::vector<double> *timings, int_t &chunkUpdates)
	{
		for (int frame = 0; frame < count; frame++, frameNumber++)
		{
			AABB::resetPool();
			Vec3::resetPool();
			if (lwjgl::Display::isCloseRequested())
				throw std::runtime_error("chunk travel window was closed");
			if (frameNumber % 10 == 0)
				minecraft.tick();
			signbench::placeTravelPlayer(*minecraft.player, position, playerY);

			const std::chrono::steady_clock::time_point start =
				std::chrono::steady_clock::now();
			level->updateLights();
			lwjgl::Display::update();
			minecraft.gameRenderer.render(0.0f);
			const std::chrono::steady_clock::time_point end =
				std::chrono::steady_clock::now();
			if (timings != nullptr)
			{
				timings->push_back(std::chrono::duration<double, std::milli>(
					end - start).count());
			}
			chunkUpdates += Chunk::updates;
			Chunk::updates = 0;
		}
	};

	int_t baselineChunkUpdates = 0;
	renderFrames(origin, settleFrames, nullptr, baselineChunkUpdates);
	glFinish();
	const signbench::ProcessMemory baseline = signbench::processMemory();

	std::unique_ptr<File> logFile(File::open(u"chunk-travel-bench.log"));
	std::unique_ptr<std::ostream> out(logFile->toStreamOut());
	auto emit = [&](const std::string &line)
	{
		std::cout << line << '\n';
		if (out != nullptr)
			*out << line << '\n';
	};

	emit("chunk-travel-bench");
	emit("backend " + std::string(renderbackend::configuration().recordName));
	emit("world " + (worldName.empty() ? std::string("generated") : worldName));
	emit("view_distance " + std::to_string(viewDistance));
	emit("cycles " + std::to_string(cycles));
	emit("radius_chunks " + std::to_string(radiusChunks));
	emit("route_positions " + std::to_string(path.size()));
	emit("frames_per_chunk " + std::to_string(framesPerChunk));
	emit("settle_frames " + std::to_string(settleFrames));
	emit("baseline_chunk_updates " + std::to_string(baselineChunkUpdates));
	emit("baseline_private_bytes " + std::to_string(baseline.privateBytes));
	emit("baseline_working_set_bytes " + std::to_string(baseline.workingSetBytes));

	for (int cycle = 1; cycle <= cycles; cycle++)
	{
		std::vector<double> timings;
		timings.reserve(path.size() * static_cast<std::size_t>(framesPerChunk) +
			static_cast<std::size_t>(settleFrames));
		int_t chunkUpdates = 0;
		for (const signbench::TravelPosition &position : path)
			renderFrames(position, framesPerChunk, &timings, chunkUpdates);
		renderFrames(origin, settleFrames, &timings, chunkUpdates);
		glFinish();

		const signbench::ProcessMemory memory = signbench::processMemory();
		std::vector<double> sorted = timings;
		std::sort(sorted.begin(), sorted.end());
		double total = 0.0;
		for (double timing : timings)
			total += timing;
		const double mean = timings.empty() ? 0.0 :
			total / static_cast<double>(timings.size());
		const std::int64_t privateGrowth = static_cast<std::int64_t>(memory.privateBytes) -
			static_cast<std::int64_t>(baseline.privateBytes);
		const std::string prefix = "cycle_" + std::to_string(cycle) + "_";
		emit(prefix + "frames " + std::to_string(timings.size()));
		emit(prefix + "chunk_updates " + std::to_string(chunkUpdates));
		emit(prefix + "mean_ms " + std::to_string(mean));
		emit(prefix + "p95_ms " +
			std::to_string(signbench::percentile(sorted, 0.95)));
		emit(prefix + "private_bytes " + std::to_string(memory.privateBytes));
		emit(prefix + "private_growth_bytes " + std::to_string(privateGrowth));
		emit(prefix + "working_set_bytes " + std::to_string(memory.workingSetBytes));
	}

	minecraft.stop();
	return 0;
}
catch (const std::exception &e)
{
	std::cerr << "chunk-travel-bench failed: " << e.what() << std::endl;
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
