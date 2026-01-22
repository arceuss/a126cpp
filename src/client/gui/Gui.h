#pragma once

#include "client/gui/GuiComponent.h"
#include "client/gui/GuiNewChat.h"
#include "client/gui/GuiMessage.h"

#include "java/Random.h"
#include <memory>
#include <vector>

class Minecraft;

class Gui : public GuiComponent
{
private:
	static constexpr int_t MAX_MESSAGE_WIDTH = 320;

	Random random;

public:
	int_t tickCount = 0;

private:
	jstring nowPlayingString = u"";
	int_t nowPlayingTime = 0;

	Minecraft &minecraft;
	
	// Beta 1.2/newb12: guiMessages - list of chat messages
	// Java: private List<GuiMessage> guiMessages = new ArrayList<>();
	// Made public so GuiNewChat can access it (in newb12 it's in the same class)
public:
	std::vector<GuiMessage> guiMessages;
private:
	
	// Alpha 1.2.6: GuiIngame has persistantChatGUI member
	// Java: private final GuiNewChat persistantChatGUI;
	std::unique_ptr<GuiNewChat> chatGUI;

	// Beta: renderSlot() - renders a single hotbar slot (Gui.java:322-341)
	void renderSlot(int_t slot, int_t x, int_t y, float a);

public:
	float progress = 0.0f;
	float tbr = 1.0f;

	Gui(Minecraft &minecraft);
	void render(float a, bool inScreen, int_t xm, int_t ym);

	void tick();
	
	// Beta 1.2/newb12: addMessage() - adds a chat message
	// Java: public void addMessage(String var1)
	void addMessage(const jstring &message);
	
	// Alpha 1.2.6: getChatGUI() - returns the chat GUI
	// Java: public GuiNewChat getChatGUI() {
	//     return this.persistantChatGUI;
	// }
	GuiNewChat &getChatGUI();
};
