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
		if (netHandler != nullptr)
		{
			netHandler->addToSendQueue(new Packet0KeepAlive());
		}
	}

	if (netHandler != nullptr)
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
