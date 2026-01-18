#pragma once

#include "client/gui/Screen.h"
#include "client/gui/GuiTextField.h"

#include "java/Type.h"
#include "java/String.h"

// Alpha 1.2.6: GuiChat - screen for typing chat messages
// Java: net.minecraft.src.GuiChat
class GuiChat : public Screen
{
private:
	jstring defaultInputText = u"";
	int_t sentHistoryCursor = -1;
	std::unique_ptr<GuiTextField> inputField;

public:
	GuiChat(Minecraft &minecraft);
	GuiChat(Minecraft &minecraft, const jstring &defaultText);
	
	virtual void init() override;
	virtual void removed() override;
	virtual void tick() override;
	
protected:
	virtual void keyPressed(char_t eventCharacter, int_t eventKey) override;
	virtual void mouseClicked(int_t x, int_t y, int_t buttonNum) override;
	
public:
	virtual void render(int_t xm, int_t ym, float a) override;
	virtual bool isPauseScreen() override;
	
private:
	void getSentHistory(int_t direction);
};
