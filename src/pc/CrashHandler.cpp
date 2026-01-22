#include "CrashHandler.h"

#include "external/CrashMessageBox.h"

namespace CrashHandler
{

void Crash(const std::string &message, const std::string &stackTrace)
{
	std::string text = message + "\n\n" + stackTrace;
	CrashMessageBox::showError("Minecraft has crashed!", text);
}

}
