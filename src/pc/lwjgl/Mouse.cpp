#include "lwjgl/Mouse.h"

#include <queue>
#include <algorithm>

#include "backends/Platform/Platform.h"
#include "lwjgl/Display.h"

#include "external/SDLException.h"

#include "SDL_video.h"
#include "SDL_mouse.h"

namespace lwjgl
{
namespace Mouse
{

static int_t staging_dx = 0;
static int_t staging_dy = 0;
static int_t staging_dz = 0;

// Driven by the Switch controller layer. SDL_GetMouseState reflects only real
// hardware, so synthesised buttons and pointer position are tracked here.
static Uint32 synthetic_buttons = 0;
static bool synthetic_pointer_active = false;
static int_t synthetic_pointer_x = 0;
static int_t synthetic_pointer_y = 0;

namespace detail
{

struct Event
{
	Sint8 button, down;
	Sint32 x, y;
	Sint32 xrel, yrel;
	Sint32 wheel;

	Event(Sint8 button = 0, Sint8 down = 0, Sint32 x = 0, Sint32 y = 0, Sint32 xrel = 0, Sint32 yrel = 0, Sint32 wheel = 0)
		: button(button), down(down), x(x), y(y), xrel(xrel), yrel(yrel), wheel(wheel)
	{ }
};

static Event event_current = {};
static std::queue<Event> event_queue;

// Beta: Map SDL button numbers to LWJGL button numbers
// SDL: 1=Left, 2=Middle, 3=Right
// LWJGL: 0=Left, 1=Right, 2=Middle
static int_t sdlToLwjglButton(int_t sdlButton)
{
	if (sdlButton == 1) return 0; // Left
	if (sdlButton == 3) return 1; // Right
	if (sdlButton == 2) return 2; // Middle
	return sdlButton - 1; // fallback for other buttons
}

// Inverse of the above, for buttons synthesised by the controller layer.
static int_t lwjglToSdlButtonLocal(int_t lwjglButton)
{
	if (lwjglButton == 0) return 1; // Left
	if (lwjglButton == 1) return 3; // Right
	if (lwjglButton == 2) return 2; // Middle
	return lwjglButton + 1; // fallback for other buttons
}

void pushEvent(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_MOUSEMOTION:
			staging_dx += e.motion.xrel;
			staging_dy -= e.motion.yrel;
			event_queue.emplace(-1, 0, e.motion.x, Display::getHeight() - e.motion.y - 1, e.motion.xrel, -e.motion.yrel, 0);
			break;
		case SDL_MOUSEWHEEL:
			event_queue.emplace(-1, 0, e.wheel.mouseX, Display::getHeight() - e.wheel.mouseY - 1, 0, 0, e.wheel.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			event_queue.emplace(sdlToLwjglButton(e.button.button), 1, e.button.x, Display::getHeight() - e.button.y - 1, 0, 0, 0);
			break;
		case SDL_MOUSEBUTTONUP:
			event_queue.emplace(sdlToLwjglButton(e.button.button), 0, e.button.x, Display::getHeight() - e.button.y - 1, 0, 0, 0);
			break;
	}
}

void addSyntheticRelativeMotion(int_t dx, int_t dy)
{
	staging_dx += dx;
	staging_dy += dy;
}

void setSyntheticButtonState(int_t button, bool down)
{
	const Uint32 mask = SDL_BUTTON(lwjglToSdlButtonLocal(button));
	if (((synthetic_buttons & mask) != 0) == down)
		return;

	if (down)
		synthetic_buttons |= mask;
	else
		synthetic_buttons &= ~mask;

	event_queue.emplace(static_cast<Sint8>(button), down ? 1 : 0, getX(), getY(), 0, 0, 0);
}

void setSyntheticPointerActive(bool active)
{
	synthetic_pointer_active = active;
}

void setSyntheticPointerPosition(int_t x, int_t y)
{
	synthetic_pointer_x = std::max(0, std::min(x, Display::getWidth() - 1));
	synthetic_pointer_y = std::max(0, std::min(y, Display::getHeight() - 1));
}

void moveSyntheticPointer(int_t dx, int_t dy)
{
	if (!synthetic_pointer_active)
		return;
	setSyntheticPointerPosition(synthetic_pointer_x + dx, synthetic_pointer_y + dy);
}

}

void setCursorPosition(int_t x, int_t y)
{
	platform::setCursorPosition(x, y);
}

// Event handling
bool next()
{
	if (detail::event_queue.empty())
		return false;
	detail::event_current = detail::event_queue.front();
	detail::event_queue.pop();
	return true;
}

int_t getEventButton()
{
	return detail::event_current.button;
}
bool getEventButtonState()
{
	return detail::event_current.down != 0;
}

int_t getEventDX()
{
	return detail::event_current.xrel;
}
int_t getEventDY()
{
	return detail::event_current.yrel;
}

int_t getEventX()
{
	return detail::event_current.x;
}
int_t getEventY()
{
	return detail::event_current.y;
}

int_t getEventDWheel()
{
	return detail::event_current.wheel;
}

// State
int_t getX()
{
	if (synthetic_pointer_active)
		return synthetic_pointer_x;

	int x;
	SDL_GetMouseState(&x, nullptr);
	return x;
}

int_t getY()
{
	if (synthetic_pointer_active)
		return Display::getHeight() - synthetic_pointer_y - 1;

	int y;
	SDL_GetMouseState(nullptr, &y);
	return lwjgl::Display::getHeight() - y - 1;
}

int_t getDX()
{
	int_t result = staging_dx;
	staging_dx = 0;
	return result;
}

int_t getDY()
{
	int_t result = staging_dy;
	staging_dy = 0;
	return result;
}

int_t getDWheel()
{
	int_t result = staging_dz;
	staging_dz = 0;
	return result;
}

// Beta: Map LWJGL button numbers to SDL button numbers
// LWJGL: 0=Left, 1=Right, 2=Middle
// SDL: 1=Left, 2=Middle, 3=Right
static int_t lwjglToSdlButton(int_t lwjglButton)
{
	if (lwjglButton == 0) return 1; // Left
	if (lwjglButton == 1) return 3; // Right
	if (lwjglButton == 2) return 2; // Middle
	return lwjglButton + 1; // fallback for other buttons
}

bool isButtonDown(int_t button)
{
	const Uint32 mask = SDL_BUTTON(lwjglToSdlButton(button));
	return (synthetic_buttons & mask) != 0 ||
		(SDL_GetMouseState(nullptr, nullptr) & mask) != 0;
}

bool isGrabbed()
{
	return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

void setGrabbed(bool grabbed)
{
	staging_dx = 0;
	staging_dy = 0;

	if (SDL_ShowCursor(grabbed ? SDL_DISABLE : SDL_ENABLE) < 0)
		throw SDLException();
	SDL_SetRelativeMouseMode(grabbed ? SDL_TRUE : SDL_FALSE);
	// SDL_CaptureMouse(grabbed ? SDL_TRUE : SDL_FALSE);
}

}
}
