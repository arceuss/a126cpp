#include "lwjgl/Gamepad.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "lwjgl/Display.h"
#include "lwjgl/Keyboard.h"
#include "lwjgl/Mouse.h"

#include "SDL.h"


namespace lwjgl
{
namespace Gamepad
{

// File-scope state and helpers use explicit internal linkage; this project
// does not use anonymous namespaces.
static const float STICK_DEAD_ZONE = 0.18f;
static const float TRIGGER_THRESHOLD = 0.35f;
static const float LOOK_PIXELS_PER_SECOND = 760.0f;
static const float MENU_POINTER_PIXELS_PER_SECOND = 540.0f;
static const double MAX_FRAME_SECONDS = 0.050;

static SDL_GameController *controller = nullptr;
static SDL_JoystickID controllerInstance = -1;

static FaceButtonLayout faceLayout = FaceButtonLayout::LCEPositions;
static bool menuMode = false;
static bool sneakLatched = false;

static float moveLeftRight = 0.0f;
static float moveForward = 0.0f;

static double lookRemainderX = 0.0;
static double lookRemainderY = 0.0;
static double pointerRemainderX = 0.0;
static double pointerRemainderY = 0.0;

static Uint64 previousCounter = 0;

static std::array<bool, SDL_CONTROLLER_BUTTON_MAX> previousButtons = {};

static bool jumpInjected = false;
static bool attackInjected = false;
static bool useInjected = false;
static bool menuClickInjected = false;
static bool suppressMenuClickUntilRelease = false;

static int_t pendingHotbarDirection = 0;


static float axisToFloat(Sint16 value)
{
    if(value >= 0)
        return static_cast<float>(value) / 32767.0f;
    return static_cast<float>(value) / 32768.0f;
}

static void applyRadialDeadZone(Sint16 rawX, Sint16 rawY, float &x, float &y)
{
    const float inputX = axisToFloat(rawX);
    const float inputY = axisToFloat(rawY);
    const float magnitude = std::sqrt(inputX * inputX + inputY * inputY);

    if(magnitude <= STICK_DEAD_ZONE || magnitude <= 0.0f)
    {
        x = 0.0f;
        y = 0.0f;
        return;
    }

    const float scaledMagnitude =
        std::min(1.0f, (magnitude - STICK_DEAD_ZONE) /
                       (1.0f - STICK_DEAD_ZONE));

    x = inputX / magnitude * scaledMagnitude;
    y = inputY / magnitude * scaledMagnitude;
}

static float squareCurve(float value)
{
    return std::copysign(value * value, value);
}

static bool buttonDown(SDL_GameControllerButton button)
{
    return controller != nullptr &&
           SDL_GameControllerGetButton(controller, button) != 0;
}

static bool buttonPressed(SDL_GameControllerButton button)
{
    const std::size_t index = static_cast<std::size_t>(button);
    return buttonDown(button) && !previousButtons[index];
}

static void snapshotButtons()
{
    std::size_t i;

    for(i = 0;i < previousButtons.size();i++)
    {
        previousButtons[i] =
            controller != nullptr &&
            SDL_GameControllerGetButton(
                controller,
                static_cast<SDL_GameControllerButton>(i)) != 0;
    }
}

SDL_GameControllerButton jumpButton()
{
    return faceLayout == FaceButtonLayout::LCEPositions
        ? SDL_CONTROLLER_BUTTON_A   /* south; physical B on Switch */
        : SDL_CONTROLLER_BUTTON_B;  /* east; physical A on Switch */
}

SDL_GameControllerButton dropButton()
{
    return faceLayout == FaceButtonLayout::LCEPositions
        ? SDL_CONTROLLER_BUTTON_B
        : SDL_CONTROLLER_BUTTON_A;
}

static void injectJump(bool down)
{
    if(jumpInjected == down)
        return;

    jumpInjected = down;
    Keyboard::detail::setSyntheticKeyState(Keyboard::KEY_SPACE, down);
}

static void injectAttack(bool down)
{
    if(attackInjected == down)
        return;

    attackInjected = down;
    Mouse::detail::setSyntheticButtonState(0, down);
}

static void injectUse(bool down)
{
    if(useInjected == down)
        return;

    useInjected = down;
    Mouse::detail::setSyntheticButtonState(1, down);
}

static void injectMenuClick(bool down)
{
    if(menuClickInjected == down)
        return;

    menuClickInjected = down;
    Mouse::detail::setSyntheticButtonState(0, down);
}

static void releaseInjectedGameplayControls()
{
    injectJump(false);
    injectAttack(false);
    injectUse(false);

    moveLeftRight = 0.0f;
    moveForward = 0.0f;
    lookRemainderX = 0.0;
    lookRemainderY = 0.0;
}

static void releaseAllInjectedControls()
{
    releaseInjectedGameplayControls();
    injectMenuClick(false);

    /*
     * Hand the pointer back to the real mouse. Without this, a screen opened
     * while no pad is attached would leave Mouse::getX/getY reporting the
     * frozen synthetic position, which breaks button hover and sliders.
     */
    Mouse::detail::setSyntheticPointerActive(false);
}

static bool openControllerIndex(int index)
{
    SDL_GameController *opened;
    SDL_Joystick *joystick;

#ifndef __SWITCH__
    /*
     * Desktop keeps mouse and keyboard authority. Adopting a pad would take
     * over menu input and override the real cursor position the moment one
     * happened to be plugged in. The console port is what needs this layer,
     * so no controller is ever opened elsewhere and every accessor below
     * stays inert.
     */
    (void)index;
    return false;
#endif

    if(controller != nullptr || !SDL_IsGameController(index))
        return false;


    opened = SDL_GameControllerOpen(index);
    if(opened == nullptr)
        return false;

    joystick = SDL_GameControllerGetJoystick(opened);
    if(joystick == nullptr)
    {
        SDL_GameControllerClose(opened);
        return false;
    }

    controller = opened;
    controllerInstance = SDL_JoystickInstanceID(joystick);
    snapshotButtons();
    previousCounter = SDL_GetPerformanceCounter();

    return true;
}

static void findController()
{
    int index;

    if(controller != nullptr)
        return;

    for(index = 0;index < SDL_NumJoysticks();index++)
    {
        if(openControllerIndex(index))
            return;
    }
}

static void closeController()
{
    releaseAllInjectedControls();

    if(controller != nullptr)
        SDL_GameControllerClose(controller);

    controller = nullptr;
    controllerInstance = -1;
    previousButtons.fill(false);
    sneakLatched = false;
    suppressMenuClickUntilRelease = false;
    pendingHotbarDirection = 0;
}

static double frameSeconds()
{
    const Uint64 counter = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    double seconds = 1.0 / 60.0;

    if(previousCounter != 0 && frequency != 0 && counter >= previousCounter)
        seconds = static_cast<double>(counter - previousCounter) /
                  static_cast<double>(frequency);

    previousCounter = counter;

    if(seconds < 0.0)
        seconds = 0.0;
    if(seconds > MAX_FRAME_SECONDS)
        seconds = MAX_FRAME_SECONDS;

    return seconds;
}

static int takeWholePixels(double &value)
{
    const int whole = static_cast<int>(value);
    value -= whole;
    return whole;
}

static void pollMenu(double seconds)
{
    float x = 0.0f;
    float y = 0.0f;

    releaseInjectedGameplayControls();
    Mouse::detail::setSyntheticPointerActive(true);

    applyRadialDeadZone(
        SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX),
        SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY),
        x, y);

    /*
     * D-pad left/right adjusts the hovered value control instead of nudging
     * the pointer, mirroring how Legacy Console Edition steps a focused
     * slider. The stick still moves the pointer, so nothing else is lost.
     */
    if(buttonPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT))
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_LEFT);
    else if(buttonPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_RIGHT);

    if(buttonDown(SDL_CONTROLLER_BUTTON_DPAD_UP))
        y = -1.0f;
    else if(buttonDown(SDL_CONTROLLER_BUTTON_DPAD_DOWN))
        y = 1.0f;

    pointerRemainderX +=
        static_cast<double>(x) * MENU_POINTER_PIXELS_PER_SECOND * seconds;
    pointerRemainderY +=
        static_cast<double>(y) * MENU_POINTER_PIXELS_PER_SECOND * seconds;

    Mouse::detail::moveSyntheticPointer(
        takeWholePixels(pointerRemainderX),
        takeWholePixels(pointerRemainderY));

    if(suppressMenuClickUntilRelease)
    {
        injectMenuClick(false);
        if(!buttonDown(jumpButton()))
            suppressMenuClickUntilRelease = false;
    }
    else
    {
        injectMenuClick(buttonDown(jumpButton()));
    }

    if(buttonPressed(dropButton()) ||
       buttonPressed(SDL_CONTROLLER_BUTTON_START))
    {
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_ESCAPE);
    }
}

static void pollGameplay(double seconds)
{
    float leftX = 0.0f;
    float leftY = 0.0f;
    float rightX = 0.0f;
    float rightY = 0.0f;
    float leftTrigger;
    float rightTrigger;

    injectMenuClick(false);
    Mouse::detail::setSyntheticPointerActive(false);

    applyRadialDeadZone(
        SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX),
        SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY),
        leftX, leftY);

    applyRadialDeadZone(
        SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX),
        SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY),
        rightX, rightY);

    /*
     * KeyboardInput uses positive X for left and positive Y for forward.
     * SDL axes use positive X for right and positive Y for down/back.
     */
    moveLeftRight = -leftX;
    moveForward = -leftY;

    rightX = squareCurve(rightX);
    rightY = squareCurve(-rightY);

    lookRemainderX +=
        static_cast<double>(rightX) * LOOK_PIXELS_PER_SECOND * seconds;
    lookRemainderY +=
        static_cast<double>(rightY) * LOOK_PIXELS_PER_SECOND * seconds;

    Mouse::detail::addSyntheticRelativeMotion(
        takeWholePixels(lookRemainderX),
        takeWholePixels(lookRemainderY));

    leftTrigger = std::max(
        0.0f,
        axisToFloat(SDL_GameControllerGetAxis(
            controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT)));
    rightTrigger = std::max(
        0.0f,
        axisToFloat(SDL_GameControllerGetAxis(
            controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)));

    /* LCE default: LT use/place, RT attack/mine. */
    injectUse(leftTrigger >= TRIGGER_THRESHOLD);
    injectAttack(rightTrigger >= TRIGGER_THRESHOLD);

    injectJump(buttonDown(jumpButton()));

    if(buttonPressed(dropButton()))
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_Q);

    /*
     * LCE uses west for crafting and north for inventory. Alpha's inventory
     * screen already contains the 2x2 crafting grid, so both open it.
     */
    if(buttonPressed(SDL_CONTROLLER_BUTTON_X) ||
       buttonPressed(SDL_CONTROLLER_BUTTON_Y))
    {
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_I);
    }

    if(buttonPressed(SDL_CONTROLLER_BUTTON_START))
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_ESCAPE);

    if(buttonPressed(SDL_CONTROLLER_BUTTON_BACK))
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_F3);

    if(buttonPressed(SDL_CONTROLLER_BUTTON_LEFTSTICK))
        Keyboard::detail::pulseSyntheticKey(Keyboard::KEY_F5);

    if(buttonPressed(SDL_CONTROLLER_BUTTON_RIGHTSTICK))
        sneakLatched = !sneakLatched;

    /* InventoryPlayer::changeCurrentItem uses +1 for previous and -1 for next. */
    if(buttonPressed(SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ||
       buttonPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT))
    {
        pendingHotbarDirection = std::min(8, pendingHotbarDirection + 1);
    }

    if(buttonPressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ||
       buttonPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
    {
        pendingHotbarDirection = std::max(-8, pendingHotbarDirection - 1);
    }
}


void initialize()
{
    SDL_GameControllerEventState(SDL_ENABLE);
    findController();

    if(controller == nullptr)
        previousCounter = SDL_GetPerformanceCounter();
}

void shutdown()
{
    closeController();
}

void handleEvent(const SDL_Event &event)
{
    switch(event.type)
    {
        case SDL_CONTROLLERDEVICEADDED:
            if(controller == nullptr)
                (void)openControllerIndex(event.cdevice.which);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            if(controller != nullptr &&
               event.cdevice.which == controllerInstance)
            {
                closeController();
                findController();
            }
            break;

        case SDL_CONTROLLERDEVICEREMAPPED:
            if(controller != nullptr &&
               event.cdevice.which == controllerInstance)
            {
                snapshotButtons();
            }
            break;

        default:
            break;
    }
}

void poll()
{
    const double seconds = frameSeconds();

    if(controller == nullptr)
    {
        findController();
        if(controller == nullptr)
        {
            releaseAllInjectedControls();
            return;
        }
    }

    if(menuMode)
        pollMenu(seconds);
    else
        pollGameplay(seconds);

    snapshotButtons();
}

void setMenuMode(bool enabled)
{
    if(menuMode == enabled)
        return;

    releaseAllInjectedControls();
    menuMode = enabled;
    sneakLatched = false;
    pendingHotbarDirection = 0;
    suppressMenuClickUntilRelease =
        menuMode && controller != nullptr && buttonDown(jumpButton());
    pointerRemainderX = 0.0;
    pointerRemainderY = 0.0;

    /*
     * Only take over the pointer when a pad is actually driving. A desktop
     * player with no controller must keep the real mouse.
     */
    const bool pointerDriven = menuMode && controller != nullptr;
    if(pointerDriven)
    {
        Mouse::detail::setSyntheticPointerPosition(
            std::max(0, Display::getWidth() / 2),
            std::max(0, Display::getHeight() / 2));
    }
    Mouse::detail::setSyntheticPointerActive(pointerDriven);

    snapshotButtons();
}

bool isConnected()
{
    return controller != nullptr;
}

const char *getName()
{
    if(controller == nullptr)
        return "No controller";

    const char *name = SDL_GameControllerName(controller);
    return name != nullptr ? name : "SDL controller";
}

void setFaceButtonLayout(FaceButtonLayout layout)
{
    if(faceLayout == layout)
        return;

    releaseAllInjectedControls();
    faceLayout = layout;
    snapshotButtons();
}

FaceButtonLayout getFaceButtonLayout()
{
    return faceLayout;
}

float getMoveLeftRight()
{
    return menuMode ? 0.0f : moveLeftRight;
}

float getMoveForward()
{
    return menuMode ? 0.0f : moveForward;
}

bool isSneaking()
{
    return !menuMode && sneakLatched;
}

int_t consumeHotbarDirection()
{
    const int_t direction = pendingHotbarDirection;
    pendingHotbarDirection = 0;
    return direction;
}

}
}
