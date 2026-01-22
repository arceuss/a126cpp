#include "client/gui/GuiConnectFailed.h"
#include "client/title/TitleScreen.h"
#include "client/Minecraft.h"

GuiConnectFailed::GuiConnectFailed(Minecraft &minecraft, const jstring &message, const jstring &detail)
	: Screen(minecraft), errorMessage(message), errorDetail(detail)
{
}

void GuiConnectFailed::tick()
{
	// No updates needed
}

void GuiConnectFailed::keyPressed(char_t eventCharacter, int_t eventKey)
{
	// No key handling
}

void GuiConnectFailed::init()
{
	buttons.clear();
	buttons.push_back(std::make_shared<Button>(0, width / 2 - 100, height / 4 + 120 + 12, u"Back to title screen"));
}

void GuiConnectFailed::buttonClicked(Button &button)
{
	if (button.id == 0)
	{
		minecraft.setScreen(std::make_shared<TitleScreen>(minecraft));
	}
}

void GuiConnectFailed::render(int_t xm, int_t ym, float a)
{
	renderBackground();
	drawCenteredString(font, errorMessage, width / 2, height / 2 - 50, 16777215);
	drawCenteredString(font, errorDetail, width / 2, height / 2 - 10, 16777215);
	Screen::render(xm, ym, a);
}
