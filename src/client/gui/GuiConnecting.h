#pragma once

#include "client/gui/Screen.h"

#include "java/Type.h"
#include "java/String.h"

class NetClientHandler;

class GuiConnecting : public Screen
{
private:
	NetClientHandler *clientHandler;
	bool cancelled;
	jstring hostName;

public:
	GuiConnecting(Minecraft &minecraft, const jstring &hostName, int_t port);

	void tick() override;

protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;

public:
	void init() override;

protected:
	void buttonClicked(Button &button) override;

public:
	void render(int_t xm, int_t ym, float a) override;

	// Static accessors for ThreadConnectToServer
	static void setNetClientHandler(GuiConnecting *gui, NetClientHandler *handler);
	static bool isCancelled(GuiConnecting *gui);
	static NetClientHandler *getNetClientHandler(GuiConnecting *gui);
};
