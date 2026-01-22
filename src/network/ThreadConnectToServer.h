#pragma once

#include "java/Type.h"
#include "java/String.h"

// Forward declarations
class GuiConnecting;
class Minecraft;

// ThreadConnectToServer - matches Java ThreadConnectToServer.java exactly
class ThreadConnectToServer {
private:
	GuiConnecting* connectingGui;
	Minecraft* mc;
	jstring hostName;
	int_t port;
	
public:
	ThreadConnectToServer(GuiConnecting* gui, Minecraft* mc, const jstring& hostName, int_t port);
	void run();  // Java Thread.run()
};
