#include "client/gui/GuiTextField.h"
#include "client/gui/Screen.h"
#include "client/renderer/Tesselator.h"
#include "pc/OpenGL.h"
#include "lwjgl/Keyboard.h"
#include "SharedConstants.h"

#include <algorithm>
#include <cmath>

GuiTextField::GuiTextField(Font &fontRenderer, int_t x, int_t y, int_t w, int_t h)
	: fontRenderer(fontRenderer), xPos(x), yPos(y), width(w), height(h),
	  text(u""), maxStringLength(32), cursorCounter(0),
	  enableBackgroundDrawing(true), canLoseFocus(true), isFocused(false),
	  isEnabled(true), lineScrollOffset(0), cursorPosition(0), selectionEnd(0),
	  enabledColor(14737632), disabledColor(7368816), isVisible(true), isChat(false)
{
}

GuiTextField::GuiTextField(Font &fontRenderer, int_t x, int_t y, int_t w, int_t h, bool isChat)
	: GuiTextField(fontRenderer, x, y, w, h)
{
	this->isChat = isChat;
}

void GuiTextField::updateCursorCounter()
{
	++cursorCounter;
}

void GuiTextField::setText(const jstring &str)
{
	if (str.length() > maxStringLength)
		text = str.substr(0, maxStringLength);
	else
		text = str;
	setCursorPositionEnd();
}

jstring GuiTextField::getText() const
{
	return text;
}

jstring GuiTextField::getSelectedText() const
{
	int_t start = cursorPosition < selectionEnd ? cursorPosition : selectionEnd;
	int_t end = cursorPosition < selectionEnd ? selectionEnd : cursorPosition;
	return text.substr(start, end - start);
}

void GuiTextField::writeText(const jstring &str)
{
	if (str.empty())
		return;

	jstring filtered;
	for (char_t c : str)
	{
		if (isAllowedCharacter(c, isChat))
			filtered.push_back(c);
	}

	int_t start = cursorPosition < selectionEnd ? cursorPosition : selectionEnd;
	int_t end = cursorPosition < selectionEnd ? selectionEnd : cursorPosition;
	int_t available = maxStringLength - text.length() + (end - start);

	if (available < filtered.length())
		filtered = filtered.substr(0, available);

	jstring newText;
	if (start > 0)
		newText = text.substr(0, start);
	newText += filtered;
	if (end < text.length())
		newText += text.substr(end);

	text = newText;
	moveCursorBy(start - selectionEnd + filtered.length());
}

void GuiTextField::deleteWords(int_t num)
{
	if (text.empty())
		return;

	if (selectionEnd != cursorPosition)
		writeText(u"");
	else
		deleteFromCursor(getNthWordFromCursor(num) - cursorPosition);
}

void GuiTextField::deleteFromCursor(int_t num)
{
	if (text.empty())
		return;

	if (selectionEnd != cursorPosition)
		writeText(u"");
	else
	{
		bool backward = num < 0;
		int_t start = backward ? cursorPosition + num : cursorPosition;
		int_t end = backward ? cursorPosition : cursorPosition + num;

		if (start < 0)
			start = 0;
		if (end > text.length())
			end = text.length();

		text = text.substr(0, start) + text.substr(end);
		if (backward)
			moveCursorBy(num);
	}
}

int_t GuiTextField::getNthWordFromCursor(int_t num)
{
	return getNthWordFromPos(num, cursorPosition);
}

int_t GuiTextField::getNthWordFromPos(int_t num, int_t pos)
{
	int_t result = pos;
	bool backward = num < 0;
	int_t count = std::abs(num);

	for (int_t i = 0; i < count; ++i)
	{
		if (!backward)
		{
			int_t len = text.length();
			size_t found = text.find(' ', result);
			if (found == jstring::npos)
				result = len;
			else
			{
				result = found;
				while (result < len && text[result] == ' ')
					++result;
			}
		}
		else
		{
			while (result > 0 && text[result - 1] == ' ')
				--result;
			while (result > 0 && text[result - 1] != ' ')
				--result;
		}
	}

	return result;
}

void GuiTextField::moveCursorBy(int_t num)
{
	setCursorPosition(selectionEnd + num);
}

void GuiTextField::setCursorPosition(int_t pos)
{
	cursorPosition = pos;
	int_t len = text.length();
	if (cursorPosition < 0)
		cursorPosition = 0;
	if (cursorPosition > len)
		cursorPosition = len;
	setSelectionPos(cursorPosition);
}

void GuiTextField::setCursorPositionZero()
{
	setCursorPosition(0);
}

void GuiTextField::setCursorPositionEnd()
{
	setCursorPosition(text.length());
}

bool GuiTextField::textboxKeyTyped(char_t ch, int_t key)
{
	if (!isEnabled || !isFocused)
		return false;

	switch (ch)
	{
	case 1: // Ctrl+A
		setCursorPositionEnd();
		setSelectionPos(0);
		return true;
	case 3: // Ctrl+C
		// TODO: Clipboard support
		return true;
	case 8: // Backspace character (\b)
		if (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LCONTROL) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RCONTROL))
			deleteWords(-1);
		else
			deleteFromCursor(-1);
		return true;
	case 22: // Ctrl+V
		// TODO: Clipboard support
		return true;
	case 24: // Ctrl+X
		// TODO: Clipboard support
		writeText(u"");
		return true;
	default:
		switch (key)
		{
		case 14: // Backspace
			if (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LCONTROL) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RCONTROL))
				deleteWords(-1);
			else
				deleteFromCursor(-1);
			return true;
		case 199: // Home
			if (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LSHIFT) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RSHIFT))
				setSelectionPos(0);
			else
				setCursorPositionZero();
			return true;
		case 203: // Left
		{
			bool shift = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LSHIFT) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RSHIFT);
			bool ctrl = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LCONTROL) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RCONTROL);
			if (shift)
			{
				if (ctrl)
					setSelectionPos(getNthWordFromPos(-1, selectionEnd));
				else
					setSelectionPos(selectionEnd - 1);
			}
			else if (ctrl)
				setCursorPosition(getNthWordFromCursor(-1));
			else
				moveCursorBy(-1);
			return true;
		}
		case 205: // Right
		{
			bool shift = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LSHIFT) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RSHIFT);
			bool ctrl = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LCONTROL) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RCONTROL);
			if (shift)
			{
				if (ctrl)
					setSelectionPos(getNthWordFromPos(1, selectionEnd));
				else
					setSelectionPos(selectionEnd + 1);
			}
			else if (ctrl)
				setCursorPosition(getNthWordFromCursor(1));
			else
				moveCursorBy(1);
			return true;
		}
		case 207: // End
			if (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LSHIFT) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RSHIFT))
				setSelectionPos(text.length());
			else
				setCursorPositionEnd();
			return true;
		case 211: // Delete
			if (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LCONTROL) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RCONTROL))
				deleteWords(1);
			else
				deleteFromCursor(1);
			return true;
		default:
			if (isAllowedCharacter(ch, isChat))
			{
				writeText(jstring(1, ch));
				return true;
			}
			return false;
		}
	}
}

void GuiTextField::mouseClicked(int_t x, int_t y, int_t button)
{
	bool inBounds = x >= xPos && x < xPos + width && y >= yPos && y < yPos + height;
	if (canLoseFocus)
		setFocused(isEnabled && inBounds);

	if (isFocused && button == 0)
	{
		int_t relX = x - xPos;
		if (enableBackgroundDrawing)
			relX -= 4;

		jstring visibleText = fontRenderer.trimStringToWidth(text.substr(lineScrollOffset), getWidth());
		setCursorPosition(fontRenderer.trimStringToWidth(visibleText, relX).length() + lineScrollOffset);
	}
}

void GuiTextField::drawTextBox()
{
	if (!isVisible)
		return;

	if (enableBackgroundDrawing)
	{
		fill(xPos - 1, yPos - 1, xPos + width + 1, yPos + height + 1, -6250336);
		fill(xPos, yPos, xPos + width, yPos + height, -16777216);
	}

	int_t color = isEnabled ? enabledColor : disabledColor;
	int_t cursorRel = cursorPosition - lineScrollOffset;
	int_t selectionRel = selectionEnd - lineScrollOffset;
	jstring visibleText = fontRenderer.trimStringToWidth(text.substr(lineScrollOffset), getWidth());
	bool cursorVisible = cursorRel >= 0 && cursorRel <= visibleText.length();
	bool showCursor = isFocused && (cursorCounter / 6) % 2 == 0 && cursorVisible;

	int_t textX = enableBackgroundDrawing ? xPos + 4 : xPos;
	int_t textY = enableBackgroundDrawing ? yPos + (height - 8) / 2 : yPos;
	int_t drawX = textX;

	if (selectionRel > visibleText.length())
		selectionRel = visibleText.length();

	if (!visibleText.empty())
	{
		jstring beforeCursor = cursorVisible ? visibleText.substr(0, cursorRel) : visibleText;
		drawX = textX + fontRenderer.width(beforeCursor);
		fontRenderer.drawShadow(beforeCursor, textX, textY, color);
	}

	bool atEnd = cursorPosition >= text.length() || text.length() >= maxStringLength;
	int_t cursorX = drawX;
	if (!cursorVisible)
		cursorX = cursorRel > 0 ? textX + width : textX;
	else if (atEnd)
	{
		cursorX = drawX - 1;
		--drawX;
	}

	if (!visibleText.empty() && cursorVisible && cursorRel < visibleText.length())
	{
		jstring afterCursor = visibleText.substr(cursorRel);
		fontRenderer.drawShadow(afterCursor, drawX, textY, color);
	}

	if (showCursor)
	{
		if (atEnd)
		{
			fill(cursorX, textY - 1, cursorX + 1, textY + 9, -3092272);
		}
		else
		{
			fontRenderer.drawShadow(u"_", cursorX, textY, color);
		}
	}

	if (selectionRel != cursorRel)
	{
		int_t selX = textX + fontRenderer.width(visibleText.substr(0, selectionRel));
		drawCursorVertical(cursorX, textY - 1, selX - 1, textY + 9);
	}
}

void GuiTextField::drawCursorVertical(int_t x1, int_t y1, int_t x2, int_t y2)
{
	if (x1 > x2)
		std::swap(x1, x2);
	if (y1 > y2)
		std::swap(y1, y2);

	Tesselator &t = Tesselator::instance;
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_OR_REVERSE);
	t.begin();
	t.vertex(x1, y2, 0.0);
	t.vertex(x2, y2, 0.0);
	t.vertex(x2, y1, 0.0);
	t.vertex(x1, y1, 0.0);
	t.end();
	glDisable(GL_COLOR_LOGIC_OP);
	glEnable(GL_TEXTURE_2D);
}

void GuiTextField::setMaxStringLength(int_t len)
{
	maxStringLength = len;
	if (text.length() > len)
		text = text.substr(0, len);
}

int_t GuiTextField::getMaxStringLength() const
{
	return maxStringLength;
}

int_t GuiTextField::getCursorPosition() const
{
	return cursorPosition;
}

bool GuiTextField::getEnableBackgroundDrawing() const
{
	return enableBackgroundDrawing;
}

void GuiTextField::setEnableBackgroundDrawing(bool enable)
{
	enableBackgroundDrawing = enable;
}

void GuiTextField::setTextColor(int_t color)
{
	enabledColor = color;
}

void GuiTextField::setFocused(bool focused)
{
	if (focused && !isFocused)
		cursorCounter = 0;
	isFocused = focused;
}

bool GuiTextField::getIsFocused() const
{
	return isFocused;
}

int_t GuiTextField::getSelectionEnd() const
{
	return selectionEnd;
}

int_t GuiTextField::getWidth() const
{
	return enableBackgroundDrawing ? width - 8 : width;
}

void GuiTextField::setCanLoseFocus(bool canLose)
{
	canLoseFocus = canLose;
}

bool GuiTextField::getIsVisible() const
{
	return isVisible;
}

void GuiTextField::setVisible(bool visible)
{
	isVisible = visible;
}

void GuiTextField::setSelectionPos(int_t pos)
{
	int_t len = text.length();
	if (pos > len)
		pos = len;
	if (pos < 0)
		pos = 0;

	selectionEnd = pos;

	if (lineScrollOffset > len)
		lineScrollOffset = len;

	int_t visibleWidth = getWidth();
	jstring visibleText = fontRenderer.trimStringToWidth(text.substr(lineScrollOffset), visibleWidth);
	int_t visibleEnd = visibleText.length() + lineScrollOffset;

	if (pos == lineScrollOffset)
	{
		jstring trimmed = fontRenderer.trimStringToWidth(text, visibleWidth, true);
		lineScrollOffset -= trimmed.length();
	}

	if (pos > visibleEnd)
		lineScrollOffset += pos - visibleEnd;
	else if (pos <= lineScrollOffset)
		lineScrollOffset -= lineScrollOffset - pos;

	if (lineScrollOffset < 0)
		lineScrollOffset = 0;
	if (lineScrollOffset > len)
		lineScrollOffset = len;
}

bool GuiTextField::isAllowedCharacter(char_t ch, bool isChat)
{
	return ch != 167 && (SharedConstants::acceptableLetters.find(ch) != jstring::npos || (!isChat && ch > 32));
}
