#include "client/gui/GuiMultiplayer.h"
#include "client/gui/GuiTextField.h"
#include "client/gui/GuiConnecting.h"
#include "client/Minecraft.h"
#include "lwjgl/Keyboard.h"

#include <algorithm>
#include <cctype>

GuiMultiplayer::GuiMultiplayer(Minecraft &minecraft, Screen *parent)
	: Screen(minecraft), parentScreen(parent), frameCounter(0)
{
}

void GuiMultiplayer::tick()
{
	++frameCounter;
	serverTextField->updateCursorCounter();
}

void GuiMultiplayer::init()
{
	lwjgl::Keyboard::enableRepeatEvents(true);
	buttons.clear();
	buttons.push_back(std::make_shared<Button>(0, width / 2 - 100, height / 4 + 96 + 12, u"Connect"));
	buttons.push_back(std::make_shared<Button>(1, width / 2 - 100, height / 4 + 120 + 12, u"Cancel"));
	
	serverTextField = std::make_unique<GuiTextField>(font, width / 2 - 100, 116, 200, 20);
	serverTextField->setMaxStringLength(128);
	serverTextField->setFocused(true);
	
	// Load last server IP from game settings
	jstring lastIp = minecraft.options.lastMpIp;
	std::replace(lastIp.begin(), lastIp.end(), '_', ':');
	serverTextField->setText(lastIp);
	
	// Enable connect button if text is not empty (matching Java: split(":").length > 0)
	// Java split always returns at least one element, so any non-empty text enables the button
	jstring text = serverTextField->getText();
	buttons[0]->active = !text.empty();
}

void GuiMultiplayer::removed()
{
	lwjgl::Keyboard::enableRepeatEvents(false);
}

void GuiMultiplayer::keyPressed(char_t eventCharacter, int_t eventKey)
{
	serverTextField->textboxKeyTyped(eventCharacter, eventKey);
	if (eventCharacter == 28) // Enter
	{
		if (buttons[0]->active)
			buttonClicked(*buttons[0]);
	}
	
	// Update connect button state (enable if text is not empty)
	jstring text = serverTextField->getText();
	buttons[0]->active = !text.empty();
}

void GuiMultiplayer::buttonClicked(Button &button)
{
	if (!button.active)
		return;

	if (button.id == 1) // Cancel
	{
		// Return to parent screen - caller should manage lifetime
		// For now, just close the screen (caller should handle navigation)
		minecraft.setScreen(nullptr);
	}
	else if (button.id == 0) // Connect
	{
		jstring serverText = serverTextField->getText();
		// Save to game settings (replace : with _)
		jstring saveText = serverText;
		std::replace(saveText.begin(), saveText.end(), ':', '_');
		minecraft.options.lastMpIp = saveText;
		minecraft.options.save();
		
		// Parse IP and port
		size_t colonPos = serverText.find(':');
		jstring hostName = colonPos != jstring::npos ? serverText.substr(0, colonPos) : serverText;
		int_t port = colonPos != jstring::npos ? parseInt(serverText.substr(colonPos + 1), 25565) : 25565;
		
		// Check for "alwaysalpha" easter egg
		jstring lowerHost = hostName;
		std::transform(lowerHost.begin(), lowerHost.end(), lowerHost.begin(), [](char_t c) { return std::tolower(c); });
		if (lowerHost.find(u"alwaysalpha") != jstring::npos)
		{
			throw std::runtime_error("No.");
		}
		
		// Open connecting screen
		minecraft.setScreen(std::make_shared<GuiConnecting>(minecraft, hostName, port));
	}
}

void GuiMultiplayer::render(int_t xm, int_t ym, float a)
{
	// Alpha 1.2.6: GuiMultiplayer.drawScreen() - 1:1 port
	renderBackground();
	drawCenteredString(font, u"Play Multiplayer", width / 2, height / 4 - 60 + 20, 16777215);
	drawString(font, u"Minecraft Multiplayer is currently not finished, but there", width / 2 - 140, height / 4 - 60 + 60 + 0, 10526880);
	drawString(font, u"is some buggy early testing going on.", width / 2 - 140, height / 4 - 60 + 60 + 9, 10526880);
	drawString(font, u"Enter the IP of a server to connect to it:", width / 2 - 140, height / 4 - 60 + 60 + 36, 10526880);
	
	// Alpha 1.2.6: Manually draw textbox (Java lines 75-81)
	// Java: int var4 = this.width / 2 - 100;
	//       int var5 = this.height / 4 - 10 + 50 + 18;
	//       short var6 = 200;
	//       byte var7 = 20;
	//       drawRect(var4 - 1, var5 - 1, var4 + var6 + 1, var5 + var7 + 1, -6250336);
	//       drawRect(var4, var5, var4 + var6, var5 + var7, -16777216);
	//       this.drawString(this.fontRenderer, this.serverTextField.getText() + (this.parentScreen / 6 % 2 == 0 ? "_" : ""), var4 + 4, var5 + (var7 - 8) / 2, 14737632);
	int_t var4 = width / 2 - 100;
	int_t var5 = height / 4 - 10 + 50 + 18;
	short_t var6 = 200;
	byte_t var7 = 20;
	fill(var4 - 1, var5 - 1, var4 + var6 + 1, var5 + var7 + 1, -6250336);  // Border (Java: drawRect)
	fill(var4, var5, var4 + var6, var5 + var7, -16777216);  // Background (Java: drawRect)
	jstring text = serverTextField->getText();
	jstring cursor = (frameCounter / 6 % 2 == 0) ? u"_" : u"";
	drawString(font, text + cursor, var4 + 4, var5 + (var7 - 8) / 2, 14737632);
	
	Screen::render(xm, ym, a);
}

int_t GuiMultiplayer::parseInt(const jstring &str, int_t defaultValue)
{
	try
	{
		jstring trimmed = str;
		trimmed.erase(0, trimmed.find_first_not_of(u" \t\n\r"));
		trimmed.erase(trimmed.find_last_not_of(u" \t\n\r") + 1);
		std::string utf8 = String::toUTF8(trimmed);
		return std::stoi(utf8);
	}
	catch (...)
	{
		return defaultValue;
	}
}
