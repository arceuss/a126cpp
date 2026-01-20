#include "client/Options.h"

#include "client/Minecraft.h"
#include "client/locale/Language.h"

#include "lwjgl/Keyboard.h"
#include "lwjgl/GLContext.h"
#include <SDL3/SDL.h>

constexpr Options::Option::Element *Options::Option::values[];

Options::Option::Element Options::Option::MUSIC = { true, false, u"options.music" };
Options::Option::Element Options::Option::SOUND = { true, false, u"options.sound" };
Options::Option::Element Options::Option::INVERT_MOUSE = { false, true, u"options.invertMouse" };
Options::Option::Element Options::Option::SENSITIVITY = { true, false, u"options.sensitivity" };
Options::Option::Element Options::Option::RENDER_DISTANCE = { false, false, u"options.renderDistance" };
Options::Option::Element Options::Option::VIEW_BOBBING = { false, true, u"options.viewBobbing" };
Options::Option::Element Options::Option::ANAGLYPH = { false, true, u"options.anaglyph" };
Options::Option::Element Options::Option::LIMIT_FRAMERATE = { true, false, u"" }; // Hardcoded string in getMessage()
Options::Option::Element Options::Option::DIFFICULTY = { false, false, u"options.difficulty" };
Options::Option::Element Options::Option::GRAPHICS = { false, false, u"options.graphics" };
Options::Option::Element Options::Option::FOV = { true, false, u"options.fov" };
Options::Option::Element Options::Option::GUI_SCALE = { false, false, u"options.guiScale" };

const char16_t *Options::RENDER_DISTANCE_NAMES[] = {
	u"options.renderDistance.far",
	u"options.renderDistance.normal",
	u"options.renderDistance.short",
	u"options.renderDistance.tiny"
};
const char16_t *Options::DIFFICULTY_NAMES[] = {
	u"options.difficulty.peaceful",
	u"options.difficulty.easy",
	u"options.difficulty.normal",
	u"options.difficulty.hard"
};

Options::Options(Minecraft &minecraft) : minecraft(minecraft)
{
	// VSync is disabled by default (set in GLContext.cpp), but we'll apply it when options are loaded
}

void Options::open(File *optionsFile)
{
	this->optionsFile.reset(File::open(*optionsFile, u"options.txt"));
	load();
	// Apply VSync setting after loading
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
	save();
}

jstring Options::getKeyDescription(int_t i)
{
	Language &l = Language::getInstance();
	return l.getElement(keyMappings[i]->name);
}

jstring Options::getKeyMessage(int_t i)
{
	return lwjgl::Keyboard::getKeyName(keyMappings[i]->key);
}

void Options::setKey(int_t i, int_t key)
{
	keyMappings[i]->key = key;
	save();
}

void Options::set(Option::Element &option, float value)
{
	if (&option == &Option::MUSIC)
		music = value;
	else if (&option == &Option::SOUND)
		sound = value;
	else if (&option == &Option::SENSITIVITY)
		mouseSensitivity = value;
	else if (&option == &Option::FOV)
		fovSetting = value;
	else if (&option == &Option::LIMIT_FRAMERATE)
	{
		// Map slider value (0.0-1.0) to fps
		// Slider value 0.0 = VSync (display refresh rate, typically 60 fps)
		// Slider value 0.1-1.0 = 10-260 fps (unlimited)
		// Get display refresh rate for VSync
		int_t displayRefreshRate = 60; // Default to 60 Hz
		SDL_Window* window = lwjgl::GLContext::detail::getWindow();
		if (window != nullptr)
		{
			SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
			if (display_id != 0)
			{
				const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display_id);
				if (mode != nullptr && mode->refresh_rate > 0.0f)
				{
					displayRefreshRate = static_cast<int_t>(mode->refresh_rate);
				}
			}
		}
		
		if (value <= 0.001f) // At minimum (0.0) = VSync
		{
			// Enable VSync and set framerate to display refresh rate
			vsync = true;
			limitFramerate = displayRefreshRate;
			SDL_GL_SetSwapInterval(1);
		}
		else
		{
			// Disable VSync and use framerate limit
			vsync = false;
			SDL_GL_SetSwapInterval(0);
			// Map slider value (0.1-1.0) to fps (10-260)
			// Map: fps = 10 + (value - 0.1) * (250 / 0.9)
			float adjustedValue = (value - 0.1f) / 0.9f; // Map 0.1-1.0 to 0.0-1.0
			int_t fps = static_cast<int_t>(10.0f + adjustedValue * 250.0f);
			// Round to nearest 10
			limitFramerate = ((fps + 5) / 10) * 10;
			// Clamp to valid range
			if (limitFramerate < 10) limitFramerate = 10;
			if (limitFramerate > 260) limitFramerate = 260;
		}
	}
}

void Options::toggle(Option::Element &option, int_t add)
{
	if (&option == &Option::INVERT_MOUSE)
		invertYMouse = !invertYMouse;
	else if (&option == &Option::RENDER_DISTANCE)
		viewDistance = (viewDistance + add) & 3;
	else if (&option == &Option::VIEW_BOBBING)
		bobView = !bobView;
	else if (&option == &Option::ANAGLYPH)
	{
		anaglyph3d = !anaglyph3d;
		minecraft.textures.reloadAll();
	}
	// LIMIT_FRAMERATE is now a slider, handled in set()
	else if (&option == &Option::DIFFICULTY)
		difficulty = (difficulty + add) & 3;
	else if (&option == &Option::GRAPHICS)
	{
		fancyGraphics = !fancyGraphics;
		minecraft.levelRenderer.allChanged();
	}
	else if (&option == &Option::GUI_SCALE)
		guiScale = (guiScale + add) & 3;

	save();
}

float Options::getProgressValue(Option::Element &option)
{
	if (&option == &Option::MUSIC)
		return music;
	else if (&option == &Option::SOUND)
		return sound;
	else if (&option == &Option::SENSITIVITY)
		return mouseSensitivity;
	else if (&option == &Option::FOV)
		return fovSetting;
	else if (&option == &Option::LIMIT_FRAMERATE)
	{
		// Map fps to slider value (0.0-1.0)
		// If VSync is enabled, return 0.0 (minimum = VSync)
		// Otherwise, map fps (10-260) to slider value (0.1-1.0)
		if (vsync)
		{
			return 0.0f; // VSync = minimum slider position
		}
		else
		{
			// Map fps (10-260) to slider value (0.1-1.0)
			// Map: value = 0.1 + (fps - 10) / 250.0 * 0.9
			float normalized = static_cast<float>(limitFramerate - 10) / 250.0f;
			return 0.1f + normalized * 0.9f;
		}
	}
	return 0.0f;
}

bool Options::getBooleanValue(Option::Element &option)
{
	if (&option == &Option::INVERT_MOUSE)
		return invertYMouse;
	else if (&option == &Option::VIEW_BOBBING)
		return bobView;
	else if (&option == &Option::ANAGLYPH)
		return anaglyph3d;
	else if (&option == &Option::LIMIT_FRAMERATE)
		return limitFramerate;
	return false;
}

jstring Options::getMessage(Option::Element &option)
{
	Language &l = Language::getInstance();
	
	// Handle FOV first (special case - it's progress but has custom display)
	if (&option == &Option::FOV)
	{
		if (fovSetting == 0.0f)
			return u"FOV: Normal";
		else if (fovSetting == 1.0f)
			return u"FOV: Quake Pro";
		else
		{
			int_t fovValue = static_cast<int_t>(70.0f + fovSetting * 40.0f);
			return u"FOV: " + String::fromUTF8(std::to_string(fovValue));
		}
	}
	
	// Handle LIMIT_FRAMERATE with hardcoded string
	if (&option == &Option::LIMIT_FRAMERATE)
	{
		jstring result = u"Max Framerate: ";
		if (vsync)
			return result + u"VSync";
		else if (limitFramerate >= 260)
			return result + u"Unlimited";
		else
			return result + String::fromUTF8(std::to_string(limitFramerate)) + u" fps";
	}
	
	jstring result = l.getElement(option.captionId) + u": ";

	if (option.isProgress)
	{
		float progress = getProgressValue(option);
		if (&option == &Option::SENSITIVITY)
		{
			if (progress == 0.0f)
				return result + l.getElement(u"options.sensitivity.min");
			else if (progress == 1.0f)
				return result + l.getElement(u"options.sensitivity.max");
			else
				return result + String::fromUTF8(std::to_string(static_cast<int_t>(progress * 200.0f))) + u'%';
		}
		else
		{
			if (progress == 0.0f)
				return result + l.getElement(u"options.off");
			else
				return result + String::fromUTF8(std::to_string(static_cast<int_t>(progress * 100.0f))) + u'%';
		}
	}
	else if (option.isBoolean)
	{
		bool value = getBooleanValue(option);
		if (value)
			return result + l.getElement(u"options.on");
		else
			return result + l.getElement(u"options.off");
	}
	else if (&option == &Option::RENDER_DISTANCE)
	{
		return result + l.getElement(RENDER_DISTANCE_NAMES[viewDistance]);
	}
	else if (&option == &Option::DIFFICULTY)
	{
		return result + l.getElement(DIFFICULTY_NAMES[difficulty]);
	}
	else if (&option == &Option::GRAPHICS)
	{
		if (fancyGraphics)
			return result + l.getElement(u"options.graphics.fancy");
		else
			return result + l.getElement(u"options.graphics.fast");
	}
	else if (&option == &Option::GUI_SCALE)
	{
		const char16_t *scaleNames[] = { u"Auto", u"Small", u"Normal", u"Large" };
		return u"GUI Scale: " + jstring(scaleNames[guiScale]);
	}

	return result;
}

void Options::load()
{
	if (!optionsFile->exists())
		return;

	std::unique_ptr<std::istream> is(optionsFile->toStreamIn());
	if (!is)
		return;
	
	std::string line;
	while (std::getline(*is, line))
	{
		if (line.back() == '\r')
			line.pop_back();

		size_t pos = line.find(':');
		if (pos == std::string::npos)
			continue;

		std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 1);

		if (key == "music")
			music = readFloat(value);
		if (key == "sound")
			sound = readFloat(value);
		if (key == "mouseSensitivity")
			mouseSensitivity = readFloat(value);
		if (key == "invertYMouse")
			invertYMouse = value == "true";
		if (key == "viewDistance")
			viewDistance = std::stoi(value);
		if (key == "bobView")
			bobView = value == "true";
		if (key == "anaglyph3d")
			anaglyph3d = value == "true";
		if (key == "limitFramerate")
		{
			// Support both old boolean format and new int format
			if (value == "true")
				limitFramerate = 120; // Default to 120 fps for old saves
			else if (value == "false")
				limitFramerate = 260; // Unlimited for old saves
			else
				limitFramerate = std::stoi(value);
			// Clamp to valid range
			if (limitFramerate < 10) limitFramerate = 10;
			if (limitFramerate > 260) limitFramerate = 260;
		}
		if (key == "vsync")
			vsync = value == "true";
		if (key == "difficulty")
			difficulty = std::stoi(value);
		if (key == "fancyGraphics")
			fancyGraphics = value == "true";
		if (key == "fov")
			fovSetting = readFloat(value);
		if (key == "guiScale")
			guiScale = std::stoi(value);
		if (key == "skin")
			skin = String::fromUTF8(value);
		if (key == "username")
			username = String::fromUTF8(value);
		if (key == "lastServer")
			lastMpIp = String::fromUTF8(value);
		if (key == "calibratedLeftStickDeadzone")
			calibratedLeftStickDeadzone = readFloat(value);
		if (key == "calibratedRightStickDeadzone")
			calibratedRightStickDeadzone = readFloat(value);
		if (key == "deadzonesCalibrated")
			deadzonesCalibrated = value == "true";
		if (key == "controllerHorizontalLookSensitivity")
			controllerHorizontalLookSensitivity = readFloat(value);
		if (key == "controllerVerticalLookSensitivity")
			controllerVerticalLookSensitivity = readFloat(value);

		jstring jkey = String::fromUTF8(key);
		for (int_t i = 0; i < keyMappings.size(); i++)
		{
			if (jkey == u"key_" + keyMappings[i]->name)
				keyMappings[i]->key = std::stoi(value);
		}
	}
}

float Options::readFloat(const std::string &s)
{
	if (s == "true")
		return 1.0f;
	else if (s == "false")
		return 0.0f;
	else
		return std::stof(s);
}

void Options::save()
{
	std::unique_ptr<std::ostream> os(optionsFile->toStreamOut());
	if (!os)
		return;

	*os << std::boolalpha;
	*os << "music:" << music << '\n';
	*os << "sound:" << sound << '\n';
	*os << "invertYMouse:" << invertYMouse << '\n';
	*os << "mouseSensitivity:" << mouseSensitivity << '\n';
	*os << "viewDistance:" << viewDistance << '\n';
	*os << "bobView:" << bobView << '\n';
	*os << "anaglyph3d:" << anaglyph3d << '\n';
	*os << "limitFramerate:" << limitFramerate << '\n';
	*os << "vsync:" << (vsync ? "true" : "false") << '\n';
	*os << "difficulty:" << difficulty << '\n';
	*os << "fancyGraphics:" << fancyGraphics << '\n';
	*os << "fov:" << fovSetting << '\n';
	*os << "guiScale:" << guiScale << '\n';
	*os << "skin:" << String::toUTF8(skin) << '\n';
	*os << "username:" << String::toUTF8(username) << '\n';
	*os << "lastServer:" << String::toUTF8(lastMpIp) << '\n';
	*os << "calibratedLeftStickDeadzone:" << calibratedLeftStickDeadzone << '\n';
	*os << "calibratedRightStickDeadzone:" << calibratedRightStickDeadzone << '\n';
	*os << "deadzonesCalibrated:" << deadzonesCalibrated << '\n';
	*os << "controllerHorizontalLookSensitivity:" << controllerHorizontalLookSensitivity << '\n';
	*os << "controllerVerticalLookSensitivity:" << controllerVerticalLookSensitivity << '\n';

	for (int_t i = 0; i < keyMappings.size(); i++)
		*os << "key_" << String::toUTF8(keyMappings[i]->name) << ':' << keyMappings[i]->key << '\n';
}
