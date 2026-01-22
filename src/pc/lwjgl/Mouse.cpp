#include "lwjgl/Mouse.h"

#include <queue>

#include "lwjgl/GLContext.h"
#include "lwjgl/Display.h"

#include "external/SDLException.h"

#include "external/SDLCompat.h"

namespace lwjgl
{
namespace Mouse
{

static int_t staging_dx = 0;
static int_t staging_dy = 0;
static int_t staging_dz = 0;

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

void pushEvent(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_MOUSEMOTION:
			staging_dx += e.motion.xrel;
			staging_dy -= e.motion.yrel;
			event_queue.emplace(-1, 0, e.motion.x, Display::getHeight() - e.motion.y - 1, e.motion.xrel, -e.motion.yrel, 0);
			break;
		case SDL_MOUSEBUTTONDOWN:
			// SDL 1.2 uses buttons 4/5 for wheel up/down.
			if (e.button.button == 4)
			{
				staging_dz += 1;
				event_queue.emplace(-1, 0, e.button.x, Display::getHeight() - e.button.y - 1, 0, 0, 1);
			}
			else if (e.button.button == 5)
			{
				staging_dz -= 1;
				event_queue.emplace(-1, 0, e.button.x, Display::getHeight() - e.button.y - 1, 0, 0, -1);
			}
			else
			{
				event_queue.emplace(sdlToLwjglButton(e.button.button), 1, e.button.x, Display::getHeight() - e.button.y - 1, 0, 0, 0);
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (e.button.button != 4 && e.button.button != 5)
				event_queue.emplace(sdlToLwjglButton(e.button.button), 0, e.button.x, Display::getHeight() - e.button.y - 1, 0, 0, 0);
			break;
	}
}

}

void setCursorPosition(int_t x, int_t y)
{
	SDL_WarpMouse(x, y);
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
	int x;
	SDL_GetMouseState(&x, nullptr);
	return x;
}

int_t getY()
{
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
	return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(lwjglToSdlButton(button));
}

bool isGrabbed()
{
	return SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON;
}

void setGrabbed(bool grabbed)
{
	staging_dx = 0;
	staging_dy = 0;

	if (SDL_ShowCursor(grabbed ? SDL_DISABLE : SDL_ENABLE) < 0)
		throw SDLException();
	SDL_WM_GrabInput(grabbed ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

}
}