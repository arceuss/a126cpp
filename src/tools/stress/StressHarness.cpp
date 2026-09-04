// OpenGL.h reaches windows.h; keep std::min/max usable below.
#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif

#include "tools/stress/StressHarness.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "tools/BenchUtil.h"
#include "tools/MemoryProbe.h"

#include "client/Minecraft.h"
#include "client/gamemode/SurvivalMode.h"
#include "client/gui/GuiConnectFailed.h"
#include "client/gui/GuiConnecting.h"
#include "client/multiplayer/MultiPlayerLevel.h"
#include "util/BackgroundTask.h"
#include "network/NetClientHandler.h"
#include "client/player/LocalPlayer.h"
#include "client/renderer/Chunk.h"
#include "backends/Backend.h"
#include "java/File.h"
#include "java/String.h"
#include "java/System.h"
#include "legacygl/Context.h"
#include "legacygl/PhaseProfile.h"
#include "lwjgl/Display.h"
#include "util/Memory.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/dimension/Dimension.h"
#include "world/phys/AABB.h"
#include "world/phys/Vec3.h"
#include "OpenGL.h"
#include "backends/Platform/Platform.h"
#include "stb_image_write.h"

namespace stress
{

bool Params::has(const std::string &key) const
{
	return values.find(key) != values.end();
}

int Params::intOr(const std::string &key, int fallback) const
{
	auto it = values.find(key);
	return it == values.end() ? fallback : std::atoi(it->second.c_str());
}

double Params::doubleOr(const std::string &key, double fallback) const
{
	auto it = values.find(key);
	return it == values.end() ? fallback : std::atof(it->second.c_str());
}

std::string Params::stringOr(const std::string &key, const std::string &fallback) const
{
	auto it = values.find(key);
	return it == values.end() ? fallback : it->second;
}

void pinPlayer(LocalPlayer &player, double x, double y, double z, float yRot, float xRot)
{
	player.moveTo(x, y, z, yRot, xRot);
	player.xo = player.xOld = x;
	player.yo = player.yOld = y;
	player.zo = player.zOld = z;
	player.yRotO = yRot;
	player.xRotO = xRot;
	player.xd = 0.0;
	player.yd = 0.0;
	player.zd = 0.0;
	player.health = 20;
}

using Clock = std::chrono::steady_clock;

static double millisBetween(Clock::time_point a, Clock::time_point b)
{
	return std::chrono::duration<double, std::milli>(b - a).count();
}

struct Window
{
	std::vector<double> frame, tick, light, swap, render, finish;

	void clear()
	{
		frame.clear(); tick.clear(); light.clear(); swap.clear(); render.clear(); finish.clear();
	}
};

static double meanOf(const std::vector<double> &samples)
{
	if (samples.empty())
		return 0.0;
	double total = 0.0;
	for (double sample : samples)
		total += sample;
	return total / static_cast<double>(samples.size());
}

static std::string fmt(double value)
{
	char buffer[32];
	std::snprintf(buffer, sizeof(buffer), "%.3f", value);
	return buffer;
}

static std::string mib(std::uint64_t bytes)
{
	return fmt(static_cast<double>(bytes) / (1024.0 * 1024.0));
}

int run(const Options &options)
try
{
#ifdef A126_ENABLE_MEMORY_PROBE
	legacygl::phaseProfileEnabled = true;
#endif

	// "all" runs every offline scenario in one process for unattended runs:
	// one fresh level per scenario, one log per scenario. The server-driven
	// "command" scenario is excluded; use it explicitly with --server.
	// Every scenario runs twice, lit and fullbright (--no-lighting), so the
	// lighting engine's share falls out of the log diff.
	struct Run
	{
		std::string scenario;
		bool noLighting = false;
	};
	std::vector<Run> runs;
	if (options.scenario != "all")
		runs.push_back({ options.scenario, options.noLighting });

	const int frameWidth = 854;
	const int frameHeight = 480;
	// Heap, as Minecraft::start does: the object is large and the console's
	// main-thread stack is small.
	std::unique_ptr<Minecraft> minecraftOwner = Util::make_unique<Minecraft>(frameWidth, frameHeight, false);
	Minecraft &minecraft = *minecraftOwner;
	minecraft.unattended = true;
	std::cerr << "stress: init" << std::endl;
	minecraft.init();
	std::cerr << "stress: init done" << std::endl;
#ifndef __SWITCH__
	if (lwjgl::Display::isVisible())
	{
		std::cerr << "stress: unattended window became visible" << std::endl;
		return 1;
	}
#endif
	// Minecraft::run adopts the window's size on its first frame; this loop
	// replaces run, so do the same. On the console the window is the panel.
	if (lwjgl::Display::getWidth() != minecraft.width || lwjgl::Display::getHeight() != minecraft.height)
		minecraft.resize(lwjgl::Display::getWidth(), lwjgl::Display::getHeight());
	minecraft.options.viewDistance = static_cast<int_t>(options.viewDistance);
	if (options.fancyGraphics >= 0)
		minecraft.options.fancyGraphics = options.fancyGraphics != 0;

	const bool online = !options.serverHost.empty();
	if (options.scenario == "all")
	{
		if (online)
		{
			std::cerr << "stress: 'all' runs offline scenarios only; use single scenarios with --server" << std::endl;
			return 2;
		}
		for (const char *name : { "idle", "spin", "daycycle", "cave", "walk", "travel", "farlands",
			"building", "lighting", "fluids", "mobs", "entities", "tnt", "sounds" })
		{
			runs.push_back({ name, false });
			runs.push_back({ name, true });
		}
	}
	for (const Run &run : runs)
	{
		if (makeScenario(run.scenario) == nullptr)
		{
			std::cerr << "stress: unknown scenario '" << run.scenario << "'; known:";
			for (const std::string &known : scenarioNames())
				std::cerr << ' ' << known;
			std::cerr << std::endl;
			return 2;
		}
	}
	if (!options.user.empty())
		minecraft.options.username = String::fromUTF8(options.user);

	for (std::size_t runIndex = 0; runIndex < runs.size(); runIndex++)
	{
		const bool noLighting = runs[runIndex].noLighting;
		std::unique_ptr<Scenario> scenario = makeScenario(runs[runIndex].scenario);
		std::cerr << "stress: scenario " << scenario->name() << (noLighting ? " (no lighting)" : "")
			<< " (" << (runIndex + 1) << '/' << runs.size() << ')' << std::endl;
		if (online && !scenario->supportsMultiplayer())
		{
			std::cerr << "stress: scenario '" << scenario->name()
				<< "' edits the world and cannot run against a server" << std::endl;
			return 2;
		}
		const long_t seed = static_cast<long_t>(options.params.intOr("seed", 1234567));
		const benchutil::ProcessMemory memoryAtStart = benchutil::processMemory();


	auto levelStart = Clock::now();
	std::shared_ptr<Level> level;
	if (online)
	{
		// The production connect path: the connecting screen spawns the socket
		// thread, the login handler binds the MultiPlayerLevel, and the first
		// position packet dismisses the terrain screen.
		minecraft.setScreen(Util::make_shared<GuiConnecting>(minecraft,
			String::fromUTF8(options.serverHost), static_cast<int_t>(options.serverPort)));
		const double timeoutMs = options.params.doubleOr("connect_timeout_ms", 60000.0);
		// A server measures movement against wall time, so online ticks run
		// at the real 20 Hz; unpaced frames would look like a speed hack.
		Clock::time_point nextTickAt = Clock::now();
		auto pacedTick = [&]()
		{
			std::this_thread::sleep_until(nextTickAt);
			nextTickAt += std::chrono::milliseconds(50);
			AABB::resetPool();
			Vec3::resetPool();
			minecraft.tick();
			lwjgl::Display::update();
			minecraft.gameRenderer.render(0.0f);
		};
		while (minecraft.level == nullptr || minecraft.player == nullptr || minecraft.screen != nullptr)
		{
			if (millisBetween(levelStart, Clock::now()) > timeoutMs)
			{
				std::cerr << "stress: no world from " << options.serverHost << ':'
					<< options.serverPort << " within " << timeoutMs << " ms" << std::endl;
				return 1;
			}
			if (dynamic_cast<GuiConnectFailed *>(minecraft.screen.get()) != nullptr)
			{
				std::cerr << "stress: connection to " << options.serverHost << ':'
					<< options.serverPort << " failed or was closed by the server" << std::endl;
				return 1;
			}
			pacedTick();
		}
		level = minecraft.level;
		// The terrain screen closes on the first position packet, before the
		// spawn chunks have all landed. Let them arrive and the player settle
		// on the ground before the scenario takes its start position.
		const int settleTicks = options.params.intOr("settle_ticks", 60);
		for (int i = 0; i < settleTicks && minecraft.level.get() == level.get(); i++)
			pacedTick();
		if (minecraft.level.get() != level.get())
		{
			std::cerr << "stress: server closed the level while settling" << std::endl;
			return 1;
		}
	}
	else if (!options.world.empty())
	{
		std::cerr << "stress: loading world " << options.world << std::endl;
		minecraft.selectLevel(String::fromUTF8(options.world));
		level = minecraft.level;
	}
	else
	{
		level = Util::make_shared<Level>(u"stress", Dimension::Id_Normal, seed);
		minecraft.gameMode = Util::make_shared<SurvivalMode>(minecraft);
		std::cerr << "stress: generating level" << std::endl;
		minecraft.setLevel(level, u"Stress");
		std::cerr << "stress: level bound" << std::endl;
	}
	if (level == nullptr || minecraft.player == nullptr)
	{
		std::cerr << "stress: the world produced no player" << std::endl;
		return 1;
	}
	if (noLighting)
	{
		if (online)
			std::cerr << "stress: --no-lighting ignored for server-driven levels" << std::endl;
		else
			level->lightingEnabled = false;
	}
	const double levelMs = millisBetween(levelStart, Clock::now());
	minecraft.setScreen(nullptr);
	minecraft.options.showDebugInfo = true;

	World world{ minecraft, *level, *minecraft.player, seed, options.tickInterval, online };

	const benchutil::ProcessMemory memoryAfterLevel = benchutil::processMemory();
	auto setupStart = Clock::now();
	std::cerr << "stress: setup " << scenario->name() << std::endl;
	scenario->setup(world, options.params);
	std::cerr << "stress: setup done, light queue " << level->lightUpdates.size() << std::endl;
	const double setupMs = millisBetween(setupStart, Clock::now());
	const std::size_t lightQueueAfterSetup = level->lightUpdates.size();
	const benchutil::ProcessMemory memoryAfterSetup = benchutil::processMemory();

	double settleLightMs = 0.0;
	long_t settleLightCalls = 0;
	double settleRebuildMs = 0.0;
	if (scenario->settleBeforeMeasure())
	{
		auto settleStart = Clock::now();
		while (level->updateLights())
			settleLightCalls++;
		settleLightCalls++;
		auto afterLights = Clock::now();
		settleLightMs = millisBetween(settleStart, afterLights);
		minecraft.gameRenderer.updateAllChunks();
		settleRebuildMs = millisBetween(afterLights, Clock::now());
		Chunk::updates = 0;
	}

	// In all-mode each scenario gets its own log, with -nolight on the
	// fullbright variant; a single scenario keeps --log (or its default).
	const std::string runLogPath = runs.size() > 1
		? std::string("stress-") + scenario->name() + (noLighting ? "-nolight-" : "-") +
			(options.nullSink ? std::string("null") : std::string(renderbackend::configuration().recordName)) + ".log"
		: options.logPath;
	std::unique_ptr<File> logFile(File::open(String::fromUTF8(runLogPath)));
	std::unique_ptr<std::ostream> out(logFile != nullptr ? logFile->toStreamOut() : nullptr);
	auto emit = [&](const std::string &line)
	{
		std::cout << line << '\n';
		if (out != nullptr)
			*out << line << '\n';
	};

	const int measuredFrames = options.frames > 0 ? options.frames : scenario->defaultFrames();
	const int totalFrames = options.warmupFrames + measuredFrames;

	emit("stress");
	emit("scenario " + std::string(scenario->name()));
	emit("backend " + std::string(renderbackend::configuration().recordName));
	emit(std::string("sink ") + (options.nullSink ? "null" : "backend"));
	emit("seed " + std::to_string(seed));
	emit(std::string("world ") + (online ? options.serverHost + ":" + std::to_string(options.serverPort) :
		options.world.empty() ? std::string("generated") : options.world));
	emit(std::string("online ") + (online ? "1" : "0"));
	emit("username " + String::toUTF8(minecraft.options.username));
	emit("width " + std::to_string(minecraft.width));
	emit("height " + std::to_string(minecraft.height));
	emit("view_distance " + std::to_string(options.viewDistance));
	emit("fancy_graphics " + std::to_string(minecraft.options.fancyGraphics ? 1 : 0));
	emit(std::string("lighting ") + (noLighting ? "0" : "1"));
	emit("tick_interval_frames " + std::to_string(options.tickInterval));
	emit("warmup_frames " + std::to_string(options.warmupFrames));
	emit("requested_measured_frames " + std::to_string(measuredFrames));
	emit("sample_every_frames " + std::to_string(options.sampleEvery));
	emit("finish_each_frame " + std::to_string(options.finishEachFrame ? 1 : 0));
	for (const auto &entry : options.params.values)
		emit("param_" + entry.first + " " + entry.second);
	emit("level_ms " + fmt(levelMs));
	emit("setup_ms " + fmt(setupMs));
	emit("setup_light_queue " + std::to_string(lightQueueAfterSetup));
	emit("settle_light_ms " + fmt(settleLightMs));
	emit("settle_light_calls " + std::to_string(settleLightCalls));
	emit("settle_rebuild_ms " + fmt(settleRebuildMs));
	emit("start_private_bytes " + std::to_string(memoryAtStart.privateBytes));
	emit("level_private_bytes " + std::to_string(memoryAfterLevel.privateBytes));
	emit("setup_private_bytes " + std::to_string(memoryAfterSetup.privateBytes));
	emit("sample_columns frame tick frame_ms tick_ms light_ms swap_ms render_ms finish_ms "
		"private_mb ws_mb chunks entities light_queue scheduled_ticks chunk_updates "
		"retained_mb resident_mb capacity_mb x y z");

	Window all;
	Window window;
	all.frame.reserve(static_cast<std::size_t>(measuredFrames));
	long_t gameTicks = 0;
	int_t measuredChunkUpdates = 0;
	int_t warmupChunkUpdates = 0;
	std::size_t peakLightQueue = 0;
	std::size_t warmupEndLightQueue = 0;
	std::size_t warmupEndChunks = 0;
	int disconnectedAtFrame = -1;
	Clock::time_point nextOnlineTickAt = Clock::now();
	benchutil::ProcessMemory memoryAtMeasureStart;
	benchutil::ProcessMemory memoryPeak;
	long_t loopStartMs = System::currentTimeMillis();
	long_t fpsMs = loopStartMs;
	int fpsFrames = 0;
	int_t fpsChunkUpdates = 0;

	auto sample = [&](int frame, const Window &w)
	{
		const benchutil::ProcessMemory memory = benchutil::processMemory();
		memoryPeak.privateBytes = std::max(memoryPeak.privateBytes, memory.privateBytes);
		memoryPeak.workingSetBytes = std::max(memoryPeak.workingSetBytes, memory.workingSetBytes);
		const legacygl::Context::RetainedGeometry retained = legacygl::context().retainedGeometry();
		legacygl::Sink::ResidentStats resident;
		const bool haveResident = legacygl::context().backendResidentStats(resident);
		emit("sample " + std::to_string(frame) + " " + std::to_string(gameTicks) + " " +
			fmt(meanOf(w.frame)) + " " + fmt(meanOf(w.tick)) + " " + fmt(meanOf(w.light)) + " " +
			fmt(meanOf(w.swap)) + " " + fmt(meanOf(w.render)) + " " + fmt(meanOf(w.finish)) + " " +
			mib(memory.privateBytes) + " " + mib(memory.workingSetBytes) + " " +
			std::to_string(LevelChunk::liveInstances) + " " +
			std::to_string(level->entities.size()) + " " +
			std::to_string(level->lightUpdates.size()) + " " +
			std::to_string(level->scheduledTickCount()) + " " +
			std::to_string(measuredChunkUpdates) + " " +
			mib(retained.vertexBytes + retained.commandBytes + retained.cachedBytes) + " " +
			(haveResident ? mib(resident.logicalBytes) : "0") + " " +
			(haveResident ? mib(resident.pageCapacityBytes) : "0") + " " +
			fmt(minecraft.player->x) + " " + fmt(minecraft.player->y) + " " + fmt(minecraft.player->z));
		// The F3 section/entity counters (rendered/total, frustum, occluded,
		// empty; entities rendered/total, culled) for the last frame.
		emit("render_stats " + std::to_string(frame) + " " +
			String::toUTF8(minecraft.levelRenderer.gatherStats1()) + " | " +
			String::toUTF8(minecraft.levelRenderer.gatherStats2()));
#ifdef A126_ENABLE_MEMORY_PROBE
		// Dirty-source attribution (fixture F1), cumulative since warm-up.
		memoryprobe::CountEntry entries[32];
		const std::size_t n = memoryprobe::counters(entries, 32);
		std::string line = "counters " + std::to_string(frame);
		for (std::size_t i = 0; i < n; i++)
			line += std::string(" ") + entries[i].name + "=" + std::to_string(entries[i].count);
		emit(line);
#endif
	};

	for (int frame = 0; frame < totalFrames; frame++)
	{
		AABB::resetPool();
		Vec3::resetPool();
		if (frame == options.warmupFrames)
		{
			std::cerr << "stress: warm-up done, measuring" << std::endl;
			A126_PROBE_REPORT("warmup");
			legacygl::resetPhaseProfile();
			memoryAtMeasureStart = benchutil::processMemory();
			// The renderer pulls the far view in during warm-up, and every new
			// chunk queues light records. This is the backlog the measured
			// frames start with; the port drains it 500 records per frame.
			warmupEndLightQueue = level->lightUpdates.size();
			warmupEndChunks = static_cast<std::size_t>(LevelChunk::liveInstances);
			Chunk::updates = 0;
			window.clear();
		}
		if (lwjgl::Display::isCloseRequested())
			break;
		if (online && minecraft.level.get() != level.get())
		{
			// Kicked or moved to another level: the World reference is dead.
			std::cerr << "stress: server closed the level at frame " << frame << std::endl;
			disconnectedAtFrame = frame;
			break;
		}

		if (online && frame % options.tickInterval == 0)
			std::this_thread::sleep_until(nextOnlineTickAt);
		auto start = Clock::now();
		if (frame % options.tickInterval == 0)
		{
			nextOnlineTickAt += std::chrono::milliseconds(50);
			minecraft.tick();
			scenario->onTick(world, gameTicks);
			gameTicks++;
		}
		auto afterTick = Clock::now();

		// Same cadence as Minecraft::run: singleplayer drains the queue every
		// frame, a server-fed level gets one 5000-record call.
		peakLightQueue = std::max(peakLightQueue, level->lightUpdates.size());
		if (online)
			level->updateLights();
		else
			while (level->updateLights())
				;
		auto afterLight = Clock::now();

		{
			A126_PROBE_SCOPE(memoryprobe::Bucket::Swap);
			lwjgl::Display::update();
		}
		auto afterSwap = Clock::now();

		const float partialTick = static_cast<float>(frame % options.tickInterval) /
			static_cast<float>(options.tickInterval);
		{
			A126_PROBE_SCOPE(memoryprobe::Bucket::Frame);
			minecraft.gameRenderer.render(partialTick);
		}
		auto afterRender = Clock::now();

		if (options.finishEachFrame)
			glFinish();
		auto afterFinish = Clock::now();

		const bool measured = frame >= options.warmupFrames;
		if (measured)
		{
			const double frameMs = millisBetween(start, afterFinish);
			const double tickMs = millisBetween(start, afterTick);
			const double lightMs = millisBetween(afterTick, afterLight);
			const double swapMs = millisBetween(afterLight, afterSwap);
			const double renderMs = millisBetween(afterSwap, afterRender);
			const double finishMs = millisBetween(afterRender, afterFinish);
			all.frame.push_back(frameMs); all.tick.push_back(tickMs); all.light.push_back(lightMs);
			all.swap.push_back(swapMs); all.render.push_back(renderMs); all.finish.push_back(finishMs);
			window.frame.push_back(frameMs); window.tick.push_back(tickMs); window.light.push_back(lightMs);
			window.swap.push_back(swapMs); window.render.push_back(renderMs); window.finish.push_back(finishMs);
			measuredChunkUpdates += Chunk::updates;
		}
		else
		{
			warmupChunkUpdates += Chunk::updates;
		}
		// The F3 overlay's "N fps, M chunk updates" line is filled by
		// Minecraft::run, which the harness replaces: keep it the same way.
		fpsChunkUpdates += Chunk::updates;
		fpsFrames++;
		while (System::currentTimeMillis() >= fpsMs + 1000)
		{
			minecraft.fpsString = String::fromUTF8(std::to_string(fpsFrames) + " fps, " +
				std::to_string(fpsChunkUpdates) + " chunk updates");
			fpsChunkUpdates = 0;
			fpsMs += 1000;
			fpsFrames = 0;
		}
		Chunk::updates = 0;

		const int measuredIndex = frame - options.warmupFrames + 1;
		if (measured && (measuredIndex % options.sampleEvery == 0 || measuredIndex == measuredFrames))
		{
			sample(measuredIndex, window);
			window.clear();
		}
	}

	if (!options.capturePath.empty() && !options.nullSink)
	{
		// Swap away the last loop frame, render once into a clean buffer, and
		// read it back before the next swap, exactly as the sign bench does.
		lwjgl::Display::update();
		minecraft.gameRenderer.render(0.0f);
		glFinish();
		int drawableWidth = 0;
		int drawableHeight = 0;
		platform::getDrawableSize(drawableWidth, drawableHeight);
		std::vector<unsigned char> pixels(
			static_cast<std::size_t>(drawableWidth) * static_cast<std::size_t>(drawableHeight) * 3);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, drawableWidth, drawableHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
		stbi_flip_vertically_on_write(1);
		const int written = stbi_write_png(options.capturePath.c_str(), drawableWidth, drawableHeight, 3,
			pixels.data(), drawableWidth * 3);
		stbi_flip_vertically_on_write(0);
		std::cerr << "stress: " << (written != 0 ? "wrote " : "could not write ") << options.capturePath << std::endl;
	}

	const double loopSeconds =
		static_cast<double>(System::currentTimeMillis() - loopStartMs) / 1000.0;
	std::vector<double> sorted = all.frame;
	std::sort(sorted.begin(), sorted.end());
	const double mean = meanOf(all.frame);
	const benchutil::ProcessMemory memoryAtEnd = benchutil::processMemory();

	emit("measured_frames " + std::to_string(all.frame.size()));
	emit("disconnected_at_frame " + std::to_string(disconnectedAtFrame));
	emit("game_ticks " + std::to_string(gameTicks));
	emit("ticks_per_second " + fmt(loopSeconds > 0.0 ? gameTicks / loopSeconds : 0.0));
	emit("warmup_chunk_updates " + std::to_string(warmupChunkUpdates));
	emit("chunk_updates " + std::to_string(measuredChunkUpdates));
	emit("peak_light_queue " + std::to_string(peakLightQueue));
	emit("end_light_queue " + std::to_string(level->lightUpdates.size()));
	emit("end_entities " + std::to_string(level->entities.size()));
	emit("end_live_chunks " + std::to_string(LevelChunk::liveInstances));
	emit("end_scheduled_ticks " + std::to_string(level->scheduledTickCount()));
	emit("chunk_source " + String::toUTF8(level->gatherChunkSourceStats()));
	emit("mean_ms " + fmt(mean));
	emit("min_ms " + fmt(sorted.empty() ? 0.0 : sorted.front()));
	emit("p50_ms " + fmt(benchutil::percentile(sorted, 0.5)));
	emit("p95_ms " + fmt(benchutil::percentile(sorted, 0.95)));
	emit("p99_ms " + fmt(benchutil::percentile(sorted, 0.99)));
	emit("max_ms " + fmt(sorted.empty() ? 0.0 : sorted.back()));
	emit("mean_fps " + fmt(mean > 0.0 ? 1000.0 / mean : 0.0));
	emit("mean_tick_ms " + fmt(meanOf(all.tick)));
	emit("mean_light_ms " + fmt(meanOf(all.light)));
	emit("mean_swap_ms " + fmt(meanOf(all.swap)));
	emit("mean_render_ms " + fmt(meanOf(all.render)));
	emit("mean_finish_ms " + fmt(meanOf(all.finish)));
	emit("measure_start_private_bytes " + std::to_string(memoryAtMeasureStart.privateBytes));
	emit("warmup_end_light_queue " + std::to_string(warmupEndLightQueue));
	emit("warmup_end_live_chunks " + std::to_string(warmupEndChunks));
	emit("end_private_bytes " + std::to_string(memoryAtEnd.privateBytes));
	emit("end_working_set_bytes " + std::to_string(memoryAtEnd.workingSetBytes));
	emit("peak_private_bytes " + std::to_string(memoryPeak.privateBytes));
	emit("private_growth_bytes " + std::to_string(
		static_cast<std::int64_t>(memoryAtEnd.privateBytes) -
		static_cast<std::int64_t>(memoryAtMeasureStart.privateBytes)));

	const legacygl::Context::RetainedGeometry retained = legacygl::context().retainedGeometry();
	emit("legacygl_lists " + std::to_string(retained.lists));
	emit("legacygl_geometries " + std::to_string(retained.geometries));
	emit("legacygl_vertices " + std::to_string(retained.vertices));
	emit("legacygl_vertex_bytes " + std::to_string(retained.vertexBytes));
	emit("legacygl_command_bytes " + std::to_string(retained.commandBytes));
	emit("legacygl_cached_bytes " + std::to_string(retained.cachedBytes));
	legacygl::Sink::ResidentStats resident;
	if (legacygl::context().backendResidentStats(resident))
	{
		emit("resident_logical_bytes " + std::to_string(resident.logicalBytes));
		emit("resident_entries " + std::to_string(resident.entries));
		emit("resident_pages " + std::to_string(resident.pages));
		emit("resident_capacity_bytes " + std::to_string(resident.pageCapacityBytes));
		emit("resident_batched_draws " + std::to_string(resident.batchedDraws));
		emit("resident_multidraws " + std::to_string(resident.multidraws));
		emit("resident_block_overflows " + std::to_string(resident.batchBlockOverflows));
	}
	emit("heap_top" + benchutil::heapHistogram(16));

	std::vector<std::string> extra;
	scenario->report(world, extra);
	for (const std::string &line : extra)
		emit(line);

	benchutil::emitPhaseProfile(emit);

	A126_PROBE_REPORT("stress");
#ifdef A126_ENABLE_MEMORY_PROBE
	// The probe truncates its file per process, so a battery of runs keeps
	// only the last one. Fold this run's bucket reports into its own log.
	{
		std::unique_ptr<File> probeFile(File::open(u"a126cpp-profile.txt"));
		std::unique_ptr<std::istream> in(probeFile != nullptr && probeFile->exists() ? probeFile->toStreamIn() : nullptr);
		if (in != nullptr)
		{
			emit("probe_report_begin");
			std::string probeLine;
			while (std::getline(*in, probeLine))
				emit("  " + probeLine);
			emit("probe_report_end");
		}
	}
#endif
	if (online && minecraft.level.get() == level.get())
	{
		MultiPlayerLevel *mp = dynamic_cast<MultiPlayerLevel *>(level.get());
		if (mp != nullptr && mp->getConnection() != nullptr)
			mp->getConnection()->disconnect();
	}
	// Release the level and its player before the next scenario binds its
	// own; the window, context, and backend stay up for the whole process.
	} // for each lit/unlit scenario run
	minecraft.stop();
	BackgroundTask::joinAll();
	return 0;
}
catch (const std::exception &e)
{
	std::cerr << "stress failed: " << e.what() << std::endl;
	return 1;
}

}
