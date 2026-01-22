#pragma once

#include "client/gui/Screen.h"
#include "client/gui/GuiTextField.h"

class Options;

class ClientOptionsScreen : public Screen
{
private:
	std::shared_ptr<Screen> lastScreen;
	Options &options;
	std::unique_ptr<GuiTextField> usernameField;

protected:
	jstring title = u"Client Options";

public:
	ClientOptionsScreen(Minecraft &minecraft, std::shared_ptr<Screen> lastScreen, Options &options);

	void init() override;
	void removed() override;

protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;
	void mouseClicked(int_t x, int_t y, int_t buttonNum) override;
	void buttonClicked(Button &button) override;
	void tick() override;

public:
	void render(int_t xm, int_t ym, float a) override;
};
