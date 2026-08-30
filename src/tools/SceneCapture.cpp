#include "tools/SceneCapture.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "client/Minecraft.h"
#include "client/gamemode/SurvivalMode.h"
#include "java/Random.h"
#include "java/String.h"
#include "java/System.h"
#include "legacygl/Context.h"
#include "legacygl/Trace.h"
#include "lwjgl/Display.h"
#include "util/Memory.h"
#include "client/player/LocalPlayer.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"
#include "world/phys/AABB.h"
#include "world/phys/Vec3.h"
#include "OpenGL.h"

#include "stb_image_write.h"

// Deterministic scene capture.
//
// The frame is produced by the same code the client runs: the real timer, the
// real tick, the real GameRenderer. Nothing here reaches around the renderer to
// draw something simpler, because a capture that does not go through the
// production path proves nothing about the production path.
//
// Determinism comes from a fixed seed, a fixed time of day, a fixed camera and a
// fixed number of warm-up frames. The default world is generated in memory, so a
// capture does not depend on a save file that a previous run may have modified.

struct CaptureOptions
{
	std::string output = "capture.png";
	int width = 1920;
	int height = 1080;
	// Chunk display lists are built a few per frame, so the capture waits for
	// the renderer to settle before reading the framebuffer.
	int warmupFrames = 240;
	long long seed = 1234567LL;
	// Empty means "generate an in-memory level from the seed"; otherwise the
	// named save under .mcbetacpp/saves is loaded.
	std::string world;
	// Noon, so the capture does not depend on the day/night phase.
	long long timeOfDay = 6000;
	bool hasPosition = false;
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	// Alpha's F3 overlay. Useful when a capture does not look like what was
	// expected: it reports the camera position and the chunk counts.
	bool debugOverlay = false;
	float yaw = 0.0f;
	float pitch = 0.0f;
	bool drawGui = true;
	bool listScenes = false;
};

static bool parseInt(const char *text, int &out)
{
	char *end = nullptr;
	const long value = std::strtol(text, &end, 10);
	if (end == text || *end != '\0')
		return false;
	out = static_cast<int>(value);
	return true;
}

static bool parseLongLong(const char *text, long long &out)
{
	char *end = nullptr;
	out = std::strtoll(text, &end, 10);
	return end != text && *end == '\0';
}

static bool parseDouble(const char *text, double &out)
{
	char *end = nullptr;
	out = std::strtod(text, &end);
	return end != text && *end == '\0';
}

static void printUsage()
{
	std::cout <<
		"Usage: Alpha126Cpp --capture [options]\n"
		"\n"
		"  --out <path>        output PNG (default capture.png)\n"
		"  --width <n>         capture width in pixels (default 1920)\n"
		"  --height <n>        capture height in pixels (default 1080)\n"
		"  --seed <n>          world seed for the generated level (default 1234567)\n"
		"  --world <name>      load this save from .mcbetacpp/saves instead of generating\n"
		"  --time <ticks>      level time of day, 0-23999 (default 6000, noon)\n"
		"  --warmup <n>        frames to render before capturing (default 240)\n"
		"  --pos <x> <y> <z>   camera position (default: spawn, on the surface)\n"
		"  --rot <yaw> <pitch> camera rotation in degrees (default 0 0)\n"
		"  --no-gui            render the world only, without the HUD\n"
		"  --debug             draw the F3 overlay (camera position, chunk counts)\n"
		"\n"
		"The capture runs headless: the window stays hidden, no input is read and\n"
		"the camera is held at the requested position and angles. Two runs with the\n"
		"same arguments produce byte-identical PNGs.\n"
		"\n"
		"The resolution is the window size, so it is limited by what the driver\n"
		"will give a window on this display.\n";
}

static bool parseOptions(int argc, char **argv, int firstArgument, CaptureOptions &options)
{
	for (int i = firstArgument; i < argc; i++)
	{
		const std::string argument = argv[i];
		const bool hasNext = (i + 1) < argc;

		if (argument == "--help")
		{
			options.listScenes = true;
			return true;
		}
		if (argument == "--debug")
		{
			options.debugOverlay = true;
			continue;
		}
		if (argument == "--no-gui")
		{
			options.drawGui = false;
			continue;
		}
		if (argument == "--out" && hasNext)
		{
			options.output = argv[++i];
			continue;
		}
		if (argument == "--world" && hasNext)
		{
			options.world = argv[++i];
			continue;
		}
		if (argument == "--width" && hasNext)
		{
			if (!parseInt(argv[++i], options.width))
				return false;
			continue;
		}
		if (argument == "--height" && hasNext)
		{
			if (!parseInt(argv[++i], options.height))
				return false;
			continue;
		}
		if (argument == "--warmup" && hasNext)
		{
			if (!parseInt(argv[++i], options.warmupFrames))
				return false;
			continue;
		}
		if (argument == "--seed" && hasNext)
		{
			if (!parseLongLong(argv[++i], options.seed))
				return false;
			continue;
		}
		if (argument == "--time" && hasNext)
		{
			if (!parseLongLong(argv[++i], options.timeOfDay))
				return false;
			continue;
		}
		if (argument == "--pos" && (i + 3) < argc)
		{
			if (!parseDouble(argv[i + 1], options.x) || !parseDouble(argv[i + 2], options.y) ||
				!parseDouble(argv[i + 3], options.z))
				return false;
			options.hasPosition = true;
			i += 3;
			continue;
		}
		if (argument == "--rot" && (i + 2) < argc)
		{
			double yaw = 0.0;
			double pitch = 0.0;
			if (!parseDouble(argv[i + 1], yaw) || !parseDouble(argv[i + 2], pitch))
				return false;
			options.yaw = static_cast<float>(yaw);
			options.pitch = static_cast<float>(pitch);
			i += 2;
			continue;
		}

		std::cerr << "capture: unknown option " << argument << '\n';
		return false;
	}

	if (options.width <= 0 || options.height <= 0)
	{
		std::cerr << "capture: width and height must be positive\n";
		return false;
	}
	if (options.warmupFrames < 0)
		options.warmupFrames = 0;
	return true;
}

// Reads the back buffer before it is swapped. Reading after the swap would
// sample a buffer whose contents OpenGL leaves undefined.
static bool writeCapture(const CaptureOptions &options)
{
	std::vector<unsigned char> pixels(
		static_cast<std::size_t>(options.width) * static_cast<std::size_t>(options.height) * 3);

	// Rows are three bytes per pixel, so an unpadded read needs alignment one.
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, options.width, options.height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	// glReadPixels returns rows bottom-up from the lower-left origin; PNG rows
	// run top-down.
	stbi_flip_vertically_on_write(1);
	const int written = stbi_write_png(options.output.c_str(), options.width, options.height, 3, pixels.data(),
		options.width * 3);
	stbi_flip_vertically_on_write(0);

	if (written == 0)
	{
		std::cerr << "capture: could not write " << options.output << '\n';
		return false;
	}
	return true;
}

int runSceneCapture(int argc, char **argv, int firstArgument)
try
{
	CaptureOptions options;
	if (!parseOptions(argc, argv, firstArgument, options))
	{
		printUsage();
		return 2;
	}
	if (options.listScenes)
	{
		printUsage();
		return 0;
	}

	// Gate A compares the production render call, not timing-dependent setup,
	// chunk-list construction or the fixture's framebuffer readback.
	legacygl::traceCaptureFrameOnly();

	// The game's no-argument Random instances intentionally use the clock.
	// Capture advances the real simulation, so seed those future instances too.
	Random::setDefaultSeedForCapture(static_cast<long_t>(options.seed));

	Minecraft minecraft(options.width, options.height, false);
	// Keep the platform window hidden and suppress the focus-loss pause menu.
	minecraft.unattended = true;
	minecraft.init();

	if (options.world.empty())
	{
		// In-memory level: generated from the seed, never written to disk, so a
		// capture cannot be perturbed by an earlier run.
		std::shared_ptr<Level> level = Util::make_shared<Level>(
			u"scene-capture", Dimension::Id_Normal, static_cast<long_t>(options.seed));
		minecraft.gameMode = Util::make_shared<SurvivalMode>(minecraft);
		minecraft.setLevel(level, u"Generating level");
	}
	else
	{
		minecraft.gameMode = Util::make_shared<SurvivalMode>(minecraft);
		minecraft.selectLevel(String::fromUTF8(options.world));
	}

	if (minecraft.player == nullptr || minecraft.level == nullptr)
	{
		std::cerr << "capture: the world produced no player\n";
		return 1;
	}

	minecraft.setScreen(nullptr);
	minecraft.level->time = static_cast<long_t>(options.timeOfDay);
	minecraft.options.showDebugInfo = options.debugOverlay;

	// The camera the capture asked for. It is reapplied after every tick: the
	// client's tick reads look input and runs player physics, and a capture that
	// drifted or fell would not be reproducible. This pinning is local to the
	// fixture; nothing in the renderer changes.
	double cameraX = options.x;
	double cameraY = options.y;
	double cameraZ = options.z;
	if (!options.hasPosition)
	{
		// Feet on the surface at the spawn column. Alpha's camera sits 1.62
		// blocks above the feet, so this is eye level, and standing on ground
		// keeps the player from accruing fall damage during the warm-up.
		const int_t spawnX = minecraft.level->xSpawn;
		const int_t spawnZ = minecraft.level->zSpawn;
		cameraX = spawnX + 0.5;
		cameraY = static_cast<double>(minecraft.level->getHeightmap(spawnX, spawnZ));
		cameraZ = spawnZ + 0.5;
	}

	auto pinCamera = [&]()
	{
		LocalPlayer &player = *minecraft.player;
		player.moveTo(cameraX, cameraY, cameraZ, options.yaw, options.pitch);
		// moveTo keeps the previous rotation for interpolation; a fixed camera
		// wants the interpolated view to equal the requested one exactly.
		player.yRotO = options.yaw;
		player.xRotO = options.pitch;
		player.xd = 0.0;
		player.yd = 0.0;
		player.zd = 0.0;
		// A camera placed in mid-air by --pos would fall, take damage and end up
		// showing the death screen instead of the scene.
		player.health = 20;
	};

	pinCamera();

	// Let the world settle: lighting, then chunk meshes. updateAllChunks forces
	// the whole visible set instead of the handful the renderer would rebuild
	// per frame, so the warm-up does not have to be long enough to cover them.
	while (minecraft.level->updateLights())
		;
	minecraft.gameRenderer.updateAllChunks();

	// A fixed schedule, not the wall-clock Timer: the client's timer decides how
	// many ticks a frame owes from elapsed real time, so two runs of the same
	// capture would simulate different amounts of world and produce different
	// pixels. One tick per frame and a fixed partial tick make the frame a
	// function of the seed, the camera, the time of day and the frame count.
	//
	// The partial tick is deliberately not zero: GameRenderer's hurt tilt takes
	// its branch when hurtTime - a is exactly zero and then divides by a
	// hurtDuration that is still zero, so a frame at a == 0 asks GL to rotate by
	// NaN.
	const float partialTick = 0.5f;
	for (int frame = 0; frame <= options.warmupFrames; frame++)
	{
		AABB::resetPool();
		Vec3::resetPool();

		// The time of day is forced before the tick, not after: the tick is what
		// derives the sky darkening and the light-map from it, so overwriting
		// afterwards would leave the world lit for whatever time the save
		// happened to hold.
		minecraft.level->time = static_cast<long_t>(options.timeOfDay);

		if (minecraft.screen != nullptr)
			minecraft.setScreen(nullptr);

		minecraft.tick();

		pinCamera();
		minecraft.level->time = static_cast<long_t>(options.timeOfDay);

		while (minecraft.level->updateLights())
			;

		// The renderer rebuilds only a few chunks per frame, and ticking keeps
		// marking them dirty, so the frame that gets captured forces the whole
		// visible set first. Without this the capture can show half-built
		// terrain, or none at all.
		if (frame == options.warmupFrames)
		{
			minecraft.gameRenderer.updateAllChunks();
			legacygl::traceBeginCaptureFrame(legacygl::context().sequence());
			// renderHit animates from the wall clock. Fix it only for the frame
			// whose pixels and call stream are compared between backends.
			System::setCurrentTimeMillisForCapture(0);
		}

		try
		{
			if (options.drawGui)
				minecraft.gameRenderer.render(partialTick);
			else
				minecraft.gameRenderer.renderLevel(partialTick);
		}
		catch (...)
		{
			if (frame == options.warmupFrames)
			{
				System::clearCurrentTimeMillisForCapture();
				legacygl::traceEndCaptureFrame();
			}
			throw;
		}

		if (frame == options.warmupFrames)
		{
			System::clearCurrentTimeMillisForCapture();
			legacygl::traceEndCaptureFrame();
			break;
		}

		// Swap without pumping the event queue: no mouse motion, no key state
		// and no focus change can reach the client, so the camera stays exactly
		// where the capture put it.
		lwjgl::Display::update(false);
	}

	const bool written = writeCapture(options);

	std::cout << "capture: " << options.width << 'x' << options.height << " -> " << options.output
			  << (options.world.empty() ? " (generated seed " + std::to_string(options.seed) + ")"
										: " (world " + options.world + ")")
			  << '\n';

	minecraft.stop();
	return written ? 0 : 1;
}
catch (const std::exception &e)
{
	std::cerr << "capture failed: " << e.what() << std::endl;
	return 1;
}
