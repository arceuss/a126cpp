#pragma once

#include "java/Type.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include <memory>
#include <vector>
#include <string>

namespace lwjgl
{
namespace Gamepad
{

// Controller state structure
struct ControllerState
{
	SDL_Gamepad* gamepad = nullptr;
	SDL_JoystickID instanceId = 0;
	std::string name;
	bool connected = false;
	
	// Button states (standard gamepad buttons)
	bool buttonSouth = false;      // A / Cross
	bool buttonEast = false;       // B / Circle
	bool buttonWest = false;       // X / Square
	bool buttonNorth = false;      // Y / Triangle
	
	bool buttonLeftShoulder = false;
	bool buttonRightShoulder = false;
	bool buttonLeftStick = false;
	bool buttonRightStick = false;
	bool buttonBack = false;       // Select / Share
	bool buttonStart = false;      // Start / Options
	bool buttonGuide = false;      // Xbox / PS button
	
	bool buttonDpadUp = false;
	bool buttonDpadDown = false;
	bool buttonDpadLeft = false;
	bool buttonDpadRight = false;
	
	// Axis values (range: -32768 to 32767)
	short leftStickX = 0;
	short leftStickY = 0;
	short rightStickX = 0;
	short rightStickY = 0;
	short leftTrigger = 0;         // Range: 0 to 32767
	short rightTrigger = 0;        // Range: 0 to 32767
	
	// Previous frame states (for detecting button presses)
	bool prevButtonSouth = false;
	bool prevButtonEast = false;
	bool prevButtonWest = false;
	bool prevButtonNorth = false;
	bool prevButtonLeftShoulder = false;
	bool prevButtonRightShoulder = false;
	bool prevButtonLeftStick = false;
	bool prevButtonRightStick = false;
	bool prevButtonBack = false;
	bool prevButtonStart = false;
	bool prevButtonJump = false;
	bool prevButtonSneak = false;
};

// Initialize gamepad system
void init();

// Update gamepad states (call every frame)
void update();

// Get number of connected controllers
int_t getControllerCount();

// Get controller state by index (0 = first controller)
ControllerState* getController(int_t index);

// Get the first connected controller (for now, we'll use the first one)
ControllerState* getFirstController();

// Event handlers (called from centralized SDL event loop)
void handleGamepadAdded(SDL_JoystickID jid);
void handleGamepadRemoved(SDL_JoystickID jid);

// Rumble/Haptic feedback
// Rumble the controller with low-frequency (strong) and high-frequency (weak) motors
// Values are normalized from 0.0f to 1.0f, duration is in milliseconds
// Returns true on success, false if rumble is not supported
bool rumbleGamepad(ControllerState* controller, float lowFrequency, float highFrequency, Uint32 durationMs);
bool rumbleGamepad(ControllerState* controller, float lowFrequency, float highFrequency, int_t durationTicks);

// Cleanup
void shutdown();

} // namespace Gamepad
} // namespace lwjgl