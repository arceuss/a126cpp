#include "Minecraft.h"

#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <typeinfo>

#include "SharedConstants.h"

#include "client/renderer/Chunk.h"
#include "client/renderer/Tesselator.h"
#include "client/renderer/TextureWaterFX.h"
#include "client/renderer/TextureWaterSideFX.h"
#include "client/renderer/TextureLavaFX.h"
#include "client/renderer/TextureLavaSideFX.h"
#include "client/renderer/TexturePortalFX.h"
#include "client/renderer/TextureCompassFX.h"
#include "client/renderer/TextureClockFX.h"
#include "client/renderer/TextureFireFX.h"
#include "client/gui/ScreenSizeCalculator.h"
#include "client/gui/PauseScreen.h"
#include "client/gui/InventoryScreen.h"
#include "client/gui/DeathScreen.h"
#include "client/gui/GuiChat.h"
#include "client/gui/SlideButton.h"
#include "client/title/TitleScreen.h"
#include "client/player/KeyboardInput.h"
#include "client/player/ControllerInput.h"
#include "client/player/EntityClientPlayerMP.h"
#include <SDL3/SDL_gamepad.h> // For SDL_GAMEPAD_BUTTON enum

#include "client/gamemode/SurvivalMode.h"

#include "world/phys/Vec3.h"
#include "world/phys/AABB.h"
#include "world/phys/HitResult.h"
#include "world/level/chunk/ChunkCache.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/dimension/HellDimension.h"
#include "world/item/Items.h"
#include "world/item/crafting/FurnaceRecipes.h"
#include "Facing.h"

#include "java/System.h"
#include "java/Runtime.h"
#include "java/File.h"
#include "java/String.h"

#include "util/Mth.h"

#include <fstream>
#include <vector>
#include <utility>

#include "lwjgl/Display.h"
#include "lwjgl/Keyboard.h"
#include "lwjgl/Mouse.h"
#include "lwjgl/Gamepad.h"
#include <algorithm>
#include <cmath>

#include "CrashHandler.h"

const jstring Minecraft::VERSION_STRING = u"Minecraft " + SharedConstants::VERSION_STRING;

std::array<long, 512> Minecraft::frameTimes = {};
std::array<long, 512> Minecraft::tickTimes = {};
int_t Minecraft::frameTimePos = 0;

std::shared_ptr<File> Minecraft::workDir;

Minecraft::Minecraft(int_t width, int_t height, bool fullscreen)
{
	// orgWidth = width;
	orgHeight = height;

	this->width = width;
	this->height = height;
	this->fullscreen = fullscreen;
}

void Minecraft::onCrash(const std::string &message, const std::exception &e)
{
	CrashHandler::Crash(message + ": " + e.what());
}

void Minecraft::init()
{
	Tile::initTiles();
	Items::initItems();  // Alpha: Initialize item registry
	FurnaceRecipes::getInstance().init();  // Initialize furnace recipes after items are ready

	// Setup LWJGL
	if (fullscreen)
	{
		lwjgl::Display::setFullscreen(true);
		width = lwjgl::Display::getDisplayMode().getWidth();
		height = lwjgl::Display::getDisplayMode().getHeight();
		if (width < 1)
			width = 1;
		if (height < 1)
			height = 1;
	}
	else
	{
		lwjgl::Display::setDisplayMode(lwjgl::DisplayMode(width, height));
	}

	lwjgl::Display::setTitle(u"Minecraft Minecraft " + VERSION_STRING);

	lwjgl::Display::create();
	
	// Initialize gamepad subsystem
	lwjgl::Gamepad::init();

	// TODO
	// EntityRenderDispatcher.instance.itemInHandRenderer = new ItemInHandRenderer(this);

	workingDirectory = getWorkingDirectory();

	options.open(workingDirectory.get());
	texturePackRepository.updateListAndSelect();
	
	// Load controller cursor texture (Controlify-style) - must be after texture pack repository is initialized
	// Texture paths use /gui/ format, not /resource/gui/
	controllerCursorTexture = textures.loadTexture(u"/gui/virtual_mouse.png");

	font = Util::make_unique<Font>(options, u"/font/default.png", textures);
	
	// Beta: Render loading screen (Minecraft.java:215)
	renderLoadingScreen();

	// Beta: Initialize sound engine (Minecraft.java:242)
	soundEngine.init(&options);

	// Beta: Load all sounds from resource directory (BackgroundDownloader.java:66-80, Minecraft.java:fileDownloaded)
	// This automatically loads all sounds from sound/, newsound/, streaming/, music/, and newmusic/ directories
	std::unique_ptr<File> resourceDir(File::openResourceDirectory());
	if (resourceDir && resourceDir->exists() && resourceDir->isDirectory())
		{
		loadAllSounds(resourceDir.get(), u"");
		std::cerr << "Sound loading complete" << std::endl;
		}
		else
		{
		std::cerr << "Warning: Could not open resource directory for sound loading" << std::endl;
		}

	checkGlError("Pre startup");

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glCullFace(GL_BACK);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	checkGlError("Startup");

	// TODO
	// soundEngine

	// Alpha: Register animated textures (Minecraft.startGame() lines 242-250)
	// Beta 1.2: Register all fluid texture FX (Minecraft.java:243-249)
	// Alpha 1.2.6: Register TextureFlamesFX(0) and TextureFlamesFX(1) (Minecraft.java:249-250)
	textures.registerTextureFX(std::make_unique<TextureLavaFX>());      // Stationary lava
	textures.registerTextureFX(std::make_unique<TextureWaterFX>());     // Stationary water
	textures.registerTextureFX(std::make_unique<TextureWaterSideFX>()); // Flowing water
	textures.registerTextureFX(std::make_unique<TextureLavaSideFX>()); // Flowing lava
	textures.registerTextureFX(std::make_unique<TexturePortalFX>());   // Portal animation (Alpha: Minecraft.java:244)
	textures.registerTextureFX(std::make_unique<TextureCompassFX>(this)); // Compass animation (Alpha: Minecraft.java:245)
	textures.registerTextureFX(std::make_unique<TextureClockFX>(this)); // Clock animation (Alpha: Minecraft.java:246 TextureWatchFX)
	textures.registerTextureFX(std::make_unique<TextureFireFX>(0));     // Fire texture 0
	textures.registerTextureFX(std::make_unique<TextureFireFX>(1));     // Fire texture 1

	setScreen(Util::make_shared<TitleScreen>(*this));
}

void Minecraft::renderLoadingScreen()
{
	ScreenSizeCalculator ssc(width, height, options);
	int_t w = ssc.getWidth();
	int_t h = ssc.getHeight();

	// Beta: Exact order of operations matching Minecraft.java:269-303
	// Beta: 16640 = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);  // Beta: 5889
	glLoadIdentity();
	glOrtho(0.0, w, h, 0.0, 1000.0, 3000.0);

	glMatrixMode(GL_MODELVIEW);  // Beta: 5888
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -2000.0f);

	glViewport(0, 0, width, height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	
	Tesselator &tesselator = Tesselator::instance;
	
	glDisable(GL_LIGHTING);  // Beta: 2896
	glEnable(GL_TEXTURE_2D);  // Beta: 3553
	glDisable(GL_FOG);  // Beta: 2912

	glBindTexture(GL_TEXTURE_2D, textures.loadTexture(u"/title/mojang.png"));  // Beta: line 286

	// Beta: Draw mojang.png background (Minecraft.java:287-293)
	// Beta uses (0.0, 0.0) for all UV coordinates - matching exactly
	tesselator.begin();
	tesselator.color(0xFFFFFF);  // Beta: 16777215
	tesselator.vertexUV(0.0, height, 0.0, 0.0, 0.0);  // Beta: line 289
	tesselator.vertexUV(width, height, 0.0, 0.0, 0.0);  // Beta: line 290
	tesselator.vertexUV(width, 0.0, 0.0, 0.0, 0.0);  // Beta: line 291
	tesselator.vertexUV(0.0, 0.0, 0.0, 0.0, 0.0);  // Beta: line 292
	tesselator.end();

	// Beta: Draw Minecraft logo (Minecraft.java:294-298)
	// Note: mojang.png is still bound from line 189, blit() uses it
	short_t gw = 256;  // Beta: var5
	short_t gh = 256;  // Beta: var6
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Beta: line 296
	tesselator.color(0xFFFFFF);  // Beta: line 297
	blit((width / 2 - gw) / 2, (height / 2 - gh) / 2, 0, 0, gw, gh);  // Beta: line 298

	glDisable(GL_LIGHTING);  // Beta: line 299
	glDisable(GL_FOG);  // Beta: line 300
	glEnable(GL_ALPHA_TEST);  // Beta: 3008, line 301
	glAlphaFunc(GL_GREATER, 0.1f);  // Beta: 516, line 302

	lwjgl::Display::swapBuffers();
}

void Minecraft::blit(int_t x, int_t y, int_t sx, int_t sy, int_t w, int_t h)
{
	const float us = 1.0f / 256.0f;
	const float vs = 1.0f / 256.0f;
	Tesselator &t = Tesselator::instance;
	t.begin();
	t.vertexUV(x + 0, y + h, 0.0, (sx + 0) * us, (sy + h) * vs);
	t.vertexUV(x + w, y + h, 0.0, (sx + w) * us, (sy + h) * vs);
	t.vertexUV(x + w, y + 0, 0.0, (sx + w) * us, (sy + 0) * vs);
	t.vertexUV(x + 0, y + 0, 0.0, (sx + 0) * us, (sy + 0) * vs);
	t.end();
}

const std::shared_ptr<File> &Minecraft::getWorkingDirectory()
{
	if (workDir == nullptr)
	{
		workDir.reset(File::openWorkingDirectory(u".mcbetacpp"));
		if (!workDir->exists() && !workDir->mkdirs())
			throw std::runtime_error(String::toUTF8(u"The working directory could not be created: " + workDir->toString()));
	}

	return workDir;
}

void Minecraft::setScreen(std::shared_ptr<Screen> screen)
{
	// Beta: Minecraft.setScreen(Screen var1) - sets current screen (Minecraft.java:364-389)
	if (this->screen != nullptr)
		this->screen->removed();  // Beta: this.screen.removed() (Minecraft.java:367)

	if (screen == nullptr && level == nullptr)  // Beta: if (var1 == null && this.level == null) (Minecraft.java:370)
		screen = Util::make_shared<TitleScreen>(*this);  // Beta: var1 = new TitleScreen() (Minecraft.java:371)
	else if (screen == nullptr && player != nullptr && player->health <= 0)  // Beta: else if (var1 == null && this.player.health <= 0) (Minecraft.java:372)
		screen = Util::make_shared<DeathScreen>(*this);  // Beta: var1 = new DeathScreen() (Minecraft.java:373)

	this->screen = std::move(screen);
	if (this->screen != nullptr)  // Beta: if (var1 != null) (Minecraft.java:377)
	{
		releaseMouse();  // Beta: this.releaseMouse() (Minecraft.java:378)
		ScreenSizeCalculator ssc(width, height, options);
		int_t w = ssc.getWidth();
		int_t h = ssc.getHeight();
		this->screen->init(w, h);  // Beta: var1.init(this, var3, var4) (Minecraft.java:382)
		noRender = false;  // Beta: this.noRender = false (Minecraft.java:383)
		
		// Initialize controller virtual cursor position (center of screen)
		controllerCursorX = w / 2.0f;
		controllerCursorY = h / 2.0f;
		controllerCursorVisible = false; // Will be set based on screen type
		
		// Reset D-pad state
		prevDpadUp = false;
		prevDpadDown = false;
		prevDpadLeft = false;
		prevDpadRight = false;
		
		// Reset button focus for menu screens
		this->screen->setFocusedButtonIndex(-1);
	}
	else  // Beta: else (Minecraft.java:384)
	{
		grabMouse();  // Beta: this.grabMouse() (Minecraft.java:385)
	}
}

void Minecraft::checkGlError(const std::string &at)
{
	GLenum error_code = glGetError();
	if (error_code != GL_NO_ERROR)
	{
		std::string error_string;
		switch (error_code)
		{
			default:
				error_string = "unknown error code";
				break;
			case GL_NO_ERROR:
				error_string = "no error";
				break;
			case GL_INVALID_ENUM:
				error_string = "invalid enumerant";
				break;
			case GL_INVALID_VALUE:
				error_string = "invalid value";
				break;
			case GL_INVALID_OPERATION:
				error_string = "invalid operation";
				break;
			case GL_STACK_OVERFLOW:
				error_string = "stack overflow";
				break;
			case GL_STACK_UNDERFLOW:
				error_string = "stack underflow";
				break;
			case GL_OUT_OF_MEMORY:
				error_string = "out of memory";
				break;
		}

		std::cout << "########## GL ERROR ##########\n";
		std::cout << "@ " << at << '\n';
		std::cout << error_code << ": " << error_string << '\n';
	}
}

Minecraft::~Minecraft()
{
	MemoryTracker::release();
}

void Minecraft::generateFlyby()
{
	gameMode = Util::make_shared<SurvivalMode>(*this);
	selectLevel(u"flyby");
	setScreen(nullptr);

	double player_y = 0.0;

	// Prepare to create a TGA file
	std::vector<char> image_data(width * height * 3);
	std::unique_ptr<File> flyby_file(File::open(*workingDirectory, u"flyby"));
	flyby_file->mkdir();

	char header[18] = {};
	header[2] = 2;
	header[12] = width & 0xFF;
	header[13] = (width >> 8) & 0xFF;
	header[14] = height & 0xFF;
	header[15] = (height >> 8) & 0xFF;
	header[16] = 24;

	// Setup flyby
	int_t frame = -20;
	int_t seconds = 60 * 5 + 52;
	int_t frames = seconds * 60;

	player->yRot = player->yRotO = 12.0f;
	double sin = -std::sin(player->yRot * Mth::PI / 180.0);
	double cos = std::cos(player->yRot * Mth::PI / 180.0);

	player->x = player->xo = player->xOld = 0.0;
	player->z = player->zo = player->zOld = 0.0;

	level->time = 0;

	for (; frame < frames; frame++)
	{
		if ((frame % 100) == 0)
		{
			std::cout << (static_cast<double>(frame) * 100.0 / static_cast<double>(frames)) << "%, free: " << (float)(Runtime::getRuntime().freeMemory() / 1024) / 1024.0f << " MB\n";
		}

		double speed = 0.125 + (static_cast<double>(frame) / static_cast<double>(frames)) * 5.0;

		// Tick world
		AABB::resetPool();
		Vec3::resetPool();

		if (frame < 0)
		{
			level->setSpawnSettings(options.difficulty > 0, true);
			level->tick();
		}

		gameRenderer.tick();

		glEnable(GL_TEXTURE_2D);

		while (level->updateLights());

		// Move player
		player->x = player->xo = player->xOld += sin * speed;
		player->z = player->zo = player->zOld += cos * speed;

		int_t check_distance = 100;
		double min_height = 0.0;
		double check_step = 1.0;

		for (double d = -4.0; d < check_distance; d += check_step)
		{
			for (int_t i = 0; i < 9; i++)
			{
				double x = ((i % 3) / 2.0f) - 0.5;
				double z = ((i / 3) / 2.0f) - 0.5;
				double y = level->getHeightmap(Mth::floor(player->x + sin * d + x), Mth::floor(player->z + cos * d + z));
				if (y > min_height)
					min_height = y;
			}
		}

		double target_y = min_height + 4.0;
		if (player_y == 0.0)
			player_y = target_y;
		else
			player_y += (target_y - player_y) * speed / check_distance * 4.0;

		player->xRot = player->xRotO = (player_y - 64.0) / 2.0f;
		player->y = player->yo = player->yOld = player_y;

		// Render world
		gameRenderer.renderLevel(1.0f);

		lwjgl::Display::update();

		// Save to TGA
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_UNSIGNED_BYTE, image_data.data());

		if (frame >= 0)
		{
			jstring name = String::toString(frame);
			for (; name.size() < 6; name = u"0" + name);

			std::unique_ptr<File> file(File::open(*flyby_file, name + u".tga"));
			std::unique_ptr<std::ostream> out(file->toStreamOut());
			out->write(header, 18);
			out->write(image_data.data(), image_data.size());
		}
	}
}

void Minecraft::run()
{
	// init
	running = true;

#ifdef NDEBUG
	try
	{
#endif
		init();
#ifdef NDEBUG
	}
	catch (std::exception &e)
	{
		onCrash("Failed to start game", e);
		return;
	}
#endif

#ifdef NDEBUG
	try
	{
#endif
		if (FLYBY_MODE)
			generateFlyby();

		long_t fpsMs = System::currentTimeMillis();
		int_t fpsFrames = 0;
		int_t fpsTicks = 0;

		while (running)
		{
			AABB::resetPool();
			Vec3::resetPool();

			if (lwjgl::Display::isCloseRequested())
				stop();

			if (pause && level != nullptr)
			{
				float aO = timer.a;
				timer.advanceTime();
				timer.a = aO;
			}
			else
			{
				timer.advanceTime();
			}

			// Tick game
			long_t tickNano = System::nanoTime();
			for (byte_t i = 0; i < timer.ticks; i++)
			{
				ticks++;
				fpsTicks++;
				tick();
			}
			long_t tickNanos = System::nanoTime() - tickNano;

		checkGlError("Pre render");
		
		// Beta: Update sound engine every frame (Minecraft.java:588)
		if (player != nullptr)
			soundEngine.update(player.get(), timer.a);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

			// Update lights once per frame (was being called twice before)
			if (level != nullptr)
				level->updateLights();

			// Limit framerate if set (260 = unlimited)
			// VSync is handled by SDL_GL_SetSwapInterval, so we only need to limit if VSync is disabled
			if (!options.vsync && options.limitFramerate < 260)
			{
				// Calculate target frame time in milliseconds
				// fps = 1000 / frameTimeMs
				// frameTimeMs = 1000 / fps
				long_t targetFrameTime = 1000 / options.limitFramerate;
				long_t currentTime = System::currentTimeMillis();
				static long_t lastFrameTime = 0;
				if (lastFrameTime == 0)
					lastFrameTime = currentTime;
				long_t elapsed = currentTime - lastFrameTime;
				
				if (elapsed < targetFrameTime)
				{
					// Sleep to limit framerate
					std::this_thread::sleep_for(std::chrono::milliseconds(targetFrameTime - elapsed));
					lastFrameTime = System::currentTimeMillis();
				}
				else
				{
					lastFrameTime = currentTime;
				}
			}
			else if (options.vsync)
			{
				// VSync is enabled - reset lastFrameTime so it doesn't interfere
				static long_t lastFrameTime = 0;
				lastFrameTime = 0;
			}

			if (!lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_F7))
				lwjgl::Display::update();

			if (!noRender)
			{
				if (gameMode != nullptr)
					gameMode->render(timer.a);
				gameRenderer.render(timer.a);
			}

			if (!lwjgl::Display::isActive())
			{
				if (fullscreen)
					toggleFullscreen();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			if (options.showDebugInfo)
			{
				renderFpsMeter(tickNanos);
			}
			else
			{
				lastTimer = System::nanoTime();
			}

			// Removed std::this_thread::yield() - causes unnecessary context switches
			if (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_F7))
				lwjgl::Display::update();

			if (!fullscreen)
			{
				int_t w = lwjgl::Display::getWidth();
				int_t h = lwjgl::Display::getHeight();
				if (w != width || h != height)
				{
					width = w;
					height = h;
					if (width <= 0)
						width = 1;
					if (height <= 0)
						height = 1;
					resize(width, height);
				}
			}

			checkGlError("Post render");
			fpsFrames++;

			pause = !isOnline() && screen != nullptr && screen->isPauseScreen();

			while (System::currentTimeMillis() >= fpsMs + 1000)
			{
				fpsString = String::fromUTF8(std::to_string(fpsFrames) + " fps, " + std::to_string(Chunk::updates) + " chunk updates");
				Chunk::updates = 0;
				fpsMs += 1000;
				fpsFrames = 0;
			}
		}
#ifdef NDEBUG
	}
	catch (std::exception &e)
	{
		onCrash("Unexpected error", e);
		return;
	}
#endif
}

void Minecraft::renderFpsMeter(long tickNanos)
{
	// Update times
	long_t target = 16666666LL; // 1 / 60
	if (lastTimer == -1)
		lastTimer = System::nanoTime();

	long_t now = System::nanoTime();

	tickTimes[frameTimePos & (frameTimes.size() - 1)] = tickNanos;
	frameTimes[frameTimePos & (frameTimes.size() - 1)] = now - lastTimer;
	frameTimePos++;

	lastTimer = now;

	// Draw
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, height, 0.0, 1000.0, 3000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -2000.0f);

	glLineWidth(1.0f);
	glDisable(GL_TEXTURE_2D);

	Tesselator &t = Tesselator::instance;
	t.begin(GL_QUADS);

	int_t targetHeight = target / 200000LL;

	t.color(0x20000000);
	t.vertex(0.0, this->height - targetHeight, 0.0);
	t.vertex(0.0, this->height, 0.0);
	t.vertex(frameTimes.size(), this->height, 0.0);
	t.vertex(frameTimes.size(), this->height - targetHeight, 0.0);

	t.color(0x20200000);
	t.vertex(0.0, this->height - targetHeight * 2, 0.0);
	t.vertex(0.0, this->height - targetHeight, 0.0);
	t.vertex(frameTimes.size(), this->height - targetHeight, 0.0);
	t.vertex(frameTimes.size(), this->height - targetHeight * 2, 0.0);

	t.end();

	int_t total = 0;
	for (int_t i = 0; i < frameTimes.size(); i++)
		total += frameTimes[i];
	int_t avg = total / 200000LL / frameTimes.size();

	t.begin(GL_QUADS);
	t.color(0x20400000);
	t.vertex(0.0, (height - avg), 0.0);
	t.vertex(0.0, height, 0.0);
	t.vertex(frameTimes.size(), height, 0.0);
	t.vertex(frameTimes.size(), (height - avg), 0.0);
	t.end();

	t.begin(GL_LINES);
	for (int_t i = 0; i < frameTimes.size(); i++)
	{
		int_t x = ((i - frameTimePos) & (frameTimes.size() - 1)) * 255 / frameTimes.size();
		int_t m = x * x / 255;
		m = m * m / 255;
		int_t n = m * m / 255;
		n = n * n / 255;

		if (frameTimes[i] > target)
			t.color(0xFF000000 + (m * 0x010000));
		else
			t.color(0xFF000000 + (m * 0x0100));

		long_t ft = frameTimes[i] / 200000LL;
		long_t tt = tickTimes[i] / 200000LL;

		t.vertex((i + 0.5F), (static_cast<float>(height - ft) + 0.5F), 0.0);
		t.vertex((i + 0.5F), (height + 0.5F), 0.0);

		t.color(0xFF000000 + (m * 0x010000) + (m * 0x0100) + (m * 0x01));
		t.vertex((i + 0.5F), (static_cast<float>(height - ft) + 0.5F), 0.0);
		t.vertex((i + 0.5F), (static_cast<float>(height - ft - tt) + 0.5F), 0.0);
	}
	t.end();

	glEnable(GL_TEXTURE_2D);
}

void Minecraft::stop()
{
	running = false;
}

void Minecraft::grabMouse()
{
	if (!lwjgl::Display::isActive())
		return;
	if (mouseGrabbed)
		return;
	mouseGrabbed = true;
	mouseHandler.grab();
	setScreen(nullptr);
	lastClickTick = ticks + 10000;
}

void Minecraft::releaseMouse()
{
	if (!mouseGrabbed)
		return;
	mouseGrabbed = false;
	mouseHandler.release();
}

void Minecraft::pauseGame()
{
	if (this->screen != nullptr)
		return;
	setScreen(Util::make_shared<PauseScreen>(*this));
}

void Minecraft::handleMouseDown(int_t button, bool down)
{
	// Beta: handleMouseDown() (Minecraft.java:835-849)
	if (!gameMode->instaBuild)
	{
		if (button != 0 || missTime <= 0)
		{
			if (down && hitResult.type == HitResult::Type::TILE && button == 0)
			{
				int_t x = hitResult.x;
				int_t y = hitResult.y;
				int_t z = hitResult.z;
				Facing f = hitResult.f;
				gameMode->continueDestroyBlock(x, y, z, f);
				particleEngine.crack(x, y, z, static_cast<int_t>(f));
			}
			else
			{
				gameMode->stopDestroyBlock();
			}
		}
	}
}

void Minecraft::handleMouseClick(int_t button)
{
	if (button == 0 && missTime > 0)
		return;
	if (button == 0)
		player->swing();

	bool canUseItem = true;

	if (hitResult.type == HitResult::Type::NONE)
	{
		if (button == 0 && !gameMode->isCreativeMode())
			missTime = 10;
	}
	else if (hitResult.type == HitResult::Type::ENTITY)
	{
		auto player = std::static_pointer_cast<Player>(this->player);

		if (button == 0)
			gameMode->attack(player, hitResult.entity);
		else if (button == 1)
			gameMode->interact(player, hitResult.entity);
	}
	else if (hitResult.type == HitResult::Type::TILE)
	{
		int_t x = hitResult.x;
		int_t y = hitResult.y;
		int_t z = hitResult.z;
		Facing f = hitResult.f;

		Tile &t = *Tile::tiles[level->getTile(x, y, z)];

		if (button == 0)
		{
			level->extinguishFire(x, y, z, hitResult.f);
			// Alpha 1.2.6: Check userType >= 100 for block breaking (Minecraft.java:876-878)
			// Java: if (var7 != Tile.unbreakable || this.player.userType >= 100) {
			//     this.gameMode.startDestroyBlock(var3, var4, var5, this.hitResult.f);
			// }
			if (t.id != 7 || player->userType >= 100)  // Tile 7 is bedrock (unbreakable)
			{
				gameMode->startDestroyBlock(x, y, z, hitResult.f);
			}
		}
		else if (button == 1)
		{
			// Beta: Right-click block placement (Minecraft.java:879-895)
			auto playerPtr = std::static_pointer_cast<Player>(this->player);
			ItemStack *item = playerPtr->inventory.getSelected();  // Beta: ItemInstance var8 = this.player.inventory.getSelected() (Minecraft.java:880)
			int_t oldCount = item != nullptr ? item->stackSize : 0;  // Beta: int var9 = var8 != null ? var8.count : 0 (Minecraft.java:881)
			
			if (gameMode->useItemOn(playerPtr, level, item, x, y, z, f))  // Beta: this.gameMode.useItemOn(this.player, this.level, var8, var3, var4, var5, var6) (Minecraft.java:882)
			{
				canUseItem = false;  // Beta: var2 = false (Minecraft.java:883)
				playerPtr->swing();  // Beta: this.player.swing() (Minecraft.java:884)
			}
			
			if (item != nullptr)
			{
				// Beta: if (var8.count == 0) this.player.inventory.items[this.player.inventory.selected] = null (Minecraft.java:891-892)
				if (item->isEmpty())
				{
					ItemStack *selected = playerPtr->inventory.getSelected();
					if (selected != nullptr)
						*selected = ItemStack();  // Clear stack
				}
				// Beta: else if (var8.count != var9) this.gameRenderer.itemInHandRenderer.itemPlaced() (Minecraft.java:893-894)
				else if (item->stackSize != oldCount)
				{
					gameRenderer.itemInHandRenderer.itemPlaced();
				}
			}
		}
	}

	if (canUseItem && button == 1)
	{
		// Beta: Right-click item use (Minecraft.java:899-903)
		auto playerPtr = std::static_pointer_cast<Player>(this->player);
		ItemStack *item = playerPtr->inventory.getSelected();  // Beta: ItemInstance var10 = this.player.inventory.getSelected() (Minecraft.java:900)
		if (item != nullptr && !item->isEmpty() && gameMode->useItem(playerPtr, level, item))  // Beta: if (var10 != null && this.gameMode.useItem(this.player, this.level, var10)) (Minecraft.java:901)
		{
			gameRenderer.itemInHandRenderer.itemUsed();  // Beta: this.gameRenderer.itemInHandRenderer.itemUsed() (Minecraft.java:902)
		}
	}
}

void Minecraft::handleControllerTriggerDown(int_t button, bool down)
{
	// 1:1 copy of handleMouseDown for controller triggers
	// button: 0 = right trigger (attack), 1 = left trigger (use)
	// Beta: handleMouseDown() (Minecraft.java:835-849)
	if (!gameMode->instaBuild)
	{
		if (button != 0 || missTime <= 0)
		{
			if (down && hitResult.type == HitResult::Type::TILE && button == 0)
			{
				int_t x = hitResult.x;
				int_t y = hitResult.y;
				int_t z = hitResult.z;
				Facing f = hitResult.f;
				gameMode->continueDestroyBlock(x, y, z, f);
				particleEngine.crack(x, y, z, static_cast<int_t>(f));
			}
			else
			{
				gameMode->stopDestroyBlock();
			}
		}
	}
}

void Minecraft::handleControllerTriggerClick(int_t button)
{
	// 1:1 copy of handleMouseClick for controller triggers
	// button: 0 = right trigger (attack), 1 = left trigger (use)
	if (button == 0 && missTime > 0)
		return;
	if (button == 0)
		player->swing();

	bool canUseItem = true;

	if (hitResult.type == HitResult::Type::NONE)
	{
		if (button == 0 && !gameMode->isCreativeMode())
			missTime = 10;
	}
	else if (hitResult.type == HitResult::Type::ENTITY)
	{
		auto player = std::static_pointer_cast<Player>(this->player);

		if (button == 0)
			gameMode->attack(player, hitResult.entity);
		else if (button == 1)
			gameMode->interact(player, hitResult.entity);
	}
	else if (hitResult.type == HitResult::Type::TILE)
	{
		int_t x = hitResult.x;
		int_t y = hitResult.y;
		int_t z = hitResult.z;
		Facing f = hitResult.f;

		Tile &t = *Tile::tiles[level->getTile(x, y, z)];

		if (button == 0)
		{
			level->extinguishFire(x, y, z, hitResult.f);
			// Alpha 1.2.6: Check userType >= 100 for block breaking (Minecraft.java:876-878)
			// Java: if (var7 != Tile.unbreakable || this.player.userType >= 100) {
			//     this.gameMode.startDestroyBlock(var3, var4, var5, this.hitResult.f);
			// }
			if (t.id != 7 || player->userType >= 100)  // Tile 7 is bedrock (unbreakable)
			{
				gameMode->startDestroyBlock(x, y, z, hitResult.f);
			}
		}
		else if (button == 1)
		{
			// Beta: Right-click block placement (Minecraft.java:879-895)
			auto playerPtr = std::static_pointer_cast<Player>(this->player);
			ItemStack *item = playerPtr->inventory.getSelected();  // Beta: ItemInstance var8 = this.player.inventory.getSelected() (Minecraft.java:880)
			int_t oldCount = item != nullptr ? item->stackSize : 0;  // Beta: int var9 = var8 != null ? var8.count : 0 (Minecraft.java:881)
			
			if (gameMode->useItemOn(playerPtr, level, item, x, y, z, f))  // Beta: this.gameMode.useItemOn(this.player, this.level, var8, var3, var4, var5, var6) (Minecraft.java:882)
			{
				canUseItem = false;  // Beta: var2 = false (Minecraft.java:883)
				playerPtr->swing();  // Beta: this.player.swing() (Minecraft.java:884)
			}
			
			if (item != nullptr)
			{
				// Beta: if (var8.count == 0) this.player.inventory.items[this.player.inventory.selected] = null (Minecraft.java:891-892)
				if (item->isEmpty())
				{
					ItemStack *selected = playerPtr->inventory.getSelected();
					if (selected != nullptr)
						*selected = ItemStack();  // Clear stack
				}
				// Beta: else if (var8.count != var9) this.gameRenderer.itemInHandRenderer.itemPlaced() (Minecraft.java:893-894)
				else if (item->stackSize != oldCount)
				{
					gameRenderer.itemInHandRenderer.itemPlaced();
				}
			}
		}
	}

	if (canUseItem && button == 1)
	{
		// Beta: Right-click item use (Minecraft.java:899-903)
		auto playerPtr = std::static_pointer_cast<Player>(this->player);
		ItemStack *item = playerPtr->inventory.getSelected();  // Beta: ItemInstance var10 = this.player.inventory.getSelected() (Minecraft.java:900)
		if (item != nullptr && !item->isEmpty() && gameMode->useItem(playerPtr, level, item))  // Beta: if (var10 != null && this.gameMode.useItem(this.player, this.level, var10)) (Minecraft.java:901)
		{
			gameRenderer.itemInHandRenderer.itemUsed();  // Beta: this.gameRenderer.itemInHandRenderer.itemUsed() (Minecraft.java:902)
		}
	}
}

void Minecraft::toggleFullscreen()
{
	fullscreen = !fullscreen;
	std::cout << "Toggle fullscreen!\n";

	lwjgl::Display::setFullscreen(fullscreen);
	width = lwjgl::Display::getWidth();
	height = lwjgl::Display::getHeight();
	if (width <= 0)
		width = 1;
	if (height <= 0)
		height = 1;

	releaseMouse();
	lwjgl::Display::update();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	if (fullscreen)
		grabMouse();

	if (screen != nullptr)
	{
		releaseMouse();
		resize(width, height);
	}

	std::cout << "Size: " << width << ", " << height << '\n';
}

void Minecraft::resize(int_t w, int_t h)
{
	if (w <= 0)
		w = 1;
	if (h <= 0)
		h = 1;

	width = w;
	height = h;

	if (screen != nullptr)
	{
		ScreenSizeCalculator ssc(width, height, options);
		int_t w = ssc.getWidth();
		int_t h = ssc.getHeight();
		screen->init(w, h);
	}
}

void Minecraft::handleGrabTexture()
{
	if (hitResult.type != HitResult::Type::NONE)
	{
		int_t i = level->getTile(hitResult.x, hitResult.y, hitResult.z);
		// TODO
	}
}

void Minecraft::tick()
{

	gameRenderer.pick(1.0f);

	// Update controller input screen state (for preventing movement/jump in GUI)
	screenOpenForController = (screen != nullptr);
	if (player != nullptr)
	{
		auto* controllerInput = dynamic_cast<ControllerInput*>(player->input.get());
		if (controllerInput != nullptr)
		{
			controllerInput->setScreenOpenPtr(&screenOpenForController);
		}
	}

	// Update chunk caching
	if (player != nullptr)
	{
		player->prepareForTick();

		auto chunkSource = level->getChunkSource();
		if (chunkSource->isChunkCache())
		{
			ChunkCache &chunkCache = static_cast<ChunkCache &>(*chunkSource);
			int_t x = Mth::floor(static_cast<float>(static_cast<int_t>(player->x))) >> 4;
			int_t z = Mth::floor(static_cast<float>(static_cast<int_t>(player->z))) >> 4;
			chunkCache.centerOn(x, z);
		}
	}

	// 
	if (!pause && level != nullptr)
		gameMode->tick();

	// Texture update
	glBindTexture(GL_TEXTURE_2D, textures.loadTexture(u"/terrain.png"));
	if (!pause)
		textures.tick();

	// Show death screen when dead
	if (screen == nullptr && player != nullptr && player->health <= 0)
		setScreen(nullptr);

	// Update gamepad state (must be called every frame to get D-pad state)
	lwjgl::Gamepad::update();
	
	// Get controller state once for use throughout tick() function
	auto* controllerState = lwjgl::Gamepad::getFirstController();
	
	// Controller button actions (Controlify-style, handled before second update call)
	// Check for controller buttons even if player isn't using controller input
	// Must be done here BEFORE the second Gamepad::update() call to preserve prevButton states
	if (screen == nullptr && player != nullptr && controllerState != nullptr && controllerState->connected)
	{
		// Y button (North) = Open inventory (Controlify default binding, console edition style)
		bool yButtonPressed = controllerState->buttonNorth && !controllerState->prevButtonNorth;
		if (yButtonPressed)
		{
			setScreen(Util::make_shared<InventoryScreen>(*this));
		}
		
		// Start button = Open pause menu (Controlify default binding)
		bool startButtonPressed = controllerState->buttonStart && !controllerState->prevButtonStart;
		if (startButtonPressed)
		{
			pauseGame();
		}
	}
	
	// Tick screen
	if (screen != nullptr)
		lastClickTick = ticks + 10000;
	if (screen != nullptr)
	{
		std::shared_ptr<Screen> screen_ptr = screen; // Ensure that the screen is not deleted while processing events
		
		// Dynamic input switching: check both controller and mouse/keyboard, process whichever is active
		// This allows seamless switching between input methods (mixed input mode)
		
		// Check which input device is actively being used
		// This determines which cursor position to use and which input to process
		bool controllerActive = false;
		bool mouseKeyboardActive = false;
		ControllerInput* activeControllerInput = nullptr;
		
		// Get controller input from player (if player is using controller)
		ControllerInput* controllerInput = (player != nullptr) ? dynamic_cast<ControllerInput*>(player->input.get()) : nullptr;
		
		// Check for controller input activity
		if (controllerInput != nullptr)
		{
			// Player is using controller input - check if controller is active
			auto* state = controllerInput->getControllerState();
			if (state != nullptr)
			{
				if (state->buttonSouth || state->buttonEast || state->buttonWest || 
				    state->buttonNorth || state->buttonStart || state->buttonBack ||
				    state->buttonDpadUp || state->buttonDpadDown ||
				    state->buttonDpadLeft || state->buttonDpadRight)
				{
					controllerActive = true;
				}
				
				float leftStickX = static_cast<float>(state->leftStickX) / 32767.0f;
				float leftStickY = static_cast<float>(state->leftStickY) / 32767.0f;
				float rightStickX = static_cast<float>(state->rightStickX) / 32767.0f;
				float rightStickY = static_cast<float>(state->rightStickY) / 32767.0f;
				
				if (std::abs(leftStickX) > 0.1f || std::abs(leftStickY) > 0.1f ||
				    std::abs(rightStickX) > 0.1f || std::abs(rightStickY) > 0.1f)
				{
					controllerActive = true;
				}
			}
			
			if (controllerActive)
			{
				activeControllerInput = controllerInput;
			}
		}
		else if (controllerState != nullptr && controllerState->connected)
		{
			// Controller is connected but player isn't using it - check if controller is active for menu navigation
			if (controllerState->buttonSouth || controllerState->buttonEast || controllerState->buttonWest || 
			    controllerState->buttonNorth || controllerState->buttonStart || controllerState->buttonBack ||
			    controllerState->buttonDpadUp || controllerState->buttonDpadDown ||
			    controllerState->buttonDpadLeft || controllerState->buttonDpadRight)
			{
				controllerActive = true;
			}
			
			float leftStickX = static_cast<float>(controllerState->leftStickX) / 32767.0f;
			float leftStickY = static_cast<float>(controllerState->leftStickY) / 32767.0f;
			float rightStickX = static_cast<float>(controllerState->rightStickX) / 32767.0f;
			float rightStickY = static_cast<float>(controllerState->rightStickY) / 32767.0f;
			
			if (std::abs(leftStickX) > 0.1f || std::abs(leftStickY) > 0.1f ||
			    std::abs(rightStickX) > 0.1f || std::abs(rightStickY) > 0.1f)
			{
				controllerActive = true;
			}
			
			if (controllerActive)
			{
				// Create a temporary controller input just for menu navigation
				static ControllerInput tempControllerInput(controllerState, options);
				activeControllerInput = &tempControllerInput;
			}
		}
		
		// Check for mouse/keyboard input activity
		// Check for mouse button events
		if (lwjgl::Mouse::getEventButton() >= 0)
		{
			mouseKeyboardActive = true;
		}
		
		// Check for keyboard events
		if (lwjgl::Keyboard::getEventKey() >= 0)
		{
			mouseKeyboardActive = true;
		}
		
		// Check for significant mouse movement (only if controller is not active)
		if (!controllerActive)
		{
			int_t mouseX = lwjgl::Mouse::getX();
			int_t mouseY = lwjgl::Mouse::getY();
			// Use a threshold to detect actual mouse movement
			if (std::abs(mouseX - width / 2) > 10 || std::abs(mouseY - height / 2) > 10)
			{
				mouseKeyboardActive = true;
			}
		}
		
		// Process controller input if controller is active
		// Controller input takes priority when both are active
		if (controllerActive && activeControllerInput != nullptr)
		{
			controllerInputActive = true;
			updateControllerCursor(activeControllerInput);
		}
		else
		{
			controllerInputActive = false;
		}
		
		// Always process mouse/keyboard events (they work even when controller is connected)
		// Both input methods can coexist - mouse/keyboard won't interfere with controller navigation
		screen_ptr->updateEvents();
		if (screen != nullptr)
			screen_ptr->tick();
	}

	// Update gamepad states (must be called every frame to read button/axis states)
	lwjgl::Gamepad::update();
	
	// Handle controller button actions (third person view, etc.)
	if (player != nullptr)
	{
		auto* controllerInput = dynamic_cast<ControllerInput*>(player->input.get());
		if (controllerInput != nullptr)
		{
			controllerInput->updateButtonActions(*player);
		}
	}
	
	// Event processing
	if (screen == nullptr || screen->passEvents)
	{
		while (lwjgl::Mouse::next())
		{
			long_t tickDelta = System::currentTimeMillis() - lastTickTime;
			if (tickDelta > 200)
				continue;

			int_t dwheel = lwjgl::Mouse::getEventDWheel();
			// Alpha: Scroll wheel hotbar selection (Minecraft.java:999-1002)
			if (dwheel != 0 && screen == nullptr && player != nullptr)
			{
				player->inventory.changeCurrentItem(dwheel);
			}

			if (screen == nullptr)
			{
				if (!mouseGrabbed && lwjgl::Mouse::getEventButtonState())
				{
					grabMouse();
					continue;
				}
				if (lwjgl::Mouse::getEventButton() == 0 && lwjgl::Mouse::getEventButtonState())
				{
					handleMouseClick(0);
					lastClickTick = ticks;
				}
				if (lwjgl::Mouse::getEventButton() == 1 && lwjgl::Mouse::getEventButtonState())
				{
					handleMouseClick(1);
					lastClickTick = ticks;
				}
				if (lwjgl::Mouse::getEventButton() == 2 && lwjgl::Mouse::getEventButtonState())
					handleGrabTexture();
				continue;
			}
			if (screen != nullptr)
				screen->mouseEvent();
		}

		if (missTime > 0)
			missTime--;

		while (lwjgl::Keyboard::next())
		{
			player->setKey(lwjgl::Keyboard::getEventKey(), lwjgl::Keyboard::getEventKeyState());

			if (lwjgl::Keyboard::getEventKeyState())
			{
				if (lwjgl::Keyboard::getEventKey() == lwjgl::Keyboard::KEY_F11)
				{
					toggleFullscreen();
					continue;
				}

				if (screen != nullptr)
				{
					screen->keyboardEvent();
				}
				else
				{
					if (lwjgl::Keyboard::getEventKey() == lwjgl::Keyboard::KEY_ESCAPE)
						pauseGame();
					if (lwjgl::Keyboard::getEventKey() == lwjgl::Keyboard::KEY_F3)
						options.showDebugInfo = !options.showDebugInfo;  // Alpha: toggle debug screen (Minecraft.java:945-947)
					if (lwjgl::Keyboard::getEventKey() == lwjgl::Keyboard::KEY_S && options.showDebugInfo)
						reloadSound();  // Alpha: F3+S reloads sound (Minecraft.java:937-939)
					if (lwjgl::Keyboard::getEventKey() == lwjgl::Keyboard::KEY_F5)
					{
						++options.thirdPersonView;  // Alpha: cycle third person view (Minecraft.java:950-953)
						if (options.thirdPersonView > 2)
							options.thirdPersonView = 0;
					}
					
					// Alpha 1.2.6: Drop item on drop key (Q) - Java: if (Keyboard.getEventKey() == this.options.keyBindDrop.keyCode) { this.thePlayer.dropCurrentItem(); }
					// Java: This is inside the currentScreen == null block
					if (lwjgl::Keyboard::getEventKey() == options.keyDrop.key)
					{
						if (isOnline())
						{
							// Multiplayer: Send Packet14BlockDig(4, 0, 0, 0, 0) via EntityClientPlayerMP
							EntityClientPlayerMP* mpPlayer = dynamic_cast<EntityClientPlayerMP*>(player.get());
							if (mpPlayer != nullptr)
							{
								mpPlayer->dropCurrentItem();
							}
							else
							{
								std::cout << "[Drop] WARNING: dynamic_cast to EntityClientPlayerMP failed!" << std::endl;
							}
						}
						else
						{
							// Single-player: Drop item locally
							// Note: In single-player, LocalPlayer should have a dropCurrentItem method, but for now just drop the selected item
							ItemStack *selected = player->inventory.getCurrentItem();
							if (selected != nullptr && !selected->isEmpty())
							{
								player->drop(*selected);
								player->removeSelectedItem();
							}
						}
					}
				}

				// Alpha: Hotbar selection with number keys (Minecraft.java:982-986)
				for (int_t i = 0; i < 9; i++)
				{
					if (lwjgl::Keyboard::getEventKey() == lwjgl::Keyboard::KEY_1 + i)
					{
						player->inventory.currentItem = i;
					}
				}

				if (lwjgl::Keyboard::getEventKey() == options.keyFog.key)
					options.toggle(Options::Option::RENDER_DISTANCE, (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LSHIFT) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RSHIFT)) ? -1 : 1);
				
				// Beta: Open inventory screen on inventory key (I)
				if (lwjgl::Keyboard::getEventKey() == options.keyInventory.key)
				{
					if (screen == nullptr)
					{
						setScreen(Util::make_shared<InventoryScreen>(*this));
					}
				}
				
				// Alpha 1.2.6: Open chat screen on chat key (T) when online
				// Java: if (this.isOnline() && Keyboard.getEventKey() == this.options.keyChat.key) {
				//     this.setScreen(new ChatScreen());
				// }
				if (isOnline() && lwjgl::Keyboard::getEventKey() == options.keyChat.key)
				{
					if (screen == nullptr)
					{
						setScreen(Util::make_shared<GuiChat>(*this));
					}
				}
			}
			}
		}
		
		// Declare controllerInput once for use in both GUI navigation and trigger handling
		ControllerInput* controllerInput = nullptr;
		
		// Controller GUI navigation (when screen is open)
		// Check both player's controller input and standalone controller for GUI actions
		if (screen != nullptr)
		{
			controllerInput = (player != nullptr) ? dynamic_cast<ControllerInput*>(player->input.get()) : nullptr;
			if (controllerInput == nullptr && controllerState != nullptr && controllerState->connected)
			{
				// Create temporary controller input for menu navigation
				static ControllerInput tempControllerInput(controllerState, options);
				controllerInput = &tempControllerInput;
			}
			
			if (controllerInput != nullptr)
			{
				// Back button = Close menu screens (ESC equivalent, Controlify-style)
				// B button (East) closing for container screens is handled in updateControllerCursor()
				if (!screen->hasSlots()) // Menu screen (title, pause, options, etc.)
				{
					// Back button closes menu screens (but not pause screen)
					if (controllerInput->wasButtonJustPressed(SDL_GAMEPAD_BUTTON_BACK))
					{
						if (!screen->isPauseScreen())
						{
							setScreen(nullptr);
							grabMouse();
						}
					}
				}
			}
		}
	

		if (screen == nullptr)
		{
			if (lwjgl::Mouse::isButtonDown(0) && static_cast<float>(ticks - lastClickTick) >= (timer.ticksPerSecond / 4.0f) && mouseGrabbed)
			{
				handleMouseClick(0);
				lastClickTick = ticks;
			}
			if (lwjgl::Mouse::isButtonDown(1) && static_cast<float>(ticks - lastClickTick) >= (timer.ticksPerSecond / 4.0f) && mouseGrabbed)
			{
				handleMouseClick(1);
				lastClickTick = ticks;
			}
			
			// Handle controller trigger clicks when screen is closed
			controllerInput = (player != nullptr && mouseGrabbed) 
				? dynamic_cast<ControllerInput*>(player->input.get()) : nullptr;
			
			if (controllerInput != nullptr)
			{
				// Controller trigger clicks (same order as mouse event handling)
				// Right trigger: Attack/Mining (left click equivalent)
				if (controllerInput->wasRightTriggerJustPressed())
				{
					handleControllerTriggerClick(0);
					lastClickTick = ticks;
				}
				// Left trigger: Place/Use (right click equivalent)
				if (controllerInput->wasLeftTriggerJustPressed())
				{
					handleControllerTriggerClick(1);
					lastClickTick = ticks;
				}
				
				// Continuous trigger clicks (same as continuous mouse clicks)
				if (controllerInput->isRightTriggerPressed() && static_cast<float>(ticks - lastClickTick) >= (timer.ticksPerSecond / 4.0f))
				{
					handleControllerTriggerClick(0);
					lastClickTick = ticks;
				}
				if (controllerInput->isLeftTriggerPressed() && static_cast<float>(ticks - lastClickTick) >= (timer.ticksPerSecond / 4.0f))
				{
					handleControllerTriggerClick(1);
					lastClickTick = ticks;
				}
				
				// Continuous trigger down (for block breaking)
				handleControllerTriggerDown(0, controllerInput->isRightTriggerPressed());
			}
			else
			{
				// Mouse input (only if not using controller)
				handleMouseDown(0, (screen == nullptr && lwjgl::Mouse::isButtonDown(0) && mouseGrabbed));
			}
		}

	// Level tick
	if (level != nullptr)
	{
		if (player != nullptr)
		{
			recheckPlayerIn++;
			if (recheckPlayerIn == 30)
			{
				recheckPlayerIn = 0;
				level->ensureAdded(player);
			}
		}

		level->difficulty = options.difficulty;
		if (level->isOnline)
			level->difficulty = 3;

		if (!pause)
			gameRenderer.tick();
		if (!pause)
			levelRenderer.tick();
		if (!pause)
			level->tickEntities();
		if (!pause || isOnline())
		{
			level->setSpawnSettings(options.difficulty > 0, true);
			level->tick();
		}
		if (!pause && level != nullptr)
			level->animateTick(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
		// Beta: Tick particle engine (Minecraft.java:1170-1172)
		if (!pause)
			particleEngine.tick();
	}

	// Beta 1.2/newb12: Tick GUI (increments ticks for chat messages)
	// Java: this.gui.tick();
	gui.tick();
	
	// Update controller trigger states at end of frame (for "just pressed" detection next frame)
	if (player != nullptr)
	{
		auto* controllerInput = dynamic_cast<ControllerInput*>(player->input.get());
		if (controllerInput != nullptr)
		{
			controllerInput->updateTriggerStates();
		}
	}

	lastTickTime = System::currentTimeMillis();
}

void Minecraft::updateControllerCursor(ControllerInput* controllerInput)
{
	if (controllerInput == nullptr || screen == nullptr)
		return;
	
	// Get screen dimensions
	ScreenSizeCalculator ssc(width, height, options);
	int_t w = ssc.getWidth();
	int_t h = ssc.getHeight();
	
	// Check if this is a container screen (has slots) or menu screen (has buttons)
	bool hasSlots = screen->hasSlots();
	
	// Only show cursor for container screens (inventory, chest, etc.)
	controllerCursorVisible = hasSlots;
	
	// Get D-pad state
	auto* controllerState = controllerInput->getControllerState();
	bool dpadUp = (controllerState != nullptr && controllerState->buttonDpadUp);
	bool dpadDown = (controllerState != nullptr && controllerState->buttonDpadDown);
	bool dpadLeft = (controllerState != nullptr && controllerState->buttonDpadLeft);
	bool dpadRight = (controllerState != nullptr && controllerState->buttonDpadRight);
	
	if (hasSlots)
	{
		// Container screen: D-pad snaps to slots, right stick moves cursor smoothly
		
		// Get right stick input for cursor movement (Controlify-style)
		float stickX = controllerInput->getRightStickX();
		float stickY = controllerInput->getRightStickY();
		
		// Move cursor based on stick input (Controlify-style)
		float speed = controllerCursorSpeed;
		controllerCursorX += stickX * speed;
		controllerCursorY += stickY * speed; // Up on stick = negative Y = moves cursor up (correct)
		
		// Snap to nearest slot when stick is released (Controlify-style)
		if (stickX == 0.0f && stickY == 0.0f && (prevCursorStickX != 0.0f || prevCursorStickY != 0.0f))
		{
			// Just released stick - snap to nearest slot
			std::vector<std::pair<int_t, int_t>> snapPoints;
			screen->collectSlotSnapPoints(snapPoints);
			
			if (!snapPoints.empty())
			{
				float minDist = 1e9f;
				int_t bestX = static_cast<int_t>(controllerCursorX);
				int_t bestY = static_cast<int_t>(controllerCursorY);
				
				for (const auto& point : snapPoints)
				{
					float dx = static_cast<float>(point.first) - controllerCursorX;
					float dy = static_cast<float>(point.second) - controllerCursorY;
					float dist = dx * dx + dy * dy;
					if (dist < minDist)
					{
						minDist = dist;
						bestX = point.first;
						bestY = point.second;
					}
				}
				
				controllerCursorX = static_cast<float>(bestX);
				controllerCursorY = static_cast<float>(bestY);
			}
		}
		
		prevCursorStickX = stickX;
		prevCursorStickY = stickY;
		
		// D-pad snaps directly to slots (Controlify-style)
		if (dpadUp && !prevDpadUp)
		{
			// Snap to nearest slot in direction
			std::vector<std::pair<int_t, int_t>> snapPoints;
			screen->collectSlotSnapPoints(snapPoints);
			
			if (!snapPoints.empty())
			{
				// Find nearest slot above current cursor
				std::pair<int_t, int_t> bestSlot = snapPoints[0];
				float minDist = 1e9f;
				
				for (const auto& point : snapPoints)
				{
					if (point.second < static_cast<int_t>(controllerCursorY)) // Above cursor
					{
						float dx = static_cast<float>(point.first) - controllerCursorX;
						float dy = static_cast<float>(point.second) - controllerCursorY;
						float dist = dx * dx + dy * dy;
						if (dist < minDist)
						{
							minDist = dist;
							bestSlot = point;
						}
					}
				}
				
				if (minDist < 1e9f)
				{
					controllerCursorX = static_cast<float>(bestSlot.first);
					controllerCursorY = static_cast<float>(bestSlot.second);
				}
			}
		}
		else if (dpadDown && !prevDpadDown)
		{
			// Snap to nearest slot below current cursor
			std::vector<std::pair<int_t, int_t>> snapPoints;
			screen->collectSlotSnapPoints(snapPoints);
			
			if (!snapPoints.empty())
			{
				std::pair<int_t, int_t> bestSlot = snapPoints[0];
				float minDist = 1e9f;
				
				for (const auto& point : snapPoints)
				{
					if (point.second > static_cast<int_t>(controllerCursorY)) // Below cursor
					{
						float dx = static_cast<float>(point.first) - controllerCursorX;
						float dy = static_cast<float>(point.second) - controllerCursorY;
						float dist = dx * dx + dy * dy;
						if (dist < minDist)
						{
							minDist = dist;
							bestSlot = point;
						}
					}
				}
				
				if (minDist < 1e9f)
				{
					controllerCursorX = static_cast<float>(bestSlot.first);
					controllerCursorY = static_cast<float>(bestSlot.second);
				}
			}
		}
		else if (dpadLeft && !prevDpadLeft)
		{
			// Snap to nearest slot to the left
			std::vector<std::pair<int_t, int_t>> snapPoints;
			screen->collectSlotSnapPoints(snapPoints);
			
			if (!snapPoints.empty())
			{
				std::pair<int_t, int_t> bestSlot = snapPoints[0];
				float minDist = 1e9f;
				
				for (const auto& point : snapPoints)
				{
					if (point.first < static_cast<int_t>(controllerCursorX)) // Left of cursor
					{
						float dx = static_cast<float>(point.first) - controllerCursorX;
						float dy = static_cast<float>(point.second) - controllerCursorY;
						float dist = dx * dx + dy * dy;
						if (dist < minDist)
						{
							minDist = dist;
							bestSlot = point;
						}
					}
				}
				
				if (minDist < 1e9f)
				{
					controllerCursorX = static_cast<float>(bestSlot.first);
					controllerCursorY = static_cast<float>(bestSlot.second);
				}
			}
		}
		else if (dpadRight && !prevDpadRight)
		{
			// Snap to nearest slot to the right
			// Prefer slots with similar Y coordinates (e.g., output slot from crafting grid)
			std::vector<std::pair<int_t, int_t>> snapPoints;
			screen->collectSlotSnapPoints(snapPoints);
			
			if (!snapPoints.empty())
			{
				std::pair<int_t, int_t> bestSlot = snapPoints[0];
				float minDist = 1e9f;
				
				for (const auto& point : snapPoints)
				{
					if (point.first > static_cast<int_t>(controllerCursorX)) // Right of cursor
					{
						float dx = static_cast<float>(point.first) - controllerCursorX;
						float dy = static_cast<float>(point.second) - controllerCursorY;
						
						// Weight vertical distance more heavily to prefer slots at similar Y level
						// This makes horizontal navigation prioritize same-row slots
						float dist = dx * dx + dy * dy * 3.0f; // Weight Y distance 3x more
						
						if (dist < minDist)
						{
							minDist = dist;
							bestSlot = point;
						}
					}
				}
				
				if (minDist < 1e9f)
				{
					controllerCursorX = static_cast<float>(bestSlot.first);
					controllerCursorY = static_cast<float>(bestSlot.second);
				}
			}
		}
		
		// Clamp cursor to screen bounds
		if (controllerCursorX < 0.0f)
			controllerCursorX = 0.0f;
		if (controllerCursorX >= static_cast<float>(w))
			controllerCursorX = static_cast<float>(w - 1);
		if (controllerCursorY < 0.0f)
			controllerCursorY = 0.0f;
		if (controllerCursorY >= static_cast<float>(h))
			controllerCursorY = static_cast<float>(h - 1);
	}
	else
	{
		// Menu screen: D-pad or left stick navigates between buttons, no cursor visible
		// Move cursor to focused button center when button is focused
		
		// Get list of active, visible buttons
		std::vector<int_t> buttonIndices;
		for (size_t i = 0; i < screen->buttons.size(); ++i)
		{
			if (screen->buttons[i]->visible && screen->buttons[i]->active)
			{
				buttonIndices.push_back(static_cast<int_t>(i));
			}
		}
		
		if (!buttonIndices.empty())
		{
			int_t currentFocus = screen->getFocusedButtonIndex();
			
			// If no button is focused, focus the first one
			if (currentFocus < 0 || currentFocus >= static_cast<int_t>(screen->buttons.size()) || 
			    !screen->buttons[currentFocus]->visible || !screen->buttons[currentFocus]->active)
			{
				currentFocus = buttonIndices[0];
				screen->setFocusedButtonIndex(currentFocus);
			}
			
			// Get left stick input for button navigation (alternative to D-pad)
			float leftStickX = controllerInput->getLeftStickX();
			float leftStickY = controllerInput->getLeftStickY();
			const float stickDeadzone = 0.5f; // Threshold for stick input
			bool stickUp = leftStickY < -stickDeadzone;
			bool stickDown = leftStickY > stickDeadzone;
			bool stickLeft = leftStickX < -stickDeadzone;
			bool stickRight = leftStickX > stickDeadzone;
			
			// Update stick navigation repeat delay (Controlify-style: 10 ticks initial, 3 ticks repeat)
			// Check if stick was just pressed (wasn't pressed before)
			bool stickJustPressed = (stickUp && !prevStickUp) || (stickDown && !prevStickDown) || 
			                        (stickLeft && !prevStickLeft) || (stickRight && !prevStickRight);
			
			// Check if stick is currently pressed in any direction
			bool stickPressed = stickUp || stickDown || stickLeft || stickRight;
			
			if (stickJustPressed)
			{
				// Stick just pressed - reset to initial delay and allow immediate navigation
				stickNavRepeatDelay = STICK_NAV_INITIAL_DELAY;
			}
			else if (stickPressed)
			{
				// Stick is held - decrement delay each tick
				if (stickNavRepeatDelay > 0)
					stickNavRepeatDelay--;
			}
			else
			{
				// Stick released - reset delay
				stickNavRepeatDelay = 0;
			}
			
			// Check if navigation is allowed (Controlify-style: allow if delay elapsed OR first press)
			bool canStickNavigate = (stickNavRepeatDelay <= 0) || stickJustPressed;
			
			// Handle D-pad or left stick navigation
			if ((dpadUp && !prevDpadUp) || (stickUp && canStickNavigate))
			{
				// Find button above current focused button
				auto* currentButton = screen->buttons[currentFocus].get();
				int_t bestIndex = currentFocus;
				float bestY = -1e9f;
				
				for (int_t idx : buttonIndices)
				{
					if (idx == currentFocus) continue;
					auto* btn = screen->buttons[idx].get();
					if (btn->y < currentButton->y && btn->y > bestY)
					{
						bestY = static_cast<float>(btn->y);
						bestIndex = idx;
					}
				}
				
				if (bestIndex != currentFocus)
				{
					screen->setFocusedButtonIndex(bestIndex);
					currentFocus = bestIndex;
					// Reset delay after successful navigation (Controlify-style)
					stickNavRepeatDelay = STICK_NAV_REPEAT_DELAY;
				}
			}
			else if ((dpadDown && !prevDpadDown) || (stickDown && canStickNavigate))
			{
				// Find button below current focused button
				auto* currentButton = screen->buttons[currentFocus].get();
				int_t bestIndex = currentFocus;
				float bestY = 1e9f;
				
				for (int_t idx : buttonIndices)
				{
					if (idx == currentFocus) continue;
					auto* btn = screen->buttons[idx].get();
					if (btn->y > currentButton->y && btn->y < bestY)
					{
						bestY = static_cast<float>(btn->y);
						bestIndex = idx;
					}
				}
				
				if (bestIndex != currentFocus)
				{
					screen->setFocusedButtonIndex(bestIndex);
					currentFocus = bestIndex;
					// Reset delay after successful navigation (Controlify-style)
					stickNavRepeatDelay = STICK_NAV_REPEAT_DELAY;
				}
			}
			else if ((dpadLeft && !prevDpadLeft) || (stickLeft && canStickNavigate))
			{
				// Check if focused button is a slider - if so, adjust slider instead of navigating (Controlify-style)
				if (currentFocus >= 0 && currentFocus < static_cast<int_t>(screen->buttons.size()))
				{
					auto* focusedButton = screen->buttons[currentFocus].get();
					SlideButton* slideButton = dynamic_cast<SlideButton*>(focusedButton);
					if (slideButton != nullptr && slideButton->active)
					{
						// This is a slider - adjust value instead of navigating (Controlify-style)
						// Use repeat delay for slider adjustment (Controlify: 15 ticks initial, 1 tick repeat)
						bool sliderJustPressed = (dpadLeft && !prevSliderDpadLeft) || (stickLeft && canStickNavigate);
						bool canAdjustSlider = (sliderAdjustRepeatDelay >= SLIDER_ADJUST_INITIAL_DELAY) || sliderJustPressed;
						
						if (canAdjustSlider)
						{
							// Step size: 1.0 / (width - 8) for smooth adjustment, matching SlideButton's internal step
							float step = 1.0f / static_cast<float>(focusedButton->w - 8);
							slideButton->value -= step;
							if (slideButton->value < 0.0f) slideButton->value = 0.0f;
							
							// Update the option if it's linked to one (standard sliders)
							Options::Option::Element* option = slideButton->getOption();
							if (option != nullptr)
							{
								options.set(*option, slideButton->value);
								slideButton->msg = options.getMessage(*option);
								options.save();
							}
							// Custom sliders (like ControllerSettingsScreen) will be handled in their tick() method
							
							if (sliderJustPressed)
								sliderAdjustRepeatDelay = 0;
							else
								sliderAdjustRepeatDelay = 0; // Reset to 0, will increment next tick
						}
						
						if (dpadLeft || stickLeft)
							sliderAdjustRepeatDelay++;
						else
							sliderAdjustRepeatDelay = 0;
						
						prevSliderDpadLeft = dpadLeft;
						// Don't navigate - slider handled the input
					}
					else
					{
						// Not a slider - navigate to button on the left
						sliderAdjustRepeatDelay = 0; // Reset slider delay when not on slider
						auto* currentButton = screen->buttons[currentFocus].get();
						int_t bestIndex = currentFocus;
						float bestX = -1e9f;
						
						for (int_t idx : buttonIndices)
						{
							if (idx == currentFocus) continue;
							auto* btn = screen->buttons[idx].get();
							if (btn->x < currentButton->x && btn->x > bestX)
							{
								bestX = static_cast<float>(btn->x);
								bestIndex = idx;
							}
						}
						
						if (bestIndex != currentFocus)
						{
							screen->setFocusedButtonIndex(bestIndex);
							currentFocus = bestIndex;
							// Reset delay after successful navigation (Controlify-style)
							stickNavRepeatDelay = STICK_NAV_REPEAT_DELAY;
						}
					}
				}
			}
			else if ((dpadRight && !prevDpadRight) || (stickRight && canStickNavigate))
			{
				// Check if focused button is a slider - if so, adjust slider instead of navigating (Controlify-style)
				if (currentFocus >= 0 && currentFocus < static_cast<int_t>(screen->buttons.size()))
				{
					auto* focusedButton = screen->buttons[currentFocus].get();
					SlideButton* slideButton = dynamic_cast<SlideButton*>(focusedButton);
					if (slideButton != nullptr && slideButton->active)
					{
						// This is a slider - adjust value instead of navigating (Controlify-style)
						// Use repeat delay for slider adjustment (Controlify: 15 ticks initial, 1 tick repeat)
						bool sliderJustPressed = (dpadRight && !prevSliderDpadRight) || (stickRight && canStickNavigate);
						bool canAdjustSlider = (sliderAdjustRepeatDelay >= SLIDER_ADJUST_INITIAL_DELAY) || sliderJustPressed;
						
						if (canAdjustSlider)
						{
							float step = 1.0f / static_cast<float>(focusedButton->w - 8);
							slideButton->value += step;
							if (slideButton->value > 1.0f) slideButton->value = 1.0f;
							
							// Update the option if it's linked to one (standard sliders)
							Options::Option::Element* option = slideButton->getOption();
							if (option != nullptr)
							{
								options.set(*option, slideButton->value);
								slideButton->msg = options.getMessage(*option);
								options.save();
							}
							// Custom sliders (like ControllerSettingsScreen) will be handled in their tick() method
							
							if (sliderJustPressed)
								sliderAdjustRepeatDelay = 0;
							else
								sliderAdjustRepeatDelay = 0; // Reset to 0, will increment next tick
						}
						
						if (dpadRight || stickRight)
							sliderAdjustRepeatDelay++;
						else
							sliderAdjustRepeatDelay = 0;
						
						prevSliderDpadRight = dpadRight;
						// Don't navigate - slider handled the input
					}
					else
					{
						// Not a slider - navigate to button on the right
						sliderAdjustRepeatDelay = 0; // Reset slider delay when not on slider
						auto* currentButton = screen->buttons[currentFocus].get();
						int_t bestIndex = currentFocus;
						float bestX = 1e9f;
						
						for (int_t idx : buttonIndices)
						{
							if (idx == currentFocus) continue;
							auto* btn = screen->buttons[idx].get();
							if (btn->x > currentButton->x && btn->x < bestX)
							{
								bestX = static_cast<float>(btn->x);
								bestIndex = idx;
							}
						}
						
						if (bestIndex != currentFocus)
						{
							screen->setFocusedButtonIndex(bestIndex);
							currentFocus = bestIndex;
							// Reset delay after successful navigation (Controlify-style)
							stickNavRepeatDelay = STICK_NAV_REPEAT_DELAY;
						}
					}
				}
			}
			else
			{
				// D-pad/stick released - reset slider adjustment delay
				sliderAdjustRepeatDelay = 0;
				prevSliderDpadLeft = dpadLeft;
				prevSliderDpadRight = dpadRight;
			}
			
			// Store previous stick state
			prevStickUp = stickUp;
			prevStickDown = stickDown;
			prevStickLeft = stickLeft;
			prevStickRight = stickRight;
			
			// Move cursor to focused button center (for button click detection)
			if (currentFocus >= 0 && currentFocus < static_cast<int_t>(screen->buttons.size()))
			{
				auto* focusedButton = screen->buttons[currentFocus].get();
				controllerCursorX = static_cast<float>(focusedButton->x + focusedButton->w / 2);
				controllerCursorY = static_cast<float>(focusedButton->y + focusedButton->h / 2);
			}
		}
	}
	
	// Store previous D-pad state
	prevDpadUp = dpadUp;
	prevDpadDown = dpadDown;
	prevDpadLeft = dpadLeft;
	prevDpadRight = dpadRight;
	
	// Handle controller button clicks (Controlify-style mappings)
	// A button (South) = left click (primary action)
	if (controllerInput->wasButtonJustPressed(SDL_GAMEPAD_BUTTON_SOUTH))
	{
		int_t xm = static_cast<int_t>(controllerCursorX);
		int_t ym = static_cast<int_t>(controllerCursorY);
		screen->mouseClicked(xm, ym, 0); // Left click
	}
	
	// B button (East) behavior depends on screen type and whether player is carrying an item
	if (controllerInput->wasButtonJustPressed(SDL_GAMEPAD_BUTTON_EAST))
	{
		if (hasSlots)
		{
			// Container screen (inventory, chest, etc.)
			// Check if player is carrying an item (picked up an item)
			ItemStack *carried = nullptr;
			if (player != nullptr)
				carried = player->inventory.getCarried();
			
			if (carried != nullptr && !carried->isEmpty())
			{
				// Player is carrying an item - B button acts like right-click (place one item)
				int_t xm = static_cast<int_t>(controllerCursorX);
				int_t ym = static_cast<int_t>(controllerCursorY);
				screen->mouseClicked(xm, ym, 1); // Right click
			}
			else
			{
				// No item being carried - B button closes screen (console edition style)
			setScreen(nullptr);
			grabMouse();
			}
		}
		else
		{
			// Menu screen: B button = right click (secondary action)
			int_t xm = static_cast<int_t>(controllerCursorX);
			int_t ym = static_cast<int_t>(controllerCursorY);
			screen->mouseClicked(xm, ym, 1); // Right click
		}
	}
}

void Minecraft::handleControllerGUIClick(ControllerInput* controllerInput)
{
	// This is called from updateControllerCursor, so it's already handled there
	// Kept as a separate function for future extensibility
}

void Minecraft::renderControllerCursor(int_t xm, int_t ym, float a)
{
	if (!controllerCursorVisible || controllerCursorTexture < 0)
		return;

	// Render cursor texture using blit-style rendering (same pattern as GuiComponent::blit())
	// Note: GUI matrix is already set up in setupGuiScreen() with glOrtho and glTranslatef(-2000.0f)
	glDisable(GL_DEPTH_TEST); // Disable depth test so cursor always renders on top
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // White, full alpha
	textures.bind(controllerCursorTexture);
	
	// Draw cursor sprite (32x32 texture, centered at cursor position, scaled to 16x16)
	// Use blit-style rendering but with custom UV for 32x32 texture instead of 256x256
	const float cursorSize = 16.0f; // Half of texture size (Controlify uses 0.5f scale)
	const float us = 1.0f / 32.0f; // UV scale for 32x32 texture
	const float vs = 1.0f / 32.0f;
	Tesselator &t = Tesselator::instance;
	t.begin();
	// Follow blit() vertex order: bottom-left, bottom-right, top-right, top-left
	t.vertexUV(static_cast<float>(xm) - cursorSize, static_cast<float>(ym) + cursorSize, -1999.0f, 0.0f * us, 32.0f * vs);
	t.vertexUV(static_cast<float>(xm) + cursorSize, static_cast<float>(ym) + cursorSize, -1999.0f, 32.0f * us, 32.0f * vs);
	t.vertexUV(static_cast<float>(xm) + cursorSize, static_cast<float>(ym) - cursorSize, -1999.0f, 32.0f * us, 0.0f * vs);
	t.vertexUV(static_cast<float>(xm) - cursorSize, static_cast<float>(ym) - cursorSize, -1999.0f, 0.0f * us, 0.0f * vs);
	t.end();
	
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST); // Re-enable depth test
}

void Minecraft::reloadSound()
{
	std::cout << "FORCING RELOAD!\n";
	// TODO
}

bool Minecraft::isOnline()
{
	return level != nullptr && level->isOnline;
}

void Minecraft::selectLevel(const jstring &name)
{
	setLevel(nullptr);

	std::unique_ptr<File> saves(File::open(*getWorkingDirectory(), u"saves"));
	std::shared_ptr<Level> new_level = Util::make_shared<Level>(saves.release(), name);
	if (new_level->isNew)
		setLevel(new_level, u"Generating level");
	else
		setLevel(new_level, u"Loading level");
}

void Minecraft::toggleDimension()
{
	// Alpha 1.2.6: toggleDimension() - only works in single-player
	// In multiplayer, dimension changes are handled by the server via Packet9 (respawn packet)
	// Java: Minecraft.func_6237_k() (Minecraft.java:1157-1195)
	if (isOnline())
	{
		// Multiplayer: Don't toggle dimension locally, server will send Packet9
		// The server handles the dimension change and sends a respawn packet
		return;
	}
	
	// Single-player: Handle dimension toggle locally (Alpha 1.2.6: Minecraft.java:1157-1195)
	if (player == nullptr || level == nullptr)
	{
		return;
	}
	
	std::cout << "Toggling dimension!!" << std::endl;
	
	// Toggle dimension
	if (player->dimension == -1)
		player->dimension = 0;
	else
		player->dimension = -1;
	
	// Remove player from old level
	level->removeEntity(player);
	player->removed = false;  // Alpha: this.thePlayer.isDead = false (Minecraft.java:1166)
	
	// Calculate new position (scale by 8.0 for dimension conversion)
	double var1 = player->x;
	double var3 = player->z;
	double var5 = 8.0;
	
	if (player->dimension == -1)
	{
		// Entering Nether: divide coordinates by 8
		var1 /= var5;
		var3 /= var5;
		player->moveTo(var1, player->y, var3, player->yRot, player->xRot);
		if (player->isAlive())
		{
			level->tick(player, false);  // Alpha: this.theWorld.func_4084_a(this.thePlayer, false) (Minecraft.java:1176)
		}
		
		// Create new level with Hell dimension
		std::shared_ptr<Level> newLevel = std::make_shared<Level>(*level, std::make_shared<HellDimension>());
		setLevel(newLevel, u"Entering the Nether", player);
	}
	else
	{
		// Leaving Nether: multiply coordinates by 8
		var1 *= var5;
		var3 *= var5;
		player->moveTo(var1, player->y, var3, player->yRot, player->xRot);
		if (player->isAlive())
		{
			level->tick(player, false);  // Alpha: this.theWorld.func_4084_a(this.thePlayer, false) (Minecraft.java:1185)
		}
		
		// Create new level with normal dimension
		std::shared_ptr<Level> newLevel = std::make_shared<Level>(*level, std::make_shared<Dimension>());
		setLevel(newLevel, u"Leaving the Nether", player);
	}
	
	// Update player position (level reference is updated by setLevel())
	// Note: player->level is a reference, so it's automatically updated when setLevel() is called
	player->moveTo(var1, player->y, var3, player->yRot, player->xRot);
	if (level != nullptr && player != nullptr)
	{
		level->tick(player, false);  // Alpha: this.theWorld.func_4084_a(this.thePlayer, false) (Minecraft.java:1192)
	}
	
	// Force portal placement (Alpha: new PortalForcer().force(this.level, this.player) - Minecraft.java:1194)
	// Note: PortalForcer is not implemented in Alpha 1.2.6, so we skip this step
}

void Minecraft::setLevel(std::shared_ptr<Level> level)
{
	setLevel(level, u"");
}

void Minecraft::setLevel(std::shared_ptr<Level> level, const jstring &title)
{
	setLevel(level, title, nullptr);
}

void Minecraft::setLevel(std::shared_ptr<Level> level, const jstring &title, std::shared_ptr<Player> player)
{
	progressRenderer->progressStart(title);
	progressRenderer->progressStage(u"");

	if (this->level != nullptr)
		this->level->forceSave(progressRenderer);
	this->level = level;

	std::cout << "Player is " << this->player << '\n';

	if (level != nullptr)
	{
		// Ensure gameMode is initialized
		if (gameMode == nullptr)
		{
			gameMode = Util::make_shared<SurvivalMode>(*this);
		}
		gameMode->initLevel(level);

		if (!isOnline())
		{
			if (player == nullptr)
				this->player = nullptr; // level->findSubclassOf<LocalPlayer>();
		}
		else if (this->player != nullptr)
		{
			this->player->resetPos();
			if (level != nullptr)
				level->addEntity(this->player);
		}

		if (!level->isOnline)
			prepareLevel(title);

		std::cout << "Player is now " << this->player << '\n';

		if (this->player == nullptr)
		{
			this->player = std::static_pointer_cast<LocalPlayer>(gameMode->createPlayer(*level));
			this->player->resetPos();
			gameMode->initPlayer(this->player);
		}
		// Initialize input (will use controller if available)
		this->player->updatePlayerInput();

		levelRenderer.setLevel(level);

		// Beta: Set particle engine level (Minecraft.java:1292-1294)
		particleEngine.setLevel(level.get());

		gameMode->adjustPlayer(this->player);
		if (player != nullptr)
			level->clearLoadedPlayerData();

		std::shared_ptr<ChunkSource> chunkSource = level->getChunkSource();
		if (chunkSource->isChunkCache())
		{
			ChunkCache &chunkCache = static_cast<ChunkCache &>(*chunkSource);
			int_t x = Mth::floor(static_cast<float>(static_cast<int_t>(this->player->x))) >> 4;
			int_t z = Mth::floor(static_cast<float>(static_cast<int_t>(this->player->z))) >> 4;
			chunkCache.centerOn(x, z);
		}

		level->loadPlayer(this->player);

		if (level->isNew)
			level->forceSave(progressRenderer);
	}
	else
	{
		this->player = nullptr;
	}
}

void Minecraft::prepareLevel(const jstring &title)
{
	progressRenderer->progressStart(title);
	progressRenderer->progressStage(u"Building terrain");

	short_t radius = 128;
	int_t i = 0;
	int_t max = radius * 2 / 16 + 1;
	max *= max;

	std::shared_ptr<ChunkSource> source = level->getChunkSource();
	int_t xSpawn = level->xSpawn;
	int_t zSpawn = level->zSpawn;
	if (player != nullptr)
	{
		xSpawn = static_cast<int_t>(player->x);
		zSpawn = static_cast<int_t>(player->z);
	}

	if (source->isChunkCache())
	{
		ChunkCache &cache = static_cast<ChunkCache &>(*source);
		cache.centerOn(xSpawn >> 4, zSpawn >> 4);
	}

	for (int_t x = -radius; x <= radius; x += 16)
	{
		for (int_t z = -radius; z <= radius; z += 16)
		{
			progressRenderer->progressStagePercentage(i++ * 100 / max);
			level->getTile(xSpawn + x, 64, zSpawn + z);

			while (level->updateLights());
		}
	}

	progressRenderer->progressStage(u"Simulating world for a bit");
}

jstring Minecraft::gatherStats1()
{
	return levelRenderer.gatherStats1();
}

jstring Minecraft::gatherStats2()
{
	return levelRenderer.gatherStats2();
}

jstring Minecraft::gatherStats4()
{
	return level->gatherChunkSourceStats();
}

jstring Minecraft::gatherStats3()
{
	return u"P: 0. T: " + level->gatherStats();
}

void Minecraft::respawnPlayer()
{
	// Beta: Minecraft.respawnPlayer() - handles player respawn (Minecraft.java:1399-1430)
	// Safety check: ensure level and dimension are valid
	if (level == nullptr || level->dimension == nullptr)
	{
		return;  // Can't respawn without a valid level
	}
	
	if (!level->dimension->mayRespawn())  // Beta: if (!this.level.dimension.mayRespawn()) (Minecraft.java:1400)
		toggleDimension();  // Beta: this.toggleDimension() (Minecraft.java:1401)
	
	int_t xSpawn = level->xSpawn;  // Beta: int var1 = this.level.xSpawn (Minecraft.java:1404)
	int_t zSpawn = level->zSpawn;  // Beta: int var2 = this.level.zSpawn (Minecraft.java:1405)
	std::shared_ptr<ChunkSource> chunkSource = level->getChunkSource();  // Beta: ChunkSource var3 = this.level.getChunkSource() (Minecraft.java:1406)
	if (chunkSource->isChunkCache())  // Beta: if (var3 instanceof ChunkCache) (Minecraft.java:1407)
	{
		ChunkCache &chunkCache = static_cast<ChunkCache &>(*chunkSource);  // Beta: ChunkCache var4 = (ChunkCache)var3 (Minecraft.java:1408)
		chunkCache.centerOn(xSpawn >> 4, zSpawn >> 4);  // Beta: var4.centerOn(var1 >> 4, var2 >> 4) (Minecraft.java:1409)
	}
	
	level->validateSpawn();  // Beta: this.level.validateSpawn() (Minecraft.java:1412)
	level->entitiesToRemove.clear();  // Beta: this.level.removeAllPendingEntityRemovals() (Minecraft.java:1413) - clear entitiesToRemove
	int_t entityId = 0;  // Beta: int var5 = 0 (Minecraft.java:1414)
	if (player != nullptr)  // Beta: if (this.player != null) (Minecraft.java:1415)
	{
		entityId = player->entityId;  // Beta: var5 = this.player.entityId (Minecraft.java:1416)
		level->removeEntity(player);  // Beta: this.level.removeEntity(this.player) (Minecraft.java:1417)
	}
	
	// newb12: Clear loaded player data before respawning so respawn uses spawn point, not saved death position
	level->clearLoadedPlayerData();
	
	player = std::static_pointer_cast<LocalPlayer>(gameMode->createPlayer(*level));  // Beta: this.player = (LocalPlayer)this.gameMode.createPlayer(this.level) (Minecraft.java:1420)
	player->resetPos();  // Beta: this.player.resetPos() (Minecraft.java:1421)
	gameMode->initPlayer(player);  // Beta: this.gameMode.initPlayer(this.player) (Minecraft.java:1422)
	level->loadPlayer(player);  // Beta: this.level.loadPlayer(this.player) (Minecraft.java:1423)
	// Initialize input (will use controller if available)
	player->updatePlayerInput();  // Beta: this.player.input = new KeyboardInput(this.options) (Minecraft.java:1424)
	player->entityId = entityId;  // Beta: this.player.entityId = var5 (Minecraft.java:1425)
	gameMode->adjustPlayer(player);  // Beta: this.gameMode.adjustPlayer(this.player) (Minecraft.java:1426)
	prepareLevel(u"Respawning");  // Beta: this.prepareLevel("Respawning") (Minecraft.java:1427)
	// Alpha 1.2.6: Clear death screen after respawn (Minecraft.java:1431-1433)
	if (dynamic_cast<DeathScreen *>(screen.get()) != nullptr)  // Beta: if (this.screen instanceof DeathScreen) (Minecraft.java:1431)
		setScreen(nullptr);  // Beta: this.displayGuiScreen(null) (Minecraft.java:1432)
}

// Beta: fileDownloaded(String name, File file) - categorizes and loads sounds (Minecraft.java:1352-1367)
void Minecraft::fileDownloaded(const jstring &name, File *file)
{
	if (!file || !file->exists())
		return;
	
	// Beta: Extract category prefix (e.g., "sound", "newsound", "streaming", "music", "newmusic")
	size_t slashPos = name.find(u"/");
	if (slashPos == jstring::npos)
		return;
	
	jstring category = name.substr(0, slashPos);
	jstring soundName = name.substr(slashPos + 1);  // Beta: Strip category prefix before calling add()
	
	// Beta: Convert file path to string for sound engine
	std::string filePath = String::toUTF8(file->toString());
	
	// Beta: Categorize and load sounds based on directory prefix (Minecraft.java:1356-1366)
	if (category == u"sound" || category == u"newsound")
	{
		// Beta: Regular sounds (Minecraft.java:1357-1359)
		soundEngine.add(soundName, filePath);
	}
	else if (category == u"streaming")
	{
		// Beta: Streaming sounds (Minecraft.java:1361)
		soundEngine.addStreaming(soundName, filePath);
	}
	else if (category == u"music" || category == u"newmusic")
	{
		// Beta: Music files (Minecraft.java:1363-1365)
		soundEngine.addMusic(soundName, filePath);
	}
}

// Beta: loadAllSounds() - recursively loads all sounds from resource directory (BackgroundDownloader.java:66-80)
void Minecraft::loadAllSounds(File *dir, const jstring &prefix)
{
	if (!dir || !dir->exists() || !dir->isDirectory())
		return;
	
	// Beta: List all files in directory (BackgroundDownloader.java:67)
	std::vector<std::unique_ptr<File>> files = dir->listFiles();
	
	// Beta: Recursively process all files (BackgroundDownloader.java:69-79)
	for (auto &file : files)
	{
		if (!file)
			continue;
		
		if (file->isDirectory())
		{
			// Beta: Recursively load subdirectories (BackgroundDownloader.java:70-71)
			jstring newPrefix = prefix + file->getName() + u"/";
			loadAllSounds(file.get(), newPrefix);
		}
		else if (file->isFile())
		{
			// Beta: Only load sound files (.ogg, .mus, .wav) - matching Beta 1.2 codec support
			jstring fileName = file->getName();
			bool isSoundFile = false;
			if (fileName.length() >= 4)
			{
				jstring ext = fileName.substr(fileName.length() - 4);
				if (ext == u".ogg" || ext == u".mus" || ext == u".wav")
					isSoundFile = true;
			}
			
			if (isSoundFile)
			{
				// Beta: Load file and call fileDownloaded (BackgroundDownloader.java:73-77)
				try
				{
					jstring fullName = prefix + fileName;
					fileDownloaded(fullName, file.get());
				}
				catch (const std::exception &e)
				{
					std::cerr << "Failed to add " << String::toUTF8(prefix + fileName) << ": " << e.what() << std::endl;
				}
			}
		}
	}
}

void Minecraft::start(const jstring *name, const jstring *sessionId)
{
	startAndConnectTo(name, sessionId, nullptr);
}

void Minecraft::startAndConnectTo(const jstring *name, const jstring *sessionId, const jstring *ip)
{
	std::unique_ptr<Minecraft> minecraft = Util::make_unique<Minecraft>(854, 480, false);

	minecraft->serverDomain = u"www.minecraft.net";
	if (name != nullptr && sessionId != nullptr)
		minecraft->user = Util::make_unique<User>(*name, *sessionId);
	else
		minecraft->user = Util::make_unique<User>(u"Player" + String::fromUTF8(std::to_string(System::currentTimeMillis() % 1000)), u"");
	
	if (ip != nullptr)
	{
		// TODO
	}

	minecraft->run();
	// minecraft.release(); // TODO: fix destructor order stuff
}
