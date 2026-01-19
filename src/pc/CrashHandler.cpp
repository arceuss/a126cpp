#include "CrashHandler.h"

#include <SDL3/SDL.h>

namespace CrashHandler
{

void Crash(const std::string &message, const std::string &stackTrace)
{
	std::string text = message + "\n\n" + stackTrace;
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Minecraft has crashed!", text.c_str(), nullptr);
}

}
