#pragma once

#include "client/gui/GuiComponent.h"
#include "client/gui/Font.h"

#include "java/Type.h"
#include "java/String.h"

class Font;

class GuiTextField : public GuiComponent
{
private:
	Font &fontRenderer;
	const int_t xPos;
	const int_t yPos;
	const int_t width;
	const int_t height;
	jstring text;
	int_t maxStringLength;
	int_t cursorCounter;
	bool enableBackgroundDrawing;
	bool canLoseFocus;
	bool isFocused;
	bool isEnabled;
	int_t lineScrollOffset;
	int_t cursorPosition;
	int_t selectionEnd;
	int_t enabledColor;
	int_t disabledColor;
	bool isVisible;
	bool isChat;

public:
	GuiTextField(Font &fontRenderer, int_t x, int_t y, int_t w, int_t h);
	GuiTextField(Font &fontRenderer, int_t x, int_t y, int_t w, int_t h, bool isChat);

	void updateCursorCounter();
	void setText(const jstring &str);
	jstring getText() const;
	jstring getSelectedText() const;
	void writeText(const jstring &str);
	void deleteWords(int_t num);
	void deleteFromCursor(int_t num);
	int_t getNthWordFromCursor(int_t num);
	int_t getNthWordFromPos(int_t num, int_t pos);
	void setCursorPosition(int_t pos);
	void setCursorPositionZero();
	void setCursorPositionEnd();
	bool textboxKeyTyped(char_t ch, int_t key);
	void mouseClicked(int_t x, int_t y, int_t button);
	void drawTextBox();
	void setMaxStringLength(int_t len);
	int_t getMaxStringLength() const;
	int_t getCursorPosition() const;
	bool getEnableBackgroundDrawing() const;
	void setEnableBackgroundDrawing(bool enable);
	void setTextColor(int_t color);
	void setFocused(bool focused);
	bool getIsFocused() const;
	int_t getSelectionEnd() const;
	int_t getWidth() const;
	void setCanLoseFocus(bool canLose);
	bool getIsVisible() const;
	void setVisible(bool visible);

private:
	void moveCursorBy(int_t num);
	void setSelectionPos(int_t pos);
	void drawCursorVertical(int_t x1, int_t y1, int_t x2, int_t y2);
	static bool isAllowedCharacter(char_t ch, bool isChat);
};
