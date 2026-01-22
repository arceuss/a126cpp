#pragma once

#include "client/gui/GuiComponent.h"
#include "client/gui/ChatLine.h"

#include <vector>
#include <memory>

#include "java/Type.h"
#include "java/String.h"

class Minecraft;
class Font;
class Gui;

// Alpha 1.2.6: GuiNewChat - displays chat messages in the ingame overlay
// Java: net.minecraft.src.GuiNewChat
class GuiNewChat : public GuiComponent
{
private:
	Minecraft &mc;
	std::vector<std::shared_ptr<ChatLine>> chatLines;
	std::vector<jstring> sentHistory;
	int_t scrollOffset = 0;
	bool isScrolled = false;

public:
	GuiNewChat(Minecraft &mc);
	
	// Alpha 1.2.6: func_73762_a() - renders chat overlay
	// Java: public void func_73762_a(int var1)
	void render(int_t updateCounter);
	
	// Alpha 1.2.6: printChatMessage() - adds a chat message
	// Java: public void printChatMessage(String var1)
	void printChatMessage(const jstring &message);
	
	// Alpha 1.2.6: printChatMessageWithOptionalDeletion() - adds a chat message with optional deletion ID
	// Java: public void printChatMessageWithOptionalDeletion(String var1, int var2)
	void printChatMessageWithOptionalDeletion(const jstring &message, int_t deletionID);
	
	// Alpha 1.2.6: func_73756_b() - returns sent history
	// Java: public List func_73756_b()
	std::vector<jstring> &getSentHistory();
	
	// Alpha 1.2.6: func_73767_b() - adds to sent history
	// Java: public void func_73767_b(String var1)
	void addToSentHistory(const jstring &message);
	
	// Alpha 1.2.6: func_73764_c() - resets scroll
	// Java: public void func_73764_c()
	void resetScroll();
	
	// Alpha 1.2.6: func_73758_b() - scrolls chat
	// Java: public void func_73758_b(int var1)
	void scroll(int_t amount);
	
	// Alpha 1.2.6: func_73760_d() - checks if chat screen is open
	// Java: public boolean func_73760_d()
	bool isChatScreenOpen() const;
	
	// Alpha 1.2.6: deleteChatLine() - deletes a chat line by ID
	// Java: public void deleteChatLine(int var1)
	void deleteChatLine(int_t id);
	
	// Alpha 1.2.6: getChatLines() - returns chat lines
	// Java: public List getChatLines()
	std::vector<std::shared_ptr<ChatLine>> &getChatLines();

private:
	void deleteChatLineInternal(int_t id);
};
