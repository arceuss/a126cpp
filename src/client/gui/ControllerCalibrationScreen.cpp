#include "client/gui/ControllerCalibrationScreen.h"
#include "client/Minecraft.h"
#include "client/player/ControllerInput.h"
#include "pc/lwjgl/Gamepad.h"
#include "java/String.h"
#include "util/Mth.h"
#include <algorithm>
#include <cmath>

ControllerCalibrationScreen::ControllerCalibrationScreen(Minecraft &minecraft, std::shared_ptr<Screen> lastScreen, Options &options)
	: Screen(minecraft), lastScreen(lastScreen), options(options)
{
	// Get the first connected controller
	controller = lwjgl::Gamepad::getFirstController();
	
	// Initialize data arrays
	leftStickData.resize(CALIBRATION_TIME, 0.0f);
	rightStickData.resize(CALIBRATION_TIME, 0.0f);
}

void ControllerCalibrationScreen::init()
{
	// Check if controller is connected
	if (controller == nullptr || !controller->connected)
	{
		statusText = u"No controller connected!";
		buttons.push_back(std::make_shared<Button>(200, width / 2 - 100, height / 2 + 20, u"Done"));
		return;
	}

	// Check if already calibrated
	if (options.deadzonesCalibrated)
	{
		calibrated = true;
		statusText = u"Calibration complete!";
		buttons.push_back(std::make_shared<Button>(200, width / 2 - 100, height / 2 + 20, u"Done"));
		return;
	}

	// Ready and Later buttons
	readyButton = std::make_shared<Button>(100, width / 2 - 150 - 5, height - 8 - 20, u"Ready");
	laterButton = std::make_shared<Button>(101, width / 2 + 5, height - 8 - 20, u"Later");
	buttons.push_back(readyButton);
	buttons.push_back(laterButton);

	statusText = u"Please keep your controller still and don't touch the sticks.\nPress Ready to start calibration.";
}

void ControllerCalibrationScreen::tick()
{
	// Check if controller disconnected
	if (controller == nullptr || !controller->connected)
	{
		minecraft.setScreen(lastScreen);
		return;
	}

	if (!calibrating)
		return;

	// Update previous values before checking state change
	float leftX = ControllerBindings::axisToFloat(controller->leftStickX);
	float leftY = ControllerBindings::axisToFloat(controller->leftStickY);
	float rightX = ControllerBindings::axisToFloat(controller->rightStickX);
	float rightY = ControllerBindings::axisToFloat(controller->rightStickY);

	// Check if state changed (user moved sticks)
	if (std::abs(leftX - prevLeftStickX) > 0.4f ||
	    std::abs(leftY - prevLeftStickY) > 0.4f ||
	    std::abs(rightX - prevRightStickX) > 0.4f ||
	    std::abs(rightY - prevRightStickY) > 0.4f)
	{
		calibrationTicks = 0;
		leftStickData.assign(CALIBRATION_TIME, 0.0f);
		rightStickData.assign(CALIBRATION_TIME, 0.0f);
		statusText = u"Please keep your controller still!";
	}

	// Update previous values for next frame
	prevLeftStickX = leftX;
	prevLeftStickY = leftY;
	prevRightStickX = rightX;
	prevRightStickY = rightY;

	if (calibrationTicks < CALIBRATION_TIME)
	{
		processAxisData(calibrationTicks);
		calibrationTicks++;
		
		// Update status text with progress
		float progress = static_cast<float>(calibrationTicks) / static_cast<float>(CALIBRATION_TIME);
		int_t percent = static_cast<int_t>(progress * 100.0f);
		statusText = u"Calibrating... " + String::fromUTF8(std::to_string(percent)) + u"%\nKeep your controller still!";
	}
	else
	{
		// Calibration complete
		calibrateAxis();
		
		calibrating = false;
		calibrated = true;
		options.deadzonesCalibrated = true;
		options.save();
		
		readyButton->active = true;
		readyButton->msg = u"Done";
		statusText = u"Calibration complete!\nLeft stick: " + String::fromUTF8(std::to_string(options.calibratedLeftStickDeadzone)) +
		             u"\nRight stick: " + String::fromUTF8(std::to_string(options.calibratedRightStickDeadzone));
	}
}

void ControllerCalibrationScreen::startCalibration()
{
	calibrating = true;
	calibrationTicks = 0;
	leftStickData.assign(CALIBRATION_TIME, 0.0f);
	rightStickData.assign(CALIBRATION_TIME, 0.0f);
	
	readyButton->active = false;
	readyButton->msg = u"Calibrating...";
	
	// Remove later button
	buttons.erase(std::remove_if(buttons.begin(), buttons.end(),
		[this](const std::shared_ptr<Button>& btn) { return btn == laterButton; }), buttons.end());
	readyButton->x = width / 2 - 75;
	
	statusText = u"Calibrating... Keep your controller still!";
}

void ControllerCalibrationScreen::processAxisData(int_t tick)
{
	if (controller == nullptr || !controller->connected)
		return;

	// Get raw axis values (normalized to -1.0 to 1.0)
	float leftX = ControllerBindings::axisToFloat(controller->leftStickX);
	float leftY = ControllerBindings::axisToFloat(controller->leftStickY);
	float rightX = ControllerBindings::axisToFloat(controller->rightStickX);
	float rightY = ControllerBindings::axisToFloat(controller->rightStickY);

	// For each stick group, find the maximum absolute value across all axes
	// Controlify does: max = Math.max(max, Math.abs(state.getAxisState(axis)))
	float leftMax = (std::max)((std::max)(std::abs(leftX), std::abs(leftY)), leftStickData[tick]);
	float rightMax = (std::max)((std::max)(std::abs(rightX), std::abs(rightY)), rightStickData[tick]);

	leftStickData[tick] = leftMax;
	rightStickData[tick] = rightMax;
}

void ControllerCalibrationScreen::calibrateAxis()
{
	// Find maximum absolute value across all ticks for each stick
	// Controlify formula: maxAbs + 0.08f
	float leftMaxAbs = 0.0f;
	float rightMaxAbs = 0.0f;

	for (int_t tick = 0; tick < CALIBRATION_TIME; tick++)
	{
		leftMaxAbs = (std::max)(leftMaxAbs, leftStickData[tick]);
		rightMaxAbs = (std::max)(rightMaxAbs, rightStickData[tick]);
	}

	// Apply Controlify formula: maxAbs + 0.08f
	options.calibratedLeftStickDeadzone = leftMaxAbs + 0.08f;
	options.calibratedRightStickDeadzone = rightMaxAbs + 0.08f;
}


void ControllerCalibrationScreen::buttonClicked(Button &button)
{
	if (!button.active)
		return;

	if (button.id == 100) // Ready
	{
		if (!calibrated)
		{
			startCalibration();
		}
		else
		{
			minecraft.setScreen(lastScreen);
		}
	}
	else if (button.id == 101) // Later
	{
		minecraft.setScreen(lastScreen);
	}
	else if (button.id == 200) // Done
	{
		minecraft.setScreen(lastScreen);
	}
}

void ControllerCalibrationScreen::render(int_t xm, int_t ym, float a)
{
	renderBackground();
	drawCenteredString(font, title, width / 2, 20, 0xFFFFFF);

	// Draw status text (multi-line)
	int_t y = 55;
	std::vector<jstring> lines;
	jstring currentLine = u"";
	for (size_t i = 0; i < statusText.length(); i++)
	{
		if (statusText[i] == '\n')
		{
			if (!currentLine.empty())
			{
				lines.push_back(currentLine);
				currentLine = u"";
			}
		}
		else
		{
			currentLine += statusText[i];
		}
	}
	if (!currentLine.empty())
	{
		lines.push_back(currentLine);
	}

	// Line height constant (matches Minecraft's font line height)
	const int_t lineHeight = 9;
	
	for (size_t i = 0; i < lines.size(); i++)
	{
		drawCenteredString(font, lines[i], width / 2, y + static_cast<int_t>(i) * lineHeight, 0xFFFFFF);
	}

	// Draw progress bar if calibrating
	if (calibrating && calibrationTicks < CALIBRATION_TIME)
	{
		float progress = static_cast<float>(calibrationTicks) / static_cast<float>(CALIBRATION_TIME);
		// Ease out cubic for visual effect (matches Controlify)
		progress = 1.0f - static_cast<float>(std::pow(1.0 - progress, 3.0));
		
		int_t barWidth = 200;
		int_t barHeight = 10;
		int_t barX = width / 2 - barWidth / 2;
		int_t barY = y + static_cast<int_t>(lines.size()) * lineHeight + 10;
		
		// Draw background
		fill(barX, barY, barX + barWidth, barY + barHeight, 0xFF000000);
		// Draw progress
		int_t progressWidth = static_cast<int_t>(barWidth * progress);
		fill(barX, barY, barX + progressWidth, barY + barHeight, 0xFF00FF00);
	}

	Screen::render(xm, ym, a);
}
