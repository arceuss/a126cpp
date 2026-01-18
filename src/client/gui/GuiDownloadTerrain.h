#pragma once

#include "client/gui/Screen.h"

#include "java/Type.h"
#include "java/String.h"

class NetClientHandler;

class GuiDownloadTerrain : public Screen
{
private:
	NetClientHandler* netHandler;
	int_t updateCounter;

public:
	GuiDownloadTerrain(Minecraft &minecraft, NetClientHandler* handler);

	void tick() override;

protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;

public:
	void init() override;

protected:
	void buttonClicked(Button &button) override;

public:
	void render(int_t xm, int_t ym, float a) override;
};
