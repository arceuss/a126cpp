#include "client/player/ControllerInput.h"

#include "client/Minecraft.h"
#include "util/Mth.h"
#include "world/entity/player/Player.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

ControllerInput::ControllerInput(lwjgl::Gamepad::ControllerState* controller, Options &options)
	: controller(controller), options(options)
{
	if (controller == nullptr)
	{
		// Fallback - this shouldn't happen, but handle gracefully
		return;
	}
}

float ControllerInput::getLeftStickDeadzone() const
{
	// Use calibrated deadzone if available, otherwise use default
	if (options.deadzonesCalibrated && options.calibratedLeftStickDeadzone > 0.0f)
		return options.calibratedLeftStickDeadzone;
	return 0.25f;  // Default deadzone for movement stick
}

float ControllerInput::getRightStickDeadzone() const
{
	// Use calibrated deadzone if available, otherwise use default
	if (options.deadzonesCalibrated && options.calibratedRightStickDeadzone > 0.0f)
		return options.calibratedRightStickDeadzone;
	return 0.05f;  // Default deadzone for look stick (small for 1:1 responsiveness)
}

float ControllerInput::getLeftStickX() const
{
	if (controller == nullptr)
		return 0.0f;
	
	// Convert short axis value to float, then apply deadzone
	float raw = ControllerBindings::axisToFloat(controller->leftStickX);
	return ControllerBindings::applyDeadzone(raw, getLeftStickDeadzone());
}

float ControllerInput::getLeftStickY() const
{
	if (controller == nullptr)
		return 0.0f;
	
	// Convert short axis value to float, then apply deadzone
	float raw = ControllerBindings::axisToFloat(controller->leftStickY);
	return ControllerBindings::applyDeadzone(raw, getLeftStickDeadzone());
}

float ControllerInput::getRightStickX() const
{
	if (controller == nullptr)
		return 0.0f;
	
	// Convert short axis value to float, then apply deadzone
	float raw = ControllerBindings::axisToFloat(controller->rightStickX);
	return ControllerBindings::applyDeadzone(raw, getRightStickDeadzone());
}

float ControllerInput::getRightStickY() const
{
	if (controller == nullptr)
		return 0.0f;
	
	// Convert short axis value to float, then apply deadzone
	float raw = ControllerBindings::axisToFloat(controller->rightStickY);
	return ControllerBindings::applyDeadzone(raw, getRightStickDeadzone());
}

float ControllerInput::getLeftTrigger() const
{
	if (controller == nullptr)
		return 0.0f;
	
	// Convert trigger value (0-32767) to float (0.0-1.0)
	return ControllerBindings::triggerToFloat(controller->leftTrigger);
}

float ControllerInput::getRightTrigger() const
{
	if (controller == nullptr)
		return 0.0f;
	
	// Convert trigger value (0-32767) to float (0.0-1.0)
	return ControllerBindings::triggerToFloat(controller->rightTrigger);
}

void ControllerInput::tick(Player &player)
{
	if (controller == nullptr || !controller->connected)
	{
		xa = 0.0f;
		ya = 0.0f;
		jumping = false;
		sneaking = false;
		return;
	}
	
	// If screen is open, disable movement and jump input (GUI mode)
	bool hasScreen = (screenOpen != nullptr && *screenOpen);
	if (hasScreen)
	{
		xa = 0.0f;
		ya = 0.0f;
		jumping = false;
		// Don't process jump/movement input when GUI is open
		// Sneaking can stay (but won't affect anything since movement is disabled)
		return;
	}
	
	// Movement from left stick (matches Controlify: WALK_FORWARD - WALK_BACKWARD, WALK_LEFT - WALK_RIGHT)
	float forwardImpulse = -getLeftStickY(); // Invert Y: up = forward
	float leftImpulse = -getLeftStickX();    // Invert X: left = positive
	
	// Apply keyboard-style movement mode if enabled (threshold-based, not analogue)
	// For now, we use analogue movement (can add toggle later)
	
	// Set movement values
	xa = leftImpulse;
	ya = forwardImpulse;
	
	// Limit movement vector length to 1.0 (like Controlify does for 1.21.5+)
	float lengthSq = xa * xa + ya * ya;
	if (lengthSq > 1.0f)
	{
		float length = sqrtf(lengthSq);
		xa = xa / length;
		ya = ya / length;
	}
	
	// Apply sneaking slowdown (like KeyboardInput)
	if (sneaking)
	{
		xa *= 0.3f;
		ya *= 0.3f;
	}
	
	// Jump handling (matches Controlify: JUMP button, justPressed logic)
	// Controlify: if (jump.justPressed()) jumping = true; if (!jump.digitalNow()) jumping = false;
	bool jumpNow = controller->buttonSouth; // South button (A/Cross) = Jump (default binding)
	if (jumpNow && !prevJump)
		jumping = true;  // Just pressed
	if (!jumpNow)
		jumping = false; // Released
	prevJump = jumpNow;
	
	// Sneak handling (matches Controlify: SNEAK button, toggle logic)
	// Controlify: Right stick button = Sneak (default binding)
	// Alpha 1.2.6 doesn't have abilities system, so we use simpler logic
	bool sneakNow = controller->buttonRightStick;
	if ((player.isInWater() && !player.onGround) || player.riding != nullptr)
	{
		// Hold-to-sneak mode (swimming, riding)
		if (sneakNow && !prevSneak)
			sneaking = true;
		if (!sneakNow)
			sneaking = false;
	}
	else
	{
		// Toggle sneak mode (walking)
		if (sneakNow && !prevSneak)
			sneakToggleState = !sneakToggleState;
		sneaking = sneakToggleState;
	}
	if (player.riding == nullptr && wasPassenger && player.onGround)
	{
		sneaking = false;
		sneakToggleState = false;
	}
	prevSneak = sneakNow;
	wasPassenger = (player.riding != nullptr);
}

void ControllerInput::updateLook(Player &player, float mouseSensitivity, float deltaTime)
{
	if (controller == nullptr || !controller->connected)
		return;
	
	// Right stick for look (matches Controlify: LOOK_RIGHT - LOOK_LEFT, LOOK_DOWN - LOOK_UP)
	// Controlify: regularImpulse = new Vector2d(LOOK_RIGHT - LOOK_LEFT, LOOK_DOWN - LOOK_UP)
	float rightX = getRightStickX();  // LOOK_RIGHT (positive) - LOOK_LEFT (negative)
	float rightY = getRightStickY(); // LOOK_DOWN (positive) - LOOK_UP (negative)
	
	// Apply invert Y setting (Controlify: if (config.vLookInvert) regularImpulse.y *= -1)
	if (options.invertYMouse)
		rightY = -rightY;
	
	// Apply STANDARD input curve (power 2.0) to length to preserve circularity
	// Controlify: regularImpulse = ControllerUtils.applyEasingToLength(regularImpulse, curve::apply)
	// STANDARD curve = InputCurve.power(2.0) = d -> Math.signum(d) * Math.pow(Math.abs(d), 2.0)
	// ControllerUtils.applyEasingToLength: calculate length, if 0 return (0,0), apply curve to length, normalize and scale
	float length = std::sqrt(rightX * rightX + rightY * rightY);
	if (length > 0.001f)
	{
		// Apply power curve to length: signum(length) * pow(abs(length), 2.0)
		// Since length is always positive, signum is always 1.0
		float easedLength = std::pow(length, 2.0f);
		
		// Normalize and scale by eased length (matches ControllerUtils.applyEasingToLength)
		float angle = std::atan2(rightY, rightX);
		rightX = std::cos(angle) * easedLength;
		rightY = std::sin(angle) * easedLength;
	}
	else
	{
		rightX = 0.0f;
		rightY = 0.0f;
	}
	
	// Apply sensitivity: 10 degrees per second at 100% sensitivity
	// Controlify: regularImpulse.x *= config.hLookSensitivity * 10f;
	// Controlify: regularImpulse.y *= config.vLookSensitivity * 10f;
	// Use sensitivity from options (configurable, defaults to 1.0f horizontal, 0.9f vertical)
	// Note: sensitivity values are stored as multipliers (0.1 = 10%, 1.0 = 100%, 2.0 = 200%)
	float hSens = options.controllerHorizontalLookSensitivity;
	float vSens = options.controllerVerticalLookSensitivity;
	
	// Calculate look input in degrees per second (matches Controlify exactly)
	// At 100% sensitivity (1.0), full stick deflection gives 5 degrees per second (halved from 10.0)
	float lookInputX = rightX * hSens * 5.0f;  // degrees per second
	float lookInputY = rightY * vSens * 5.0f;   // degrees per second
	
	// Convert to per-frame rotation (Controlify: velX = lookInputX / 0.15 * deltaTime)
	// Since Entity::turn() multiplies by 0.15 internally, we divide by 0.15 and multiply by deltaTime
	// This matches Controlify's processPlayerLook exactly
	// deltaTime is the partial tick (typically 0.0-1.0, representing fraction of a tick)
	float dx = lookInputX / 0.15f * deltaTime;
	float dy = lookInputY / 0.15f * deltaTime;
	
	// Rotate player (matches Entity::turn usage in GameRenderer)
	// Note: Minecraft's turn() expects Y to be inverted (down = positive rotation)
	dy = -dy; // Invert Y for Minecraft's coordinate system
	
	float absDx = dx >= 0.0f ? dx : -dx;
	float absDy = dy >= 0.0f ? dy : -dy;
	if (absDx > 0.001f || absDy > 0.001f)
	{
		player.turn(dx, dy);
	}
}

void ControllerInput::updateButtonActions(Player &player)
{
	if (controller == nullptr || !controller->connected)
		return;
	
	// Left bumper: Cycle hotbar forwards (move right in hotbar)
	bool leftBumperNow = controller->buttonLeftShoulder;
	if (leftBumperNow && !prevLeftBumper)
	{
		player.inventory.changeCurrentItem(1); // Move right in hotbar
	}
	prevLeftBumper = leftBumperNow;
	
	// Right bumper: Cycle hotbar backwards (move left in hotbar)
	bool rightBumperNow = controller->buttonRightShoulder;
	if (rightBumperNow && !prevRightBumper)
	{
		player.inventory.changeCurrentItem(-1); // Move left in hotbar
	}
	prevRightBumper = rightBumperNow;
	
	// Left stick button: Toggle third person view (matches legacy console edition F5 behavior)
	bool leftStickNow = controller->buttonLeftStick;
	if (leftStickNow && !prevSprint) // prevSprint is reused to track left stick button
	{
		// Toggle third person view (same as F5 key)
		++options.thirdPersonView;
		if (options.thirdPersonView > 2)
			options.thirdPersonView = 0;
	}
	prevSprint = leftStickNow;
	
	// Note: updateTriggerStates() is called at END of frame (after all checks)
}

void ControllerInput::releaseAllKeys()
{
	xa = 0.0f;
	ya = 0.0f;
	jumping = false;
	sneaking = false;
	sneakToggleState = false;
	
	prevJump = false;
	prevSneak = false;
	prevSprint = false;
	prevAttack = false;
	prevUse = false;
	prevInventory = false;
	prevLeftBumper = false;
	prevRightBumper = false;
}

bool ControllerInput::isRightTriggerPressed() const
{
	if (controller == nullptr || !controller->connected)
		return false;
	return getRightTrigger() > 0.5f;
}

bool ControllerInput::isLeftTriggerPressed() const
{
	if (controller == nullptr || !controller->connected)
		return false;
	return getLeftTrigger() > 0.5f;
}

bool ControllerInput::wasRightTriggerJustPressed() const
{
	bool current = isRightTriggerPressed();
	return current && !prevAttack;
}

bool ControllerInput::wasLeftTriggerJustPressed() const
{
	bool current = isLeftTriggerPressed();
	return current && !prevUse;
}

void ControllerInput::updateTriggerStates()
{
	// Update previous frame states for "just pressed" detection
	prevAttack = isRightTriggerPressed();
	prevUse = isLeftTriggerPressed();
}

void ControllerInput::setKey(int_t key, bool eventKeyState)
{
	// Not used for controller input (controller uses direct button/axis access)
	// This method is for compatibility with Input interface
}

bool ControllerInput::isButtonPressed(int button) const
{
	if (controller == nullptr || !controller->connected)
		return false;
	
	// Button IDs match SDL gamepad button enum
	switch (button)
	{
		case 0: return controller->buttonSouth;
		case 1: return controller->buttonEast;
		case 2: return controller->buttonWest;
		case 3: return controller->buttonNorth;
		case 4: return controller->buttonLeftShoulder;
		case 5: return controller->buttonRightShoulder;
		case 6: return controller->buttonBack;
		case 7: return controller->buttonStart;
		case 8: return controller->buttonLeftStick;
		case 9: return controller->buttonRightStick;
		default: return false;
	}
}

bool ControllerInput::wasButtonJustPressed(int button) const
{
	if (controller == nullptr || !controller->connected)
		return false;
	
	// Map button ID to controller state and check "just pressed"
	bool current = false;
	bool prev = false;
	
	switch (button)
	{
		case 0: current = controller->buttonSouth; prev = controller->prevButtonSouth; break;
		case 1: current = controller->buttonEast; prev = controller->prevButtonEast; break;
		case 2: current = controller->buttonWest; prev = controller->prevButtonWest; break;
		case 3: current = controller->buttonNorth; prev = controller->prevButtonNorth; break;
		case 4: current = controller->buttonLeftShoulder; prev = controller->prevButtonLeftShoulder; break;
		case 5: current = controller->buttonRightShoulder; prev = controller->prevButtonRightShoulder; break;
		case 8: current = controller->buttonLeftStick; prev = controller->prevButtonLeftStick; break;
		case 9: current = controller->buttonRightStick; prev = controller->prevButtonRightStick; break;
		default: return false;
	}
	
	return current && !prev;
}

bool ControllerInput::wasButtonJustPressed(SDL_GamepadButton button) const
{
	if (controller == nullptr || !controller->connected)
		return false;
	
	bool current = false;
	bool prev = false;
	
	switch (button)
	{
		case SDL_GAMEPAD_BUTTON_SOUTH: current = controller->buttonSouth; prev = controller->prevButtonSouth; break;
		case SDL_GAMEPAD_BUTTON_EAST: current = controller->buttonEast; prev = controller->prevButtonEast; break;
		case SDL_GAMEPAD_BUTTON_WEST: current = controller->buttonWest; prev = controller->prevButtonWest; break;
		case SDL_GAMEPAD_BUTTON_NORTH: current = controller->buttonNorth; prev = controller->prevButtonNorth; break;
		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: current = controller->buttonLeftShoulder; prev = controller->prevButtonLeftShoulder; break;
		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: current = controller->buttonRightShoulder; prev = controller->prevButtonRightShoulder; break;
		case SDL_GAMEPAD_BUTTON_LEFT_STICK: current = controller->buttonLeftStick; prev = controller->prevButtonLeftStick; break;
		case SDL_GAMEPAD_BUTTON_RIGHT_STICK: current = controller->buttonRightStick; prev = controller->prevButtonRightStick; break;
		case SDL_GAMEPAD_BUTTON_BACK: current = controller->buttonBack; prev = controller->prevButtonBack; break;
		case SDL_GAMEPAD_BUTTON_START: current = controller->buttonStart; prev = controller->prevButtonStart; break;
		default: return false;
	}
	
	return current && !prev;
}

void ControllerInput::playRumble(float lowFrequency, float highFrequency, int_t durationTicks) const
{
	if (controller == nullptr || !controller->connected)
		return;
	
	// Use Gamepad::rumbleGamepad to trigger rumble
	lwjgl::Gamepad::rumbleGamepad(controller, lowFrequency, highFrequency, durationTicks);
}