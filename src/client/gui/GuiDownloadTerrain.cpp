#include "client/gui/GuiDownloadTerrain.h"
#include "network/NetClientHandler.h"
#include "network/Packet0KeepAlive.h"

GuiDownloadTerrain::GuiDownloadTerrain(Minecraft &minecraft, NetClientHandler* handler)
	: Screen(minecraft), netHandler(handler), updateCounter(0)
{
}

void GuiDownloadTerrain::tick()
{
	++updateCounter;
	if (updateCounter % 20 == 0)
	{
		// Alpha 1.2.6: Send keep-alive packet every 20 ticks
		// Only send if handler is valid and not disconnected
		if (netHandler != nullptr && !netHandler->isDisconnected())
		{
			netHandler->addToSendQueue(new Packet0KeepAlive());
		}
	}

	// Alpha 1.2.6: Process read packets every tick
	// Only process if handler is valid and not disconnected
	// This prevents processing packets on a closed/invalid connection during dimension transitions
	if (netHandler != nullptr && !netHandler->isDisconnected())
	{
		netHandler->processReadPackets();
	}
}

void GuiDownloadTerrain::keyPressed(char_t eventCharacter, int_t eventKey)
{
	// No key handling
}

void GuiDownloadTerrain::init()
{
	buttons.clear();
}

void GuiDownloadTerrain::buttonClicked(Button &button)
{
	// No buttons
}

void GuiDownloadTerrain::render(int_t xm, int_t ym, float a)
{
	renderBackground(0);
	drawCenteredString(font, u"Downloading terrain", width / 2, height / 2 - 50, 16777215);
	Screen::render(xm, ym, a);
}
