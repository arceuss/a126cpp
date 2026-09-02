#ifdef __SWITCH__
#include "switch/SwitchRuntime.h"
#endif

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <memory>
#include <vector>

#include "backends/Backend.h"
#include "client/Minecraft.h"
#include "client/renderer/culling/OcclusionCuller.h"
#include "tools/SignBench.h"
#include "tools/SceneCapture.h"

#include "lwjgl/GLContext.h"
#include "java/File.h"
#include "java/String.h"
#include "java/System.h"


#ifdef __SWITCH__
// libnx calls userAppInit before the C++ static constructors and userAppExit
// after the destructors, which is the only correct place for romfs: the game
// has static objects that read resources.
extern "C" void userAppInit(void)
{
	switchruntime::initialize();
}

extern "C" void userAppExit(void)
{
	switchruntime::shutdown();
}
#endif

#if defined(__SWITCH__) && defined(A126_ENABLE_MEMORY_PROBE)
// Homebrew launched from an emulator or hbmenu gets no argv, which leaves the
// developer fixtures (--sign-bench and friends) unreachable on the console. The
// profiling build reads them from a file instead, one whitespace-separated
// command line, so a bench can run unattended. Absent file: normal startup.
static std::vector<std::string> readArgumentFile()
{
	std::vector<std::string> arguments;
	FILE *file = std::fopen("sdmc:/switch/a126cpp-args.txt", "r");
	if (file == nullptr)
		return arguments;
	std::string content;
	char chunk[256];
	std::size_t read = 0;
	while ((read = std::fread(chunk, 1, sizeof(chunk), file)) > 0)
		content.append(chunk, read);
	std::fclose(file);
	std::istringstream stream(content);
	std::string token;
	while (stream >> token)
		arguments.push_back(token);
	if (!arguments.empty())
		std::fprintf(stderr, "switch: %zu arguments read from a126cpp-args.txt\n", arguments.size());
	return arguments;
}
#endif

int main(int argc, char *argv[])
{
#if defined(__SWITCH__) && defined(A126_ENABLE_MEMORY_PROBE)
	const std::vector<std::string> fileArguments = readArgumentFile();
	std::vector<char *> fileArgv;
	if (!fileArguments.empty())
	{
		fileArgv.push_back(argc > 0 ? argv[0] : const_cast<char *>("Alpha126Cpp"));
		for (const std::string &argument : fileArguments)
			fileArgv.push_back(const_cast<char *>(argument.c_str()));
		argc = static_cast<int>(fileArgv.size());
		argv = fileArgv.data();
	}
#endif

	int firstArgument = 1;
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--backend") == 0)
	{
		if (firstArgument + 1 >= argc || std::strcmp(argv[firstArgument + 1], "--") == 0)
		{
			std::fprintf(stderr, "error: --backend requires a name\n");
			return 2;
		}
		if (std::strcmp(argv[firstArgument + 1], "--backend") == 0)
		{
			std::fprintf(stderr, "error: duplicate --backend option\n");
			return 2;
		}
		if (!renderbackend::select(argv[firstArgument + 1]))
		{
			std::fprintf(stderr, "error: unknown or unavailable backend '%s'\n", argv[firstArgument + 1]);
			return 2;
		}
		firstArgument += 2;
		if (firstArgument < argc && std::strcmp(argv[firstArgument], "--backend") == 0)
		{
			std::fprintf(stderr, "error: duplicate --backend option\n");
			return 2;
		}
	}
	// Developer fixture control, for A/B runs of any mode below: draws the
	// entities the terrain hides from the player.
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--no-occlusion") == 0)
	{
		OcclusionCuller::enabled = false;
		++firstArgument;
	}
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--") == 0)
		++firstArgument;

	lwjgl::GLContext::instantiate();

	// Developer fixture, not part of the game: renders a wall of signs or an
	// existing saved world and logs frame times. Usage:
	// --sign-bench [frames] [signs] [blankText] [finishEachFrame] [world]
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--sign-bench") == 0)
	{
		int frames = (firstArgument + 1 < argc) ? std::atoi(argv[firstArgument + 1]) : 0;
		int signs = (firstArgument + 2 < argc) ? std::atoi(argv[firstArgument + 2]) : 0;
		bool blankText = (firstArgument + 3 < argc) && std::atoi(argv[firstArgument + 3]) != 0;
		bool finishEachFrame = (firstArgument + 4 >= argc) || std::atoi(argv[firstArgument + 4]) != 0;
		std::string worldName = (firstArgument + 5 < argc) ? argv[firstArgument + 5] : "";
		return runSignBench(frames, signs, blankText, finishEachFrame, worldName);
	}

	// Developer fixture: walks the production renderer through a repeating
	// chunk route and records frame time and process memory by cycle. Usage:
	// --chunk-travel-bench [cycles] [radiusChunks] [framesPerChunk] [settleFrames]
	//                      [viewDistance] [world]
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--chunk-travel-bench") == 0)
	{
		int cycles = (firstArgument + 1 < argc) ? std::atoi(argv[firstArgument + 1]) : 0;
		int radius = (firstArgument + 2 < argc) ? std::atoi(argv[firstArgument + 2]) : 0;
		int framesPerChunk = (firstArgument + 3 < argc) ? std::atoi(argv[firstArgument + 3]) : 0;
		int settleFrames = (firstArgument + 4 < argc) ? std::atoi(argv[firstArgument + 4]) : 0;
		int viewDistance = (firstArgument + 5 < argc) ? std::atoi(argv[firstArgument + 5]) : 0;
		std::string worldName = (firstArgument + 6 < argc) ? argv[firstArgument + 6] : "";
		return runChunkTravelBench(cycles, radius, framesPerChunk, settleFrames,
			viewDistance, worldName);
	}

	// Developer fixture: teleports the player over ever-new terrain in a
	// straight line and samples process memory at a chunk interval. Usage:
	// --chunk-march-bench [chunks] [framesPerChunk] [sampleEveryChunks]
	//                     [viewDistance] [world]
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--chunk-march-bench") == 0)
	{
		int chunks = (firstArgument + 1 < argc) ? std::atoi(argv[firstArgument + 1]) : 0;
		int framesPerChunk = (firstArgument + 2 < argc) ? std::atoi(argv[firstArgument + 2]) : 0;
		int sampleEvery = (firstArgument + 3 < argc) ? std::atoi(argv[firstArgument + 3]) : 0;
		int viewDistance = (firstArgument + 4 < argc) ? std::atoi(argv[firstArgument + 4]) : 0;
		std::string worldName = (firstArgument + 5 < argc) ? argv[firstArgument + 5] : "";
		return runChunkMarchBench(chunks, framesPerChunk, sampleEvery, viewDistance, worldName);
	}
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--timer-probe") == 0)
		return runTimerProbe((firstArgument + 1 < argc) ? std::atoi(argv[firstArgument + 1]) : 0);

	// Developer fixture, not part of the game: renders one deterministic frame
	// at a requested resolution and writes it to a PNG. This is the framebuffer
	// side of the parity harness. Usage: --capture [--help]
	if (firstArgument < argc && std::strcmp(argv[firstArgument], "--capture") == 0)
		return runSceneCapture(argc, argv, firstArgument + 1);

	// Try to read username from config file
	jstring username = u"Player" + String::fromUTF8(std::to_string(System::currentTimeMillis() % 1000));
	std::unique_ptr<File> workDir(File::openWorkingDirectory(u".mcbetacpp"));
	if (workDir != nullptr)
	{
		std::unique_ptr<File> optionsFile(File::open(*workDir, u"options.txt"));
		if (optionsFile != nullptr && optionsFile->exists())
		{
			std::unique_ptr<std::istream> is(optionsFile->toStreamIn());
			if (is != nullptr)
			{
				std::string line;
				while (std::getline(*is, line))
				{
					if (line.back() == '\r')
						line.pop_back();

					size_t pos = line.find(':');
					if (pos == std::string::npos)
						continue;

					std::string key = line.substr(0, pos);
					std::string value = line.substr(pos + 1);

					if (key == "username" && !value.empty())
					{
						username = String::fromUTF8(value);
						break;
					}
				}
			}
		}
	}

	// Command-line argument overrides config
	if (firstArgument < argc && std::strlen(argv[firstArgument]) > 0)
		username = String::fromUTF8(argv[firstArgument]);

	jstring auth = u"-";
	if (firstArgument + 1 < argc && std::strlen(argv[firstArgument + 1]) > 1)
		auth = String::fromUTF8(argv[firstArgument + 1]);

	Minecraft::start(&username, &auth);

	return 0;
}
