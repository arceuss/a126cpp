#include "client/gui/ControllerSettingsScreen.h"

#include "client/Minecraft.h"
#include "client/Options.h"
#include "client/gui/SlideButton.h"
#include "client/player/ControllerInput.h"
#include "pc/lwjgl/Gamepad.h"
#include "java/String.h"
#include <cmath>

ControllerSettingsScreen::ControllerSettingsScreen(Minecraft &minecraft, std::shared_ptr<Screen> lastScreen, Options &options)
	: Screen(minecraft), lastScreen(lastScreen), options(options)
{
}

void ControllerSettingsScreen::init()
{
	title = u"Controller Settings";
	
	// Clamp sensitivity values to valid range (0.1-2.0) before using them
	float hSens = options.controllerHorizontalLookSensitivity;
	if (hSens < 0.1f) hSens = 0.1f;
	if (hSens > 2.0f) hSens = 2.0f;
	options.controllerHorizontalLookSensitivity = hSens;
	
	float vSens = options.controllerVerticalLookSensitivity;
	if (vSens < 0.1f) vSens = 0.1f;
	if (vSens > 2.0f) vSens = 2.0f;
	options.controllerVerticalLookSensitivity = vSens;
	
	// Horizontal Look Sensitivity slider (0.1 to 2.0, step 0.05)
	// Map sensitivity (0.1-2.0) to slider value (0.0-1.0): (sens - 0.1) / 1.9
	float hSliderValue = (hSens - 0.1f) / 1.9f;
	if (hSliderValue < 0.0f) hSliderValue = 0.0f;
	if (hSliderValue > 1.0f) hSliderValue = 1.0f;
	int_t hSensPercent = static_cast<int_t>(std::round(hSens * 100.0f));
	jstring hSensText = u"Horizontal Look Sensitivity: " + String::toString(hSensPercent) + u"%";
	buttons.push_back(std::make_shared<SlideButton>(100, width / 2 - 100, height / 2 - 60, nullptr, hSensText, hSliderValue));
	
	// Vertical Look Sensitivity slider (0.1 to 2.0, step 0.05)
	float vSliderValue = (vSens - 0.1f) / 1.9f;
	if (vSliderValue < 0.0f) vSliderValue = 0.0f;
	if (vSliderValue > 1.0f) vSliderValue = 1.0f;
	int_t vSensPercent = static_cast<int_t>(std::round(vSens * 100.0f));
	jstring vSensText = u"Vertical Look Sensitivity: " + String::toString(vSensPercent) + u"%";
	buttons.push_back(std::make_shared<SlideButton>(101, width / 2 - 100, height / 2 - 36, nullptr, vSensText, vSliderValue));
	
	// Done button
	buttons.push_back(std::make_shared<Button>(200, width / 2 - 100, height / 2 + 28, u"Done"));
}

void ControllerSettingsScreen::tick()
{
	// Update sensitivity values and button text while sliders are being dragged (mouse/keyboard)
	for (auto &button : buttons)
	{
		if (button->id == 100) // Horizontal Look Sensitivity
		{
			auto *slideButton = dynamic_cast<SlideButton*>(button.get());
			if (slideButton != nullptr && slideButton->sliding)
			{
				// Map slider value (0.0-1.0) to sensitivity (0.1-2.0): slider * 1.9 + 0.1
				float sliderValue = slideButton->value;
				float sens = sliderValue * 1.9f + 0.1f;
				// Round to nearest 0.05
				sens = std::round(sens * 20.0f) / 20.0f;
				// Clamp to valid range
				if (sens < 0.1f) sens = 0.1f;
				if (sens > 2.0f) sens = 2.0f;
				options.controllerHorizontalLookSensitivity = sens;
				
				// Update button text
				int_t percent = static_cast<int_t>(std::round(sens * 100.0f));
				slideButton->msg = u"Horizontal Look Sensitivity: " + String::toString(percent) + u"%";
			}
		}
		else if (button->id == 101) // Vertical Look Sensitivity
		{
			auto *slideButton = dynamic_cast<SlideButton*>(button.get());
			if (slideButton != nullptr && slideButton->sliding)
			{
				// Map slider value (0.0-1.0) to sensitivity (0.1-2.0): slider * 1.9 + 0.1
				float sliderValue = slideButton->value;
				float sens = sliderValue * 1.9f + 0.1f;
				// Round to nearest 0.05
				sens = std::round(sens * 20.0f) / 20.0f;
				// Clamp to valid range
				if (sens < 0.1f) sens = 0.1f;
				if (sens > 2.0f) sens = 2.0f;
				options.controllerVerticalLookSensitivity = sens;
				
				// Update button text
				int_t percent = static_cast<int_t>(std::round(sens * 100.0f));
				slideButton->msg = u"Vertical Look Sensitivity: " + String::toString(percent) + u"%";
			}
		}
	}
	
	// Controller input for sliders is now handled globally in Minecraft::updateControllerCursor
	// But we still need to handle the custom sensitivity sliders (they don't have Option::Element)
	// Check if a custom slider was adjusted via controller and update its value
	for (auto &button : buttons)
	{
		if (button->id == 100 || button->id == 101)
		{
			auto *slideButton = dynamic_cast<SlideButton*>(button.get());
			if (slideButton != nullptr && button->active)
			{
				// Map slider value (0.0-1.0) to sensitivity (0.1-2.0): slider * 1.9 + 0.1
				float sens = slideButton->value * 1.9f + 0.1f;
				sens = std::round(sens * 20.0f) / 20.0f;
				if (sens < 0.1f) sens = 0.1f;
				if (sens > 2.0f) sens = 2.0f;
				
				// Update if value changed (controller adjusted it)
				if (button->id == 100 && options.controllerHorizontalLookSensitivity != sens)
				{
					options.controllerHorizontalLookSensitivity = sens;
					int_t percent = static_cast<int_t>(std::round(sens * 100.0f));
					slideButton->msg = u"Horizontal Look Sensitivity: " + String::toString(percent) + u"%";
					options.save();
				}
				else if (button->id == 101 && options.controllerVerticalLookSensitivity != sens)
				{
					options.controllerVerticalLookSensitivity = sens;
					int_t percent = static_cast<int_t>(std::round(sens * 100.0f));
					slideButton->msg = u"Vertical Look Sensitivity: " + String::toString(percent) + u"%";
					options.save();
				}
			}
		}
	}
}

void ControllerSettingsScreen::buttonClicked(Button &button)
{
	if (!button.active)
		return;
	
	// Save options when slider is released (buttonClicked is called on mouse release)
	if (button.id == 100 || button.id == 101)
	{
		auto *slideButton = dynamic_cast<SlideButton*>(&button);
		if (slideButton != nullptr)
		{
			// Final update and save when slider is released
			float sliderValue = slideButton->value;
			float sens = sliderValue * 1.9f + 0.1f;
			sens = std::round(sens * 20.0f) / 20.0f;
			if (sens < 0.1f) sens = 0.1f;
			if (sens > 2.0f) sens = 2.0f;
			
			if (button.id == 100)
				options.controllerHorizontalLookSensitivity = sens;
			else
				options.controllerVerticalLookSensitivity = sens;
			
			options.save();
		}
	}
	else if (button.id == 200) // Done
	{
		options.save();
		minecraft.setScreen(lastScreen);
	}
}

void ControllerSettingsScreen::keyPressed(char_t eventCharacter, int_t eventKey)
{
	// Handle keyboard input for sliders (left/right arrow keys)
	// This allows keyboard users to adjust sliders, and also simulates what Controlify does
	for (auto &button : buttons)
	{
		if (button->id == 100 || button->id == 101)
		{
			auto *slideButton = dynamic_cast<SlideButton*>(button.get());
			if (slideButton != nullptr && button->active)
			{
				// Check if this button is hovered
				int_t xm = static_cast<int_t>(minecraft.controllerCursorX);
				int_t ym = static_cast<int_t>(minecraft.controllerCursorY);
				bool hovered = xm >= button->x && ym >= button->y && xm < button->x + button->w && ym < button->y + button->h;
				
				if (hovered)
				{
					float step = 0.05f / 1.9f; // Step size in slider value
					
					if (eventKey == 205) // Right arrow key
					{
						slideButton->value += step;
						if (slideButton->value > 1.0f) slideButton->value = 1.0f;
					}
					else if (eventKey == 203) // Left arrow key
					{
						slideButton->value -= step;
						if (slideButton->value < 0.0f) slideButton->value = 0.0f;
					}
					else
					{
						continue; // Not left/right arrow, let other handlers process it
					}
					
					// Update sensitivity value
					float sens = slideButton->value * 1.9f + 0.1f;
					sens = std::round(sens * 20.0f) / 20.0f;
					if (sens < 0.1f) sens = 0.1f;
					if (sens > 2.0f) sens = 2.0f;
					
					if (button->id == 100)
						options.controllerHorizontalLookSensitivity = sens;
					else
						options.controllerVerticalLookSensitivity = sens;
					
					// Update button text
					int_t percent = static_cast<int_t>(std::round(sens * 100.0f));
					slideButton->msg = (button->id == 100 ? u"Horizontal Look Sensitivity: " : u"Vertical Look Sensitivity: ") + String::toString(percent) + u"%";
					
					options.save();
					return; // Handled
				}
			}
		}
	}
	
	// Not handled, call parent
	Screen::keyPressed(eventCharacter, eventKey);
}

void ControllerSettingsScreen::render(int_t xm, int_t ym, float a)
{
	renderBackground();
	drawCenteredString(font, title, width / 2, 20, 0xFFFFFF);
	
	Screen::render(xm, ym, a);
}
