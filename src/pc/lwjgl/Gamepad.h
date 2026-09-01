#pragma once

#include "java/Type.h"

#include "SDL_events.h"


namespace lwjgl
{
namespace Gamepad
{

/*
 * LCE positions preserves the Xbox Legacy Console Edition physical layout:
 *
 *   south = jump, east = drop/cancel, west = crafting, north = inventory.
 *
 * On a Nintendo-labelled controller, SDL2's positional face-button names mean
 * south is the physical B button and east is the physical A button.
 */
enum class FaceButtonLayout
{
    LCEPositions,
    NintendoAB
};

void initialize();
void shutdown();

/* Called by the SDL platform event pump. */
void handleEvent(const SDL_Event &event);
void poll();

/* Minecraft calls this whenever its current Screen changes. */
void setMenuMode(bool enabled);

bool isConnected();
const char *getName();

void setFaceButtonLayout(FaceButtonLayout layout);
FaceButtonLayout getFaceButtonLayout();

/*
 * Values use Minecraft's existing KeyboardInput conventions:
 *   moveLeftRight > 0 means strafe left
 *   moveForward > 0 means move forward
 */
float getMoveLeftRight();
float getMoveForward();

bool isSneaking();
int_t consumeHotbarDirection();

}
}
