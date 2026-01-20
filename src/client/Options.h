#pragma once

#include <string>
#include <array>

#include "java/Type.h"
#include "java/File.h"

#include "lwjgl/Keyboard.h"

class Minecraft;

class KeyMapping
{
public:
	jstring name;
	int_t key;

	KeyMapping(const jstring &name, int_t key)
	{
		this->name = name;
		this->key = key;
	}
};

class Options
{
public:
	struct Option
	{
	private:
		Option() = delete;

	public:
		struct Element
		{
			bool isProgress;
			bool isBoolean;
			const char16_t *captionId;
		};

		friend class Options;

	public:
		static Element MUSIC;
		static Element SOUND;
		static Element INVERT_MOUSE;
		static Element SENSITIVITY;
		static Element RENDER_DISTANCE;
		static Element VIEW_BOBBING;
		static Element ANAGLYPH;
		static Element LIMIT_FRAMERATE;
		static Element DIFFICULTY;
		static Element GRAPHICS;
		static Element FOV;
		static Element GUI_SCALE;

		static constexpr Element *values[12] = {
			&MUSIC, &SOUND, &INVERT_MOUSE, &SENSITIVITY, &RENDER_DISTANCE, &VIEW_BOBBING, &ANAGLYPH, &LIMIT_FRAMERATE, &DIFFICULTY, &GRAPHICS, &FOV, &GUI_SCALE
		};
	};

private:
	static const char16_t *RENDER_DISTANCE_NAMES[4];
	static const char16_t *DIFFICULTY_NAMES[4];

public:
	float music = 1.0f;
	float sound = 1.0f;

	float mouseSensitivity = 0.5f;
	bool invertYMouse = false;

	int_t viewDistance = 0;
	bool bobView = true;
	bool anaglyph3d = false;
	bool limitFramerate = false;
	bool fancyGraphics = true;
	float fovSetting = 0.0f;
	int_t guiScale = 0;

	jstring skin = u"Default";
	jstring username = u"Player";  // Username setting for Player name

	KeyMapping keyUp = KeyMapping(u"key.forward", lwjgl::Keyboard::KEY_W);
	KeyMapping keyLeft = KeyMapping(u"key.left", lwjgl::Keyboard::KEY_A);
	KeyMapping keyDown = KeyMapping(u"key.back", lwjgl::Keyboard::KEY_S);
	KeyMapping keyRight = KeyMapping(u"key.right", lwjgl::Keyboard::KEY_D);
	KeyMapping keyJump = KeyMapping(u"key.jump", lwjgl::Keyboard::KEY_SPACE);
	KeyMapping keyFog = KeyMapping(u"key.fog", lwjgl::Keyboard::KEY_F);
	KeyMapping keySneak = KeyMapping(u"key.sneak", lwjgl::Keyboard::KEY_LSHIFT);
	KeyMapping keyDrop = KeyMapping(u"key.drop", lwjgl::Keyboard::KEY_Q);
	KeyMapping keyInventory = KeyMapping(u"key.inventory", lwjgl::Keyboard::KEY_I);
	KeyMapping keyChat = KeyMapping(u"key.chat", lwjgl::Keyboard::KEY_T);

	std::array<KeyMapping*, 10> keyMappings = { &keyUp, &keyLeft, &keyDown, &keyRight, &keyJump, &keySneak, &keyDrop, &keyInventory, &keyChat, &keyFog };

protected:
	Minecraft &minecraft;

private:
	std::unique_ptr<File> optionsFile;

public:
	int_t difficulty = 2;

	int_t thirdPersonView = 0;  // Alpha: thirdPersonView - 0=first person, 1=back, 2=front (GameSettings.java:43)
	bool showDebugInfo = false;  // Alpha: showDebugInfo - debug screen toggle (GameSettings.java:49)

	jstring lastMpIp = u"";

	// Controller calibration (Controlify-style)
	// Calibrated deadzones for left and right sticks
	// These are calculated during calibration: maxAbs + 0.08f
	float calibratedLeftStickDeadzone = 0.0f;   // 0.0f = not calibrated, use default
	float calibratedRightStickDeadzone = 0.0f; // 0.0f = not calibrated, use default
	bool deadzonesCalibrated = false;          // Whether calibration has been performed
	
	// Controller look sensitivity (Controlify defaults: hLookSensitivity = 1f, vLookSensitivity = 0.9f)
	float controllerHorizontalLookSensitivity = 1.0f;  // Horizontal look sensitivity (0.1-2.0)
	float controllerVerticalLookSensitivity = 0.9f;      // Vertical look sensitivity (0.1-2.0)

	Options(Minecraft &minecraft);
	void open(File *optionsFile);

	jstring getKeyDescription(int_t i);
	jstring getKeyMessage(int_t i);

	void setKey(int_t i, int_t key);
	void set(Option::Element &option, float value);
	void toggle(Option::Element &option, int_t add);

	float getProgressValue(Option::Element &option);
	bool getBooleanValue(Option::Element &option);

	jstring getMessage(Option::Element &option);

	void load();

private:
	float readFloat(const std::string &s);

public:
	void save();
};
