#include "client/gui/GuiNewChat.h"
#include "client/Minecraft.h"
#include "client/gui/Font.h"
#include "client/gui/GuiChat.h"
#include "client/gui/Gui.h"
#include "client/gui/ChatLine.h"

#include "pc/OpenGL.h"

// Alpha 1.2.6: GuiNewChat constructor
// Java: public GuiNewChat(Minecraft var1) {
//     this.mc = var1;
// }
GuiNewChat::GuiNewChat(Minecraft &mc) : mc(mc)
{
}

// Alpha 1.2.6: Helper function to extract last color code from string
// Java: private static String getFormatFromString(String var0)
static jstring getFormatFromString(const jstring &str)
{
	jstring result = u"";
	int_t lastPos = -1;
	int_t len = str.length();
	
	// Alpha 1.2.6: while(true) {
	//         var2 = var0.indexOf(167, var2 + 1);
	//         if(var2 == -1) { return var1; }
	//         if(var2 < var3 - 1) {
	//             char var4 = var0.charAt(var2 + 1);
	//             if(isFormatColor(var4)) {
	//                 var1 = "\u00a7" + var4;
	//             }
	//         }
	//       }
	while (true)
	{
		// Find next color code (167 = 0xA7 = §)
		int_t pos = str.find(167, lastPos + 1);
		if (pos == jstring::npos)
		{
			return result;
		}
		
		if (pos < len - 1)
		{
			char_t codeChar = str[pos + 1];
			// Alpha 1.2.6: isFormatColor - checks if char is 0-9, a-f, A-F
			// Java: return var0 >= 48 && var0 <= 57 || var0 >= 97 && var0 <= 102 || var0 >= 65 && var0 <= 70;
			if ((codeChar >= u'0' && codeChar <= u'9') ||
			    (codeChar >= u'a' && codeChar <= u'f') ||
			    (codeChar >= u'A' && codeChar <= u'F'))
			{
				// Alpha 1.2.6: var1 = "\u00a7" + var4;
				result = jstring(1, 167) + jstring(1, codeChar);
			}
		}
		
		lastPos = pos;
	}
}

// Alpha 1.2.6: Helper function to find split position (like sizeStringToWidth)
// Java: private int sizeStringToWidth(String var1, int var2)
static int_t sizeStringToWidth(Font &font, const jstring &text, int_t maxWidth)
{
	int_t len = text.length();
	int_t width = 0;
	int_t lastSpace = -1;
	bool bold = false;  // Alpha 1.2.6: tracks bold state (var7)
	int_t i = 0;  // Declare outside loop so it's accessible after
	
	for (i = 0; i < len; i++)
	{
		char_t ch = text[i];
		
		// Alpha 1.2.6: switch(var8) {
		//         case '\u00a7': handle color code...
		//         if(var8 == 10): handle newline...
		//         case ' ': var6 = var5;  (falls through to default)
		//         default: var4 += this.getCharWidth(var8);
		if (ch == 167)  // Color code (case '\u00a7')
		{
			// Alpha 1.2.6: if(var5 < var3 - 1) {
			//         ++var5;
			//         char var9 = var1.charAt(var5);
			//         if(var9 != 108 && var9 != 76) {  // 'l' or 'L' (bold)
			//             if(var9 == 114 || var9 == 82) { var7 = false; }  // 'r' or 'R' (reset)
			//         } else {
			//             var7 = true;  // bold
			//         }
			//       }
			if (i < len - 1)
			{
				i++;
				char_t codeChar = text[i];
				if (codeChar == u'l' || codeChar == u'L')
				{
					bold = true;
				}
				else if (codeChar == u'r' || codeChar == u'R')
				{
					bold = false;
				}
			}
			// Color codes don't add width, continue to next character
			continue;
		}
		
		// Alpha 1.2.6: if(var8 == 10) {  // newline
		//         ++var5;
		//         var6 = var5;
		//         break;
		//       }
		if (ch == u'\n')
		{
			i++;
			lastSpace = i;
			break;
		}
		
		// Alpha 1.2.6: case ' ': var6 = var5; (falls through)
		//         default: var4 += this.getCharWidth(var8);
		//         if(var7) { ++var4; }  // Bold adds 1 pixel
		if (ch == u' ')
		{
			lastSpace = i;
		}
		
		// Default case: add character width
		int_t charWidth = font.width(jstring(1, ch));
		width += charWidth;
		if (bold)
		{
			width++;  // Bold adds 1 pixel
		}
		
		// Alpha 1.2.6: if(var4 > var2) { break; }
		if (width > maxWidth)
		{
			break;
		}
	}
	
	// Alpha 1.2.6: return var5 != var3 && var6 != -1 && var6 < var5 ? var6 : var5;
	// Prefer breaking at space if possible
	// Note: i is the position after the last character processed
	int_t finalPos = i;
	if (finalPos != len && lastSpace != -1 && lastSpace < finalPos)
	{
		return lastSpace;
	}
	return finalPos;
}

// Alpha 1.2.6: Helper function to wrap string with format preservation (like wrapFormattedStringToWidth)
// Java: String wrapFormattedStringToWidth(String var1, int var2)
static jstring wrapFormattedStringToWidth(Font &font, const jstring &text, int_t maxWidth)
{
	// Alpha 1.2.6: int var3 = this.sizeStringToWidth(var1, var2);
	//         if(var1.length() <= var3) { return var1; }
	int_t splitPos = sizeStringToWidth(font, text, maxWidth);
	if (text.length() <= splitPos)
	{
		return text;
	}
	
	// Alpha 1.2.6: String var4 = var1.substring(0, var3);
	//         String var5 = getFormatFromString(var4) + var1.substring(var3 + (var1.charAt(var3) == 32 ? 1 : 0));
	//         return var4 + "\n" + this.wrapFormattedStringToWidth(var5, var2);
	jstring firstPart = text.substr(0, splitPos);
	jstring format = getFormatFromString(firstPart);
	
	// Skip space if present at split position
	int_t remainderStart = splitPos;
	if (splitPos < text.length() && text[splitPos] == u' ')
	{
		remainderStart++;
	}
	
	jstring remainder = text.substr(remainderStart);
	jstring formattedRemainder = format + remainder;
	
	return firstPart + u"\n" + wrapFormattedStringToWidth(font, formattedRemainder, maxWidth);
}

// Alpha 1.2.6: Helper function to split string to width (like Java's listFormattedStringToWidth)
// Java: public List listFormattedStringToWidth(String var1, int var2) {
//         return Arrays.asList(this.wrapFormattedStringToWidth(var1, var2).split("\n"));
//       }
static std::vector<jstring> splitStringToWidth(Font &font, const jstring &text, int_t maxWidth)
{
	// Alpha 1.2.6: Split by newlines from wrapped string
	jstring wrapped = wrapFormattedStringToWidth(font, text, maxWidth);
	std::vector<jstring> lines;
	
	jstring currentLine;
	for (int_t i = 0; i < wrapped.length(); i++)
	{
		char_t ch = wrapped[i];
		if (ch == u'\n')
		{
			if (!currentLine.empty())
			{
				lines.push_back(currentLine);
				currentLine.clear();
			}
		}
		else
		{
			currentLine.push_back(ch);
		}
	}
	
	if (!currentLine.empty())
	{
		lines.push_back(currentLine);
	}
	
	return lines;
}

// Alpha 1.2.6: func_73762_a() - renders chat overlay
// Java: public void func_73762_a(int var1)
void GuiNewChat::render(int_t updateCounter)
{
	// Alpha 1.2.6: Java lines 20-91
	byte maxVisible = 10;
	bool chatOpen = false;
	int_t visibleCount = 0;
	int_t totalLines = chatLines.size();
	float var6 = 1.0F;
	
	if (totalLines > 0)
	{
		if (isChatScreenOpen())
		{
			maxVisible = 20;
			chatOpen = true;
		}
		
		// Alpha 1.2.6: GL11.glPushMatrix();
		//         GL11.glTranslatef(2.0F, 8.0F, 0.0F);
		// Note: Translation is done in Gui::render, but Alpha 1.2.6 does it here
		// We'll do the additional translation here to match
		glPushMatrix();
		glTranslatef(2.0f, 8.0f, 0.0f);
		
		// Alpha 1.2.6: for(int var7 = 0; var7 + this.field_73768_d < this.ChatLines.size() && var7 < var2; ++var7) {
		for (int_t i = 0; i + scrollOffset < chatLines.size() && i < maxVisible; i++)
		{
			auto &line = chatLines[i + scrollOffset];
			if (line == nullptr)
				continue;
			
			// Alpha 1.2.6: var8 = var1 - var10.getUpdatedCounter();
			//         if(var8 < 200 || var3) {
			int_t age = updateCounter - line->getUpdatedCounter();
			if (age < 200 || chatOpen)
			{
				// Alpha 1.2.6: var11 = (double)var8 / 200.0D;
				//         var11 = 1.0D - var11;
				//         var11 *= 10.0D;
				//         if(var11 < 0.0D) { var11 = 0.0D; }
				//         if(var11 > 1.0D) { var11 = 1.0D; }
				//         var11 *= var11;
				//         var9 = (int)(255.0D * var11);
				double fade = static_cast<double>(age) / 200.0;
				fade = 1.0 - fade;
				fade *= 10.0;
				if (fade < 0.0)
				{
					fade = 0.0;
				}
				if (fade > 1.0)
				{
					fade = 1.0;
				}
				fade *= fade;
				int_t alpha = static_cast<int_t>(255.0 * fade);
				if (chatOpen)
				{
					alpha = 255;
				}
				
				// Alpha 1.2.6: var9 = (int)((float)var9 * var6);
				alpha = static_cast<int_t>(static_cast<float>(alpha) * var6);
				visibleCount++;
				
				// Alpha 1.2.6: if(var9 > 3) {
				if (alpha > 3)
				{
				// Alpha 1.2.6: int var14 = -var7 * 9;
				//         drawRect(-2, var14 - 9, 324, var14, var9 / 2 << 24);
				//         GL11.glEnable(GL11.GL_BLEND);
				//         String var15 = var10.getChatLineString();
				//         this.mc.fontRenderer.drawStringWithShadow(var15, 0, var14 - 8, 16777215 + (var9 << 24));
				// Note: GL_BLEND is already enabled in Gui::render before calling this
				int_t y = -i * 9;
				fill(-2, y - 9, 324, y, (alpha / 2) << 24);
				glEnable(GL_BLEND);  // Alpha 1.2.6 enables it again per line
				jstring lineText = line->getChatLineString();
				Font &font = *mc.font;
				font.drawShadow(lineText, 0, y - 8, 16777215 + (alpha << 24));
				}
			}
		}
		
		// Alpha 1.2.6: Scrollbar rendering (lines 73-87)
		if (chatOpen)
		{
			// Alpha 1.2.6: this.mc.fontRenderer.getClass();
			//         byte var16 = 9;
			//         GL11.glTranslatef(-3.0F, 0.0F, 0.0F);
			//         int var17 = var5 * var16 + var5;
			//         var8 = var4 * var16 + var4;
			//         int var19 = this.field_73768_d * var8 / var5;
			//         int var12 = var8 * var8 / var17;
			byte lineHeight = 9;
			glTranslatef(-3.0f, 0.0f, 0.0f);
			int_t totalHeight = totalLines * lineHeight + totalLines;
			int_t visibleHeight = visibleCount * lineHeight + visibleCount;
			int_t scrollbarPos = scrollOffset * visibleHeight / totalLines;
			int_t scrollbarHeight = visibleHeight * visibleHeight / totalHeight;
			
			// Alpha 1.2.6: if(var17 != var8) {
			//         var9 = var19 > 0 ? 170 : 96;
			//         int var18 = this.field_73769_e ? 13382451 : 3355562;
			//         drawRect(0, -var19, 2, -var19 - var12, var18 + (var9 << 24));
			//         drawRect(2, -var19, 1, -var19 - var12, 13421772 + (var9 << 24));
			//       }
			if (totalHeight != visibleHeight)
			{
				int_t scrollbarAlpha = scrollbarPos > 0 ? 170 : 96;
				int_t scrollbarColor = isScrolled ? 13382451 : 3355562;
				fill(0, -scrollbarPos, 2, -scrollbarPos - scrollbarHeight, scrollbarColor + (scrollbarAlpha << 24));
				fill(2, -scrollbarPos, 1, -scrollbarPos - scrollbarHeight, 13421772 + (scrollbarAlpha << 24));
			}
		}
		
		// Alpha 1.2.6: GL11.glPopMatrix();
		glPopMatrix();
	}
}

// Alpha 1.2.6: printChatMessage() - adds a chat message
// Java: public void printChatMessage(String var1)
void GuiNewChat::printChatMessage(const jstring &message)
{
	printChatMessageWithOptionalDeletion(message, 0);
}

// Alpha 1.2.6: printChatMessageWithOptionalDeletion() - adds a chat message with optional deletion ID
// Java: public void printChatMessageWithOptionalDeletion(String var1, int var2)
void GuiNewChat::printChatMessageWithOptionalDeletion(const jstring &message, int_t deletionID)
{
	// Alpha 1.2.6: Java lines 103-131
	bool chatOpen = isChatScreenOpen();
	bool isFirst = true;
	
	if (deletionID != 0)
	{
		deleteChatLine(deletionID);
	}
	
	Font &font = *mc.font;
	std::vector<jstring> lines = splitStringToWidth(font, message, 320);
	
	for (auto &line : lines)
	{
		// Alpha 1.2.6: if(var3 && this.field_73768_d > 0) {
		//         this.field_73769_e = true;
		//         this.func_73758_b(1);
		//       }
		if (chatOpen && scrollOffset > 0)
		{
			isScrolled = true;
			scroll(1);
		}
		
		// Alpha 1.2.6: if(!var4) {
		//         var6 = " " + var6;
		//       }
		//       var4 = false;
		jstring lineText = line;
		if (!isFirst)
		{
			lineText = u" " + line;
		}
		isFirst = false;
		
		// Alpha 1.2.6: this.ChatLines.add(0, new ChatLine(this.mc.ingameGUI.getUpdateCounter(), var6, var2));
		int_t updateCounter = mc.gui.tickCount;
		chatLines.insert(chatLines.begin(), std::make_shared<ChatLine>(updateCounter, lineText, deletionID));
	}
	
	// Alpha 1.2.6: while(this.ChatLines.size() > 100) {
	//         this.ChatLines.remove(this.ChatLines.size() - 1);
	//       }
	while (chatLines.size() > 100)
	{
		chatLines.pop_back();
	}
}

// Alpha 1.2.6: func_73756_b() - returns sent history
// Java: public List func_73756_b()
std::vector<jstring> &GuiNewChat::getSentHistory()
{
	return sentHistory;
}

// Alpha 1.2.6: func_73767_b() - adds to sent history
// Java: public void func_73767_b(String var1)
void GuiNewChat::addToSentHistory(const jstring &message)
{
	if (sentHistory.empty() || sentHistory.back() != message)
	{
		sentHistory.push_back(message);
	}
}

// Alpha 1.2.6: func_73764_c() - resets scroll
// Java: public void func_73764_c()
void GuiNewChat::resetScroll()
{
	scrollOffset = 0;
	isScrolled = false;
}

// Alpha 1.2.6: func_73758_b() - scrolls chat
// Java: public void func_73758_b(int var1)
void GuiNewChat::scroll(int_t amount)
{
	// Alpha 1.2.6: Java lines 149-161
	scrollOffset += amount;
	int_t totalLines = chatLines.size();
	if (scrollOffset > totalLines - 20)
	{
		scrollOffset = totalLines - 20;
	}
	
	if (scrollOffset <= 0)
	{
		scrollOffset = 0;
		isScrolled = false;
	}
}

// Alpha 1.2.6: func_73760_d() - checks if chat screen is open
// Java: public boolean func_73760_d()
bool GuiNewChat::isChatScreenOpen() const
{
	// Check if current screen is GuiChat
	return dynamic_cast<GuiChat*>(mc.screen.get()) != nullptr;
}

// Alpha 1.2.6: deleteChatLine() - deletes a chat line by ID
// Java: public void deleteChatLine(int var1)
void GuiNewChat::deleteChatLine(int_t id)
{
	deleteChatLineInternal(id);
}

// Alpha 1.2.6: getChatLines() - returns chat lines
// Java: public List getChatLines()
std::vector<std::shared_ptr<ChatLine>> &GuiNewChat::getChatLines()
{
	return chatLines;
}

// Alpha 1.2.6: deleteChatLineInternal() - internal helper to delete chat line
void GuiNewChat::deleteChatLineInternal(int_t id)
{
	for (auto it = chatLines.begin(); it != chatLines.end(); ++it)
	{
		if ((*it)->getChatLineID() == id)
		{
			chatLines.erase(it);
			return;
		}
	}
}
