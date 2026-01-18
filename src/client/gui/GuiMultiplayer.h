#pragma once

#include "client/gui/Screen.h"

#include "java/Type.h"
#include "java/String.h"

class Screen;
class GuiTextField;

class GuiMultiplayer : public Screen
{
private:
	Screen *parentScreen;
	int_t frameCounter;
	std::unique_ptr<GuiTextField> serverTextField;

public:
	GuiMultiplayer(Minecraft &minecraft, Screen *parent);

	void tick() override;
	void init() override;
	void removed() override;

protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;
	void buttonClicked(Button &button) override;

public:
	void render(int_t xm, int_t ym, float a) override;

private:
	int_t parseInt(const jstring &str, int_t defaultValue);
};
