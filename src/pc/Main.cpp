#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <memory>

#include "backends/Backend.h"
#include "client/Minecraft.h"
#include "tools/SignBench.h"
#include "tools/SceneCapture.h"

#include "lwjgl/GLContext.h"
#include "java/File.h"
#include "java/String.h"
#include "java/System.h"

int main(int argc, char *argv[])
{
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
