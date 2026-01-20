#include "client/gui/ClientOptionsScreen.h"

#include "client/Minecraft.h"
#include "client/Options.h"
#include "client/gui/GuiTextField.h"
#include "client/gui/ControllerCalibrationScreen.h"
#include "client/gui/ControllerSettingsScreen.h"
#include "pc/lwjgl/Gamepad.h"
#include "lwjgl/Keyboard.h"
#include "java/String.h"
#include <iostream>

ClientOptionsScreen::ClientOptionsScreen(Minecraft &minecraft, std::shared_ptr<Screen> lastScreen, Options &options)
	: Screen(minecraft), lastScreen(lastScreen), options(options)
{
}

void ClientOptionsScreen::init()
{
	lwjgl::Keyboard::enableRepeatEvents(true);
	
	title = u"Client Options";
	
	// Username text field
	usernameField = std::make_unique<GuiTextField>(font, width / 2 - 100, height / 2 - 40, 200, 20);
	usernameField->setMaxStringLength(16); // Minecraft username limit
	usernameField->setText(options.username);
	usernameField->setFocused(true);
	
	// Controller Calibration button (only show if controller is connected)
	if (lwjgl::Gamepad::getFirstController() != nullptr && lwjgl::Gamepad::getFirstController()->connected)
	{
		jstring calibText = options.deadzonesCalibrated ? u"Recalibrate Controller" : u"Calibrate Controller";
		buttons.push_back(std::make_shared<Button>(102, width / 2 - 100, height / 2 - 20, calibText));
		
		// Controller Settings button
		buttons.push_back(std::make_shared<Button>(103, width / 2 - 100, height / 2 + 4, u"Controller Settings"));
	}
	
	// Done button
	buttons.push_back(std::make_shared<Button>(200, width / 2 - 100, height / 2 + 28, u"Done"));
}

void ClientOptionsScreen::removed()
{
	lwjgl::Keyboard::enableRepeatEvents(false);
}

void ClientOptionsScreen::tick()
{
	usernameField->updateCursorCounter();
}

void ClientOptionsScreen::keyPressed(char_t eventCharacter, int_t eventKey)
{
	usernameField->textboxKeyTyped(eventCharacter, eventKey);
	
	if (eventKey == lwjgl::Keyboard::KEY_ESCAPE)
	{
		// Cancel - don't save changes
		minecraft.setScreen(lastScreen);
	}
	else if (eventCharacter == 28 || eventKey == lwjgl::Keyboard::KEY_RETURN) // Enter
	{
		// Save and close
		buttonClicked(*buttons[0]);
	}
	else
	{
		Screen::keyPressed(eventCharacter, eventKey);
	}
}

void ClientOptionsScreen::mouseClicked(int_t x, int_t y, int_t buttonNum)
{
	usernameField->mouseClicked(x, y, buttonNum);
	Screen::mouseClicked(x, y, buttonNum);
}

void ClientOptionsScreen::buttonClicked(Button &button)
{
	if (!button.active)
		return;
	
	if (button.id == 102) // Calibrate Controller
	{
		minecraft.setScreen(std::make_shared<ControllerCalibrationScreen>(minecraft, minecraft.screen, options));
		return;
	}
	
	if (button.id == 103) // Controller Settings
	{
		minecraft.setScreen(std::make_shared<ControllerSettingsScreen>(minecraft, minecraft.screen, options));
		return;
	}
	
	if (button.id == 200) // Done
	{
		// Save username to options
		jstring newUsername = usernameField->getText();
		
		// Trim whitespace
		while (!newUsername.empty() && newUsername[0] == ' ')
			newUsername = newUsername.substr(1);
		while (!newUsername.empty() && newUsername.back() == ' ')
			newUsername = newUsername.substr(0, newUsername.length() - 1);
		
		// Ensure username is not empty and not too long
		if (newUsername.empty())
		{
			newUsername = u"Player";
		}
		else if (newUsername.length() > 16)
		{
			newUsername = newUsername.substr(0, 16);
		}
		
		// Check if username actually changed
		if (options.username != newUsername)
		{
			// Remove old skin texture from cache if player exists
			if (minecraft.player != nullptr && !minecraft.player->customTextureUrl.empty())
			{
				minecraft.textures.removeHttpTexture(minecraft.player->customTextureUrl);
			}
			
			// Update username in options
			options.username = newUsername;
			options.save();
			
			// Update user object if it exists
			if (minecraft.user != nullptr)
			{
				minecraft.user->name = newUsername;
			}
			
			// Update player's name and skin URL if player exists
			if (minecraft.player != nullptr)
			{
				minecraft.player->name = newUsername;
				minecraft.player->customTextureUrl = u"http://betacraft.uk:11705/skin/" + newUsername + u".png";
				std::cout << "Updated player skin URL: " << String::toUTF8(minecraft.player->customTextureUrl) << std::endl;
			}
		}
		else
		{
			// Username didn't change, just save options
			options.save();
		}
		
		minecraft.setScreen(lastScreen);
	}
}

void ClientOptionsScreen::render(int_t xm, int_t ym, float a)
{
	renderBackground();
	drawCenteredString(font, title, width / 2, 20, 0xFFFFFF);
	
	// Draw "Username:" label
	drawString(font, u"Username:", width / 2 - 100, height / 2 - 52, 0xFFFFFF);
	
	// Draw text field
	usernameField->drawTextBox();
	
	Screen::render(xm, ym, a);
}
