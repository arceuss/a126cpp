#pragma once

#include "client/gui/Screen.h"

class Options;

class ControllerSettingsScreen : public Screen
{
private:
	std::shared_ptr<Screen> lastScreen;
	Options &options;

protected:
	jstring title = u"Controller Settings";

public:
	ControllerSettingsScreen(Minecraft &minecraft, std::shared_ptr<Screen> lastScreen, Options &options);

	void init() override;
	void tick() override;

protected:
	void buttonClicked(Button &button) override;
	void keyPressed(char_t eventCharacter, int_t eventKey) override;

public:
	void render(int_t xm, int_t ym, float a) override;
};
