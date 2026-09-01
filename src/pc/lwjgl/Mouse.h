#pragma once

#include "java/Type.h"

#include "SDL_events.h"

namespace lwjgl
{
namespace Mouse
{
namespace detail
{

void pushEvent(const SDL_Event &e);
void addSyntheticRelativeMotion(int_t dx, int_t dy);
void setSyntheticButtonState(int_t button, bool down);
void setSyntheticPointerActive(bool active);
void setSyntheticPointerPosition(int_t x, int_t y);
void moveSyntheticPointer(int_t dx, int_t dy);

}

void setCursorPosition(int_t x, int_t y);

// Event handling
bool next();

int_t getEventButton();
bool getEventButtonState();

int_t getEventDX();
int_t getEventDY();

int_t getEventX();
int_t getEventY();

int_t getEventDWheel();

// State
int_t getX();
int_t getY();

int_t getDX();
int_t getDY();

int_t getDWheel();

bool isButtonDown(int_t button);

bool isGrabbed();
void setGrabbed(bool grabbed);

}
}
