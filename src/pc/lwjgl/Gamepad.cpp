#include "lwjgl/Gamepad.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <memory>
#include <string>
#include <cstring>

namespace lwjgl
{
namespace Gamepad
{

static std::vector<std::unique_ptr<ControllerState>> controllers;
static bool initialized = false;

void init()
{
	if (initialized)
		return;
	
	initialized = true;
	
	std::cout << "Initializing gamepad subsystem..." << std::endl;
	
	// Discover connected controllers
	int count = 0;
	SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);
	std::cout << "Found " << count << " joystick(s)" << std::endl;
	if (joysticks != nullptr && count > 0)
	{
		for (int i = 0; i < count; i++)
		{
			SDL_JoystickID jid = joysticks[i];
			
			// Check if it's a gamepad
			if (SDL_IsGamepad(jid))
			{
				SDL_Gamepad* gamepad = SDL_OpenGamepad(jid);
				if (gamepad != nullptr)
				{
					auto state = std::make_unique<ControllerState>();
					state->gamepad = gamepad;
					state->instanceId = jid;
					const char* name = SDL_GetGamepadName(gamepad);
					state->name = name ? name : "Unknown Controller";
					state->connected = true;
					
					std::string controllerName = state->name;
					controllers.push_back(std::move(state));
					
					std::cout << "Controller connected: " << controllerName << " (ID: " << jid << ")" << std::endl;
				}
			}
		}
		SDL_free(joysticks);
	}
	
	if (controllers.empty())
	{
		std::cout << "No gamepads found" << std::endl;
	}
}

void handleGamepadAdded(SDL_JoystickID jid)
{
	if (!SDL_IsGamepad(jid))
		return;
	
	SDL_Gamepad* gamepad = SDL_OpenGamepad(jid);
	if (gamepad != nullptr)
	{
		auto state = std::make_unique<ControllerState>();
		state->gamepad = gamepad;
		state->instanceId = jid;
		const char* name = SDL_GetGamepadName(gamepad);
		state->name = name ? name : "Unknown Controller";
		state->connected = true;
		
		std::string controllerName = state->name;
		controllers.push_back(std::move(state));
		
		std::cout << "Controller connected: " << controllerName << " (ID: " << jid << ")" << std::endl;
	}
}

void handleGamepadRemoved(SDL_JoystickID jid)
{
	auto it = std::find_if(controllers.begin(), controllers.end(),
		[jid](const std::unique_ptr<ControllerState>& state) {
			return state->instanceId == jid;
		});
	
	if (it != controllers.end())
	{
		std::cout << "Controller disconnected: " << (*it)->name << std::endl;
		if ((*it)->gamepad != nullptr)
		{
			SDL_CloseGamepad((*it)->gamepad);
		}
		controllers.erase(it);
	}
}

void update()
{
	if (!initialized)
		return;
	
	// Update controller states (events are handled centrally in Minecraft::tick())
	for (auto& controller : controllers)
	{
		if (!controller->connected || controller->gamepad == nullptr)
			continue;
		
		// Store previous frame states
		controller->prevButtonSouth = controller->buttonSouth;
		controller->prevButtonEast = controller->buttonEast;
		controller->prevButtonWest = controller->buttonWest;
		controller->prevButtonNorth = controller->buttonNorth;
		controller->prevButtonLeftShoulder = controller->buttonLeftShoulder;
		controller->prevButtonRightShoulder = controller->buttonRightShoulder;
		controller->prevButtonLeftStick = controller->buttonLeftStick;
		controller->prevButtonRightStick = controller->buttonRightStick;
		controller->prevButtonBack = controller->buttonBack;
		controller->prevButtonStart = controller->buttonStart;
		
		// Read button states
		controller->buttonSouth = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_SOUTH) != 0;
		controller->buttonEast = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_EAST) != 0;
		controller->buttonWest = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_WEST) != 0;
		controller->buttonNorth = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_NORTH) != 0;
		
		controller->buttonLeftShoulder = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) != 0;
		controller->buttonRightShoulder = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) != 0;
		controller->buttonLeftStick = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK) != 0;
		controller->buttonRightStick = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK) != 0;
		controller->buttonBack = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_BACK) != 0;
		controller->buttonStart = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_START) != 0;
		controller->buttonGuide = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_GUIDE) != 0;
		
		controller->buttonDpadUp = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) != 0;
		controller->buttonDpadDown = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) != 0;
		controller->buttonDpadLeft = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) != 0;
		controller->buttonDpadRight = SDL_GetGamepadButton(controller->gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) != 0;
		
		// Read axis states
		controller->leftStickX = SDL_GetGamepadAxis(controller->gamepad, SDL_GAMEPAD_AXIS_LEFTX);
		controller->leftStickY = SDL_GetGamepadAxis(controller->gamepad, SDL_GAMEPAD_AXIS_LEFTY);
		controller->rightStickX = SDL_GetGamepadAxis(controller->gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
		controller->rightStickY = SDL_GetGamepadAxis(controller->gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
		controller->leftTrigger = SDL_GetGamepadAxis(controller->gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
		controller->rightTrigger = SDL_GetGamepadAxis(controller->gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
	}
}

int_t getControllerCount()
{
	return static_cast<int_t>(controllers.size());
}

ControllerState* getController(int_t index)
{
	if (index < 0 || index >= static_cast<int_t>(controllers.size()))
		return nullptr;
	
	if (!controllers[index]->connected)
		return nullptr;
	
	return controllers[index].get();
}

ControllerState* getFirstController()
{
	return getController(0);
}

bool rumbleGamepad(ControllerState* controller, float lowFrequency, float highFrequency, Uint32 durationMs)
{
	if (controller == nullptr || controller->gamepad == nullptr || !controller->connected)
		return false;
	
	// Clamp values to valid range (0.0 to 1.0)
	lowFrequency = std::max(0.0f, std::min(1.0f, lowFrequency));
	highFrequency = std::max(0.0f, std::min(1.0f, highFrequency));
	
	// Convert float (0.0-1.0) to Uint16 (0-0xFFFF) for SDL
	Uint16 lowFreqRumble = static_cast<Uint16>(lowFrequency * 0xFFFF);
	Uint16 highFreqRumble = static_cast<Uint16>(highFrequency * 0xFFFF);
	
	return SDL_RumbleGamepad(controller->gamepad, lowFreqRumble, highFreqRumble, durationMs);
}

bool rumbleGamepad(ControllerState* controller, float lowFrequency, float highFrequency, int_t durationTicks)
{
	// Convert ticks to milliseconds (20 ticks per second = 50ms per tick)
	Uint32 durationMs = static_cast<Uint32>(durationTicks * 50);
	return rumbleGamepad(controller, lowFrequency, highFrequency, durationMs);
}

void shutdown()
{
	for (auto& controller : controllers)
	{
		if (controller->gamepad != nullptr)
		{
			SDL_CloseGamepad(controller->gamepad);
			controller->gamepad = nullptr;
		}
		controller->connected = false;
	}
	controllers.clear();
	initialized = false;
}

} // namespace Gamepad
} // namespace lwjgl