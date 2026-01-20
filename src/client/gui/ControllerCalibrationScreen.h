#pragma once

#include "client/gui/Screen.h"
#include "client/Options.h"
#include "pc/lwjgl/Gamepad.h"
#include <vector>
#include <string>

class ControllerCalibrationScreen : public Screen
{
private:
	static constexpr int_t CALIBRATION_TIME = 100; // 100 ticks = 5 seconds at 20 tps

	std::shared_ptr<Screen> lastScreen;
	Options &options;
	lwjgl::Gamepad::ControllerState* controller;

	// Calibration state
	bool calibrating = false;
	bool calibrated = false;
	int_t calibrationTicks = 0;

	// Axis data collection (for left and right sticks)
	// We track the maximum absolute value for each stick group over 100 ticks
	std::vector<float> leftStickData;   // Max abs value per tick for left stick
	std::vector<float> rightStickData;  // Max abs value per tick for right stick

	// Previous frame state (for detecting movement)
	float prevLeftStickX = 0.0f;
	float prevLeftStickY = 0.0f;
	float prevRightStickX = 0.0f;
	float prevRightStickY = 0.0f;

	// UI
	std::shared_ptr<Button> readyButton;
	std::shared_ptr<Button> laterButton;
	jstring statusText = u"";

protected:
	jstring title = u"Controller Calibration";

public:
	ControllerCalibrationScreen(Minecraft &minecraft, std::shared_ptr<Screen> lastScreen, Options &options);

	void init() override;
	void tick() override;
	void buttonClicked(Button &button) override;
	void render(int_t xm, int_t ym, float a) override;

private:
	void startCalibration();
	void processAxisData(int_t tick);
	void calibrateAxis();
};
