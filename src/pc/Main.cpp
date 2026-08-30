#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <memory>

#include "client/Minecraft.h"
#include "tools/SignBench.h"
#include "tools/SceneCapture.h"

#include "lwjgl/GLContext.h"
#include "java/File.h"
#include "java/String.h"
#include "java/System.h"

int main(int argc, char *argv[])
{
	lwjgl::GLContext::instantiate();

	// Developer fixture, not part of the game: renders a wall of signs and logs
	// frame times. Usage: --sign-bench [frames] [signs] [blankText]
	if (argc >= 2 && std::strcmp(argv[1], "--sign-bench") == 0)
	{
		int frames = (argc >= 3) ? std::atoi(argv[2]) : 0;
		int signs = (argc >= 4) ? std::atoi(argv[3]) : 0;
		bool blankText = (argc >= 5) && std::atoi(argv[4]) != 0;
		return runSignBench(frames, signs, blankText);
	}

	if (argc >= 2 && std::strcmp(argv[1], "--timer-probe") == 0)
		return runTimerProbe((argc >= 3) ? std::atoi(argv[2]) : 0);

	// Developer fixture, not part of the game: renders one deterministic frame
	// at a requested resolution and writes it to a PNG. This is the framebuffer
	// side of the parity harness. Usage: --capture [--help]
	if (argc >= 2 && std::strcmp(argv[1], "--capture") == 0)
		return runSceneCapture(argc, argv, 2);

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
	if (argc >= 2 && std::strlen(argv[1]) > 0)
		username = String::fromUTF8(argv[1]);

	jstring auth = u"-";
	if (argc >= 3 && std::strlen(argv[2]) > 1)
		auth = String::fromUTF8(argv[2]);

	Minecraft::start(&username, &auth);

	return 0;
}
