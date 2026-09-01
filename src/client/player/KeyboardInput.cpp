#include "client/player/KeyboardInput.h"

#include "client/Options.h"
#include "lwjgl/Gamepad.h"
#include "world/entity/player/Player.h"

#include <algorithm>

KeyboardInput::KeyboardInput(Options &options) : options(options)
{

}

void KeyboardInput::setKey(int_t key, bool state)
{
	int_t id = -1;
	if (key == options.keyUp.key) id = KEY_UP;
	if (key == options.keyDown.key) id = KEY_DOWN;
	if (key == options.keyLeft.key) id = KEY_LEFT;
	if (key == options.keyRight.key) id = KEY_RIGHT;
	if (key == options.keyJump.key) id = KEY_JUMP;
	if (key == options.keySneak.key) id = KEY_SNEAK;
	if (id >= 0)
		keys[id] = state;
}

void KeyboardInput::releaseAllKeys()
{
	keys.fill(false);
}

void KeyboardInput::tick(Player &player)
{
	xa = 0.0f;
	ya = 0.0f;

	if (keys[KEY_UP]) ya++;
	if (keys[KEY_DOWN]) ya--;
	if (keys[KEY_LEFT]) xa++;
	if (keys[KEY_RIGHT]) xa--;

	// Analog stick is merged here rather than synthesised as four keys, so
	// partial-speed movement survives.
	xa += lwjgl::Gamepad::getMoveLeftRight();
	ya += lwjgl::Gamepad::getMoveForward();
	xa = std::max(-1.0f, std::min(1.0f, xa));
	ya = std::max(-1.0f, std::min(1.0f, ya));

	const int_t hotbarDirection = lwjgl::Gamepad::consumeHotbarDirection();
	if (hotbarDirection != 0)
		player.inventory.changeCurrentItem(hotbarDirection);

	jumping = keys[KEY_JUMP];
	sneaking = keys[KEY_SNEAK] || lwjgl::Gamepad::isSneaking();

	if (sneaking)
	{
		xa *= 0.3f;
		ya *= 0.3f;
	}
}
