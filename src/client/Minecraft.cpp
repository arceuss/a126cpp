#include "Minecraft.h"

#include <iostream>
#include <stdexcept>
#include <thread>
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
#include "client/title/TitleScreen.h"
#include "client/player/KeyboardInput.h"
#include "client/player/EntityClientPlayerMP.h"

#include "client/gamemode/SurvivalMode.h"

#include "world/phys/Vec3.h"
#include "world/phys/AABB.h"
#include "world/phys/HitResult.h"
#include "world/level/chunk/ChunkCache.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
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

	// TODO
	// EntityRenderDispatcher.instance.itemInHandRenderer = new ItemInHandRenderer(this);

	workingDirectory = getWorkingDirectory();

	options.open(workingDirectory.get());
	texturePackRepository.updateListAndSelect();

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

			if (options.limitFramerate)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));

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

	// Tick screen
	if (screen != nullptr)
		lastClickTick = ticks + 10000;
	if (screen != nullptr)
	{
		std::shared_ptr<Screen> screen_ptr = screen; // Ensure that the screen is not deleted while processing events
		screen->updateEvents();
		if (screen != nullptr)
			screen->tick();
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
		}

		handleMouseDown(0, (screen == nullptr && lwjgl::Mouse::isButtonDown(0) && mouseGrabbed));
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

	lastTickTime = System::currentTimeMillis();
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
		
		// Create new level with Hell dimension (dimension ID -1)
		std::shared_ptr<Level> newLevel = std::make_shared<Level>(*level, -1);
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
		
		// Create new level with normal dimension (dimension ID 0)
		std::shared_ptr<Level> newLevel = std::make_shared<Level>(*level, 0);
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
		this->player->input = Util::make_unique<KeyboardInput>(options);

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
	player->input = Util::make_unique<KeyboardInput>(options);  // Beta: this.player.input = new KeyboardInput(this.options) (Minecraft.java:1424)
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
