#pragma once

#include "client/player/Input.h"
#include "pc/lwjgl/Gamepad.h"
#include "client/Options.h"
#include <cmath>
#include <algorithm>

// Controller input binding system (simplified from Controlify)
namespace ControllerBindings
{
	// Helper to apply deadzone (matches Controlify's deadzone formula)
	inline float applyDeadzone(float value, float deadzone)
	{
		if (deadzone <= 0.0f || deadzone >= 1.0f)
			return value;
		// Controlify formula: (value - Math.copySign(Math.min(deadzone, Math.abs(value)), value)) / (1 - deadzone)
		float sign = (value >= 0.0f) ? 1.0f : -1.0f;
		float absValue = value >= 0.0f ? value : -value;
		float minVal = deadzone < absValue ? deadzone : absValue;
		return (value - sign * minVal) / (1.0f - deadzone);
	}
	
	// Convert short axis value (-32768 to 32767) to float (-1.0 to 1.0)
	inline float axisToFloat(short axis)
	{
		return static_cast<float>(axis) / 32768.0f;
	}
	
	// Convert trigger value (0 to 32767) to float (0.0 to 1.0)
	inline float triggerToFloat(short trigger)
	{
		return static_cast<float>(trigger) / 32767.0f;
	}
}

class ControllerInput : public Input
{
private:
	lwjgl::Gamepad::ControllerState* controller;
	Options &options;
	bool* screenOpen = nullptr; // Pointer to Minecraft::screen != nullptr, set from Minecraft
	
	// Deadzone settings (matching Controlify defaults - defaults to 0f, calibrated per-controller)
	// These will be set from Options if calibrated, otherwise use defaults
	float getLeftStickDeadzone() const;
	float getRightStickDeadzone() const;
	
	// Look sensitivity is now stored in Options (controllerHorizontalLookSensitivity, controllerVerticalLookSensitivity)
	// These are accessed via options in updateLook()
	
	// Previous frame button states (for "justPressed" detection)
	bool prevJump = false;
	bool prevSneak = false;
	bool prevSprint = false;
	bool prevAttack = false;
	bool prevUse = false;
	bool prevInventory = false;
	bool prevLeftBumper = false;
	bool prevRightBumper = false;
	
	// Sneak toggle state (Controlify supports toggle sneak)
	bool sneakToggleState = false;
	bool wasFlying = false;
	bool wasPassenger = false;

public:
	ControllerInput(lwjgl::Gamepad::ControllerState* controller, Options &options);
	
	// Override Input methods
	void tick(Player &player) override;
	void releaseAllKeys() override;
	void setKey(int_t key, bool eventKeyState) override;
	
	// Controller-specific methods
	void updateLook(Player &player, float mouseSensitivity, float deltaTime);
	void updateButtonActions(Player &player);
	void setScreenOpenPtr(bool* ptr) { screenOpen = ptr; } // Set pointer to screen open state
	
	// Trigger state queries (for Minecraft controller functions)
	bool isRightTriggerPressed() const; // Returns true if right trigger is pressed (>0.5f threshold)
	bool isLeftTriggerPressed() const;  // Returns true if left trigger is pressed (>0.5f threshold)
	bool wasRightTriggerJustPressed() const; // Returns true if right trigger was just pressed this frame
	bool wasLeftTriggerJustPressed() const;  // Returns true if left trigger was just pressed this frame
	void updateTriggerStates(); // Call at end of frame to update previous states
	
	// Get axis values with deadzone applied
	float getLeftStickX() const;
	float getLeftStickY() const;
	float getRightStickX() const;
	float getRightStickY() const;
	float getLeftTrigger() const;
	float getRightTrigger() const;
	
	// Button state queries
	bool isButtonPressed(int button) const; // For checking button presses
	bool wasButtonJustPressed(int button) const; // For one-frame button presses
	bool wasButtonJustPressed(SDL_GamepadButton button) const; // SDL enum version
	lwjgl::Gamepad::ControllerState* getControllerState() const { return controller; } // Get raw controller state
	
	// Rumble/Haptic feedback (Controlify-style)
	// Play rumble effect with low-frequency (strong) and high-frequency (weak) motors
	// Values are normalized from 0.0f to 1.0f
	void playRumble(float lowFrequency, float highFrequency, int_t durationTicks) const;
};