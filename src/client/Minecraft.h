#pragma once

#include <string>
#include <memory>

#include "client/Options.h"
#include "client/ProgressRenderer.h"
#include "client/Timer.h"
#include "client/User.h"
#include "client/MouseHandler.h"
#include "client/OpenGLCapabilities.h"

#include "client/gui/Gui.h"
#include "client/gui/Font.h"
#include "client/gui/Screen.h"

#include "client/renderer/LevelRenderer.h"
#include "client/renderer/GameRenderer.h"
#include "client/renderer/Textures.h"
#include "client/particle/ParticleEngine.h"

#include "client/skins/TexturePackRepository.h"

#include "client/sound/SoundEngine.h"

#include "client/gamemode/GameMode.h"

#include "client/player/LocalPlayer.h"

#include "world/level/Level.h"

#include "world/phys/HitResult.h"

#include "java/Type.h"
#include "java/System.h"
#include "java/File.h"

#include "util/Memory.h"

class ControllerInput; // Forward declaration

class Minecraft
{
public:
	static constexpr bool FLYBY_MODE = false;
	static const jstring VERSION_STRING;

	std::shared_ptr<GameMode> gameMode;

private:
	bool fullscreen = false;
	
public:
	int_t width = 0;
	int_t height = 0;

private:
	OpenGLCapabilities openGLCapabilities;

	Timer timer = Timer(20.0f);

public:
	std::shared_ptr<Level> level;
	LevelRenderer levelRenderer = LevelRenderer(*this, textures);

	std::shared_ptr<LocalPlayer> player;

	std::unique_ptr<User> user;
	jstring serverDomain;

	volatile bool pause = false;

	Textures textures = Textures(texturePackRepository, options);
	std::unique_ptr<Font> font;

	std::shared_ptr<Screen> screen;
	std::shared_ptr<ProgressRenderer> progressRenderer = Util::make_unique<ProgressRenderer>(*this);

	GameRenderer gameRenderer = GameRenderer(*this);

	ParticleEngine particleEngine = ParticleEngine(nullptr, &textures);

private:
	int_t ticks = 0;
	int_t missTime = 0;

	int_t orgWidth = 0;
	int_t orgHeight = 0;

public:
	Gui gui = Gui(*this);

	bool noRender = false;

	HitResult hitResult = HitResult();

	Options options = Options(*this);

	MouseHandler mouseHandler = MouseHandler(*this);
	TexturePackRepository texturePackRepository = TexturePackRepository(*this);

	SoundEngine soundEngine = SoundEngine();

	std::shared_ptr<File> workingDirectory;

	static std::array<long, 512> frameTimes;
	static std::array<long, 512> tickTimes;
	static int_t frameTimePos;

private:
	static std::shared_ptr<File> workDir;

public:
	volatile bool running = true;

public:
	jstring fpsString = u"";
	
private:
	bool wasDown = false;
	long_t lastTimer = -1;

public:
	bool mouseGrabbed = false;
	
	// Controller virtual mouse cursor (Controlify-style)
	float controllerCursorX = 0.0f;
	float controllerCursorY = 0.0f;
	bool controllerInputActive = false; // Track if controller is currently being used for input
	bool controllerCursorVisible = false;
	float controllerCursorSpeed = 8.0f; // Pixels per frame movement speed
	int_t controllerCursorTexture = -1; // Texture ID for cursor sprite
	bool screenOpenForController = false; // Cached screen state for controller input
	float prevCursorStickX = 0.0f; // Previous frame stick input for snap detection
	float prevCursorStickY = 0.0f;
	bool prevDpadUp = false; // Previous frame D-pad state for button navigation
	bool prevDpadDown = false;
	bool prevDpadLeft = false;
	bool prevDpadRight = false;
	
	// Left stick navigation state (for repeat delay like Controlify)
	bool prevStickUp = false;
	bool prevStickDown = false;
	bool prevStickLeft = false;
	bool prevStickRight = false;
	int_t stickNavRepeatDelay = 0; // Tick counter for stick navigation repeat delay
	const int_t STICK_NAV_INITIAL_DELAY = 10; // Initial delay: 10 ticks (0.5s at 20 TPS)
	const int_t STICK_NAV_REPEAT_DELAY = 3; // Repeat delay: 3 ticks (0.15s at 20 TPS)
	
	// Slider adjustment state (for repeat delay like Controlify)
	int_t sliderAdjustRepeatDelay = 0; // Tick counter for slider adjustment repeat delay
	const int_t SLIDER_ADJUST_INITIAL_DELAY = 15; // Initial delay: 15 ticks (Controlify uses 15)
	const int_t SLIDER_ADJUST_REPEAT_DELAY = 1; // Repeat delay: 1 tick (Controlify uses 1)
	bool prevSliderDpadLeft = false;
	bool prevSliderDpadRight = false;

private:
	int_t lastClickTick = 0;
	
public:
	bool isRaining = false;

private:
	long_t lastTickTime = System::currentTimeMillis();
	int_t recheckPlayerIn = 0;

public:
	Minecraft(int_t width, int_t height, bool fullscreen);

	void onCrash(const std::string &msg, const std::exception &e);

	void init();
	
private:
	void renderLoadingScreen();
	void blit(int_t dstx, int_t dsty, int_t srcx, int_t srcy, int_t w, int_t h);
	
	// Beta: loadAllSounds() - recursively loads all sounds from resource directory (BackgroundDownloader.java:66-80)
	void loadAllSounds(File *dir, const jstring &prefix);

public:
	static const std::shared_ptr<File> &getWorkingDirectory();

	void setScreen(std::shared_ptr<Screen> screen);
	
	// Controller GUI navigation (Controlify-style)
	void updateControllerCursor(ControllerInput* controllerInput);
	void handleControllerGUIClick(ControllerInput* controllerInput);
	void renderControllerCursor(int_t xm, int_t ym, float a);

private:
	void checkGlError(const std::string &at);

public:
	~Minecraft();

	void generateFlyby();

	void run();

	void renderFpsMeter(long tickNanos);

	void stop();
	
	void grabMouse();
	void releaseMouse();
	
	void pauseGame();

	void handleMouseDown(int_t button, bool down);
	void handleMouseClick(int_t button);
	
	// Controller-specific functions (1:1 copies of mouse behavior)
	void handleControllerTriggerClick(int_t button); // button: 0 = right trigger (attack), 1 = left trigger (use)
	void handleControllerTriggerDown(int_t button, bool down); // button: 0 = right trigger (attack)

	void toggleFullscreen();
	void resize(int_t w, int_t h);

	void handleGrabTexture();

	void tick();

	void reloadSound();

	// Beta: fileDownloaded(String name, File file) - categorizes and loads sounds (Minecraft.java:1352-1367)
	void fileDownloaded(const jstring &name, File *file);

	bool isOnline();

	void selectLevel(const jstring &name);

	void toggleDimension();

	void setLevel(std::shared_ptr<Level> level);
	void setLevel(std::shared_ptr<Level> level, const jstring &title);
	void setLevel(std::shared_ptr<Level> level, const jstring &title, std::shared_ptr<Player> player);

	void prepareLevel(const jstring &title);

	jstring gatherStats1();
	jstring gatherStats2();
	jstring gatherStats4();
	jstring gatherStats3();
	void respawnPlayer();
	static void start(const jstring *name, const jstring *sessionId);
	static void startAndConnectTo(const jstring *name, const jstring *sessionId, const jstring *ip);
	// public ClientConnection getConnection()

	friend int main(int argc, char *argv[]);

	enum class OS
	{
		linux,
		solaris,
		windows,
		macos,
		unknown,
	};
};
