#include "client/gui/GuiConnecting.h"
#include "client/gui/GuiConnectFailed.h"
#include "client/title/TitleScreen.h"
#include "network/NetClientHandler.h"
#include "network/ThreadConnectToServer.h"
#include "client/Minecraft.h"

GuiConnecting::GuiConnecting(Minecraft &minecraft, const jstring &hostName, int_t port)
	: Screen(minecraft), clientHandler(nullptr), cancelled(false), hostName(hostName)
{
	// Clear world
	minecraft.setLevel(nullptr);
	
	// Start connection thread
	ThreadConnectToServer *thread = new ThreadConnectToServer(this, &minecraft, hostName, port);
	std::thread([thread]() {
		thread->run();
		delete thread;
	}).detach();
}

void GuiConnecting::tick()
{
	// Alpha 1.2.6: GuiConnecting.updateScreen() - only processes read packets
	// Java: if(this.clientHandler != null) { this.clientHandler.processReadPackets(); }
	if (clientHandler != nullptr)
	{
		clientHandler->processReadPackets();
	}
}

void GuiConnecting::keyPressed(char_t eventCharacter, int_t eventKey)
{
	// No key handling during connection
}

void GuiConnecting::init()
{
	buttons.clear();
	buttons.push_back(std::make_shared<Button>(0, width / 2 - 100, height / 4 + 120 + 12, u"Cancel"));
}

void GuiConnecting::buttonClicked(Button &button)
{
	if (button.id == 0) // Cancel
	{
		cancelled = true;
		if (clientHandler != nullptr)
		{
			clientHandler->disconnect();
		}
		minecraft.setScreen(std::make_shared<TitleScreen>(minecraft));
	}
}

void GuiConnecting::render(int_t xm, int_t ym, float a)
{
	// Alpha 1.2.6: GuiConnecting.drawScreen() - 1:1 port
	renderBackground();
	if (clientHandler == nullptr)
	{
		// Java: this.drawCenteredString(this.fontRenderer, "Connecting to the server...", ...);
		drawCenteredString(font, u"Connecting to the server...", width / 2, height / 2 - 50, 16777215);
		// Java: this.drawCenteredString(this.fontRenderer, "", ...);
		drawCenteredString(font, u"", width / 2, height / 2 - 10, 16777215);
	}
	else
	{
		// Alpha 1.2.6: GuiConnecting.drawScreen() - only shows "Logging in..." when handler exists
		// Java: this.drawCenteredString(this.fontRenderer, "Logging in...", ...);
		drawCenteredString(font, u"Logging in...", width / 2, height / 2 - 50, 16777215);
		// Java: this.drawCenteredString(this.fontRenderer, this.clientHandler.field_1209_a, ...);
		// Note: field_1209_a should be set by server, not the IP/hostname
		// Only show if it's actually the server name (not the initial hostname)
		// For now, show empty string until server sends its name
		drawCenteredString(font, u"", width / 2, height / 2 - 10, 16777215);
	}
	Screen::render(xm, ym, a);
}

void GuiConnecting::setNetClientHandler(GuiConnecting *gui, NetClientHandler *handler)
{
	gui->clientHandler = handler;
}

bool GuiConnecting::isCancelled(GuiConnecting *gui)
{
	return gui->cancelled;
}

NetClientHandler *GuiConnecting::getNetClientHandler(GuiConnecting *gui)
{
	return gui->clientHandler;
}
