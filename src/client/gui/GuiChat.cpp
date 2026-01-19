#include "client/gui/GuiChat.h"
#include "client/Minecraft.h"
#include "client/gui/GuiNewChat.h"
#include "client/player/EntityClientPlayerMP.h"
#include "pc/lwjgl/Keyboard.h"
#include "lwjgl/GLContext.h"

#include <SDL3/SDL.h>

// Alpha 1.2.6: GuiChat constructor
// Java: public GuiChat() { }
GuiChat::GuiChat(Minecraft &minecraft) : Screen(minecraft)
{
}

// Alpha 1.2.6: GuiChat constructor with default text
// Java: public GuiChat(String var1) {
//     this.defaultInputFieldText = var1;
// }
GuiChat::GuiChat(Minecraft &minecraft, const jstring &defaultText) : Screen(minecraft), defaultInputText(defaultText)
{
}

// Alpha 1.2.6: initGui() - initializes the chat screen
// Java: public void initGui() {
//     Keyboard.enableRepeatEvents(true);
//     this.sentHistoryCursor = this.mc.ingameGUI.getChatGUI().func_73756_b().size();
//     this.inputField = new GuiTextField(this.fontRenderer, 4, this.height - 12, this.width - 4, 12, true);
//     this.inputField.setMaxStringLength(100);
//     this.inputField.setEnableBackgroundDrawing(false);
//     this.inputField.setFocused(true);
//     this.inputField.setText(this.defaultInputFieldText);
//     this.inputField.setCanLoseFocus(false);
// }
void GuiChat::init()
{
	lwjgl::Keyboard::enableRepeatEvents(true);
	
	GuiNewChat &chatGUI = minecraft.gui.getChatGUI();
	sentHistoryCursor = static_cast<int_t>(chatGUI.getSentHistory().size());
	
	inputField = std::make_unique<GuiTextField>(font, 4, height - 12, width - 4, 12, true);
	inputField->setMaxStringLength(100);
	inputField->setEnableBackgroundDrawing(false);
	inputField->setFocused(true);
	inputField->setText(defaultInputText);
	inputField->setCanLoseFocus(false);
}

// Alpha 1.2.6: onGuiClosed() - cleanup when chat screen closes
// Java: public void onGuiClosed() {
//     Keyboard.enableRepeatEvents(false);
//     this.mc.ingameGUI.getChatGUI().func_73764_c();
// }
void GuiChat::removed()
{
	lwjgl::Keyboard::enableRepeatEvents(false);
	minecraft.gui.getChatGUI().resetScroll();
	
	// SDL3: Stop text input when chat closes
	SDL_Window *window = lwjgl::GLContext::detail::getWindow();
	if (window)
		SDL_StopTextInput(window);
}

// Alpha 1.2.6: updateScreen() - updates cursor counter
// Java: public void updateScreen() {
//     super.updateScreen();
//     this.inputField.updateCursorCounter();
// }
void GuiChat::tick()
{
	Screen::tick();
	if (inputField)
		inputField->updateCursorCounter();
}

// Alpha 1.2.6: keyTyped() - handles keyboard input
// Java: protected void keyTyped(char var1, int var2)
void GuiChat::keyPressed(char_t eventCharacter, int_t eventKey)
{
	if (eventKey == lwjgl::Keyboard::KEY_ESCAPE)
	{
		minecraft.setScreen(nullptr);
	}
	else if (eventKey == lwjgl::Keyboard::KEY_RETURN)
	{
		jstring message = inputField->getText();
		// Trim whitespace from start and end
		while (!message.empty() && (message.front() == u' ' || message.front() == u'\t' || message.front() == u'\n' || message.front() == u'\r'))
			message.erase(0, 1);
		while (!message.empty() && (message.back() == u' ' || message.back() == u'\t' || message.back() == u'\n' || message.back() == u'\r'))
			message.pop_back();
		if (!message.empty())
		{
			GuiNewChat &chatGUI = minecraft.gui.getChatGUI();
			chatGUI.addToSentHistory(message);
			
			if (minecraft.player)
			{
				EntityClientPlayerMP *clientPlayer = dynamic_cast<EntityClientPlayerMP*>(minecraft.player.get());
				if (clientPlayer)
				{
					clientPlayer->sendChatMessage(message);
				}
			}
		}
		
		minecraft.setScreen(nullptr);
	}
	else if (eventKey == lwjgl::Keyboard::KEY_UP)
	{
		getSentHistory(-1);
	}
	else if (eventKey == lwjgl::Keyboard::KEY_DOWN)
	{
		getSentHistory(1);
	}
	else if (eventKey == lwjgl::Keyboard::KEY_PRIOR)  // Page Up
	{
		minecraft.gui.getChatGUI().scroll(19);
	}
	else if (eventKey == lwjgl::Keyboard::KEY_NEXT)  // Page Down
	{
		minecraft.gui.getChatGUI().scroll(-19);
	}
	else
	{
		if (inputField)
			inputField->textboxKeyTyped(eventCharacter, eventKey);
	}
}

// Alpha 1.2.6: mouseClicked() - handles mouse clicks
// Java: protected void mouseClicked(int var1, int var2, int var3)
void GuiChat::mouseClicked(int_t x, int_t y, int_t buttonNum)
{
	if (inputField && buttonNum == 0)
	{
		inputField->mouseClicked(x, y, buttonNum);
	}
	Screen::mouseClicked(x, y, buttonNum);
}

// Alpha 1.2.6: drawScreen() - renders the chat input field
// Java: public void drawScreen(int var1, int var2, float var3) {
//     drawRect(2, this.height - 14, this.width - 2, this.height - 2, Integer.MIN_VALUE);
//     this.inputField.drawTextBox();
//     super.drawScreen(var1, var2, var3);
// }
void GuiChat::render(int_t xm, int_t ym, float a)
{
	// Draw background
	fill(2, height - 14, width - 2, height - 2, 0x80000000);
	
	// Draw input field
	if (inputField)
		inputField->drawTextBox();
	
	Screen::render(xm, ym, a);
}

// Alpha 1.2.6: doesGuiPauseGame() - chat doesn't pause the game
// Java: public boolean doesGuiPauseGame() {
//     return false;
// }
bool GuiChat::isPauseScreen()
{
	return false;
}

// Alpha 1.2.6: getSentHistory() - navigates sent message history
// Java: private void getSentHistory(int var1)
void GuiChat::getSentHistory(int_t direction)
{
	GuiNewChat &chatGUI = minecraft.gui.getChatGUI();
	std::vector<jstring> &history = chatGUI.getSentHistory();
	
	int_t newCursor = sentHistoryCursor + direction;
	if (newCursor < 0)
		newCursor = 0;
	if (newCursor > static_cast<int_t>(history.size()))
		newCursor = static_cast<int_t>(history.size());
	
	if (newCursor != sentHistoryCursor)
	{
		if (newCursor == static_cast<int_t>(history.size()))
		{
			sentHistoryCursor = newCursor;
			if (inputField)
				inputField->setText(defaultInputText);
		}
		else
		{
			if (sentHistoryCursor == static_cast<int_t>(history.size()) && inputField)
			{
				defaultInputText = inputField->getText();
			}
			
			if (newCursor < static_cast<int_t>(history.size()))
			{
				if (inputField)
					inputField->setText(history[newCursor]);
				sentHistoryCursor = newCursor;
			}
		}
	}
}
