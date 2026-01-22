#include "client/renderer/GameRenderer.h"

#include "client/Minecraft.h"
#include "client/Lighting.h"
#include "client/gui/ScreenSizeCalculator.h"

#include "client/renderer/culling/FrustumCuller.h"

#include "world/level/chunk/ChunkCache.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"

#include "util/Mth.h"
#include "util/GLU.h"

#include "lwjgl/Display.h"
#include "lwjgl/GLContext.h"
#include "lwjgl/Keyboard.h"

#include "java/System.h"
#include "world/phys/Vec3.h"

#include "OpenGL.h"

GameRenderer::GameRenderer(Minecraft &mc) : mc(mc), itemInHandRenderer(ItemInHandRenderer(mc))
{

}

void GameRenderer::tick()
{
	fogBrO = fogBr;
	float brightness = mc.level->getBrightness(Mth::floor(mc.player->x), Mth::floor(mc.player->y), Mth::floor(mc.player->z));
	float dist = (3.0f - mc.options.viewDistance) / 3.0f;
	float fogBrTarget = brightness * (1.0f - dist) + dist;
	fogBr += (fogBrTarget - fogBr) * 0.1f;

	ticks++;

	itemInHandRenderer.tick();

	// TODO tickRain rain
}

void GameRenderer::pick(float a)
{
	if (mc.player == nullptr)
		return;

	double range = mc.gameMode->getPickRange();
	mc.hitResult = mc.player->pick(range, a);

	double hitRange = range;
	Vec3 *playerPos = mc.player->getPos(a);
	if (mc.hitResult.type != HitResult::Type::NONE)
		hitRange = mc.hitResult.pos->distanceTo(*playerPos);

	if (mc.gameMode->isCreativeMode())
	{
		range = 32.0;
		hitRange = 32.0;
	}
	else
	{
		if (hitRange > 3)
			hitRange = 3;
		range = hitRange;
	}

	Vec3 *look = mc.player->getViewVector(a);
	Vec3 *to = playerPos->add(look->x * range, look->y * range, look->z * range);
	hovered = nullptr;

	float skin = 1.0f;
	const auto &es = mc.level->getEntities(mc.player.get(), *mc.player->bb.expand(look->x * range, look->y * range, look->z * range)->grow(skin, skin, skin));

	double closestDist = 0.0;

	for (auto &entity : es)
	{
		if (entity->isPickable())
		{
			float radius = entity->getPickRadius();
			AABB *bb = entity->bb.grow(radius, radius, radius);
			HitResult hr = bb->clip(*playerPos, *to);
			if (bb->contains(*playerPos))
			{
				if (0.0 < closestDist || closestDist == 0.0)
				{
					hovered = entity;
					closestDist = 0.0;
				}
			}
			else if (hr.type != HitResult::Type::NONE)
			{
				double clipDist = playerPos->distanceTo(*hr.pos);
				if (clipDist < closestDist || closestDist == 0.0)
				{
					hovered = entity;
					closestDist = clipDist;
				}
			}
		}
	}

	if (hovered != nullptr && !mc.gameMode->isCreativeMode())
		mc.hitResult = HitResult(hovered);
}

float GameRenderer::getFov(float a)
{
	auto &player = mc.player;

	float result = 70.0f;
	
	// Apply FOV setting: 0.0 = Normal (70), 1.0 = Quake Pro (110), else 70-110 range
	result += mc.options.fovSetting * 40.0f;
	
	// TODO: Apply FOV multiplier from item in hand (fovModifierHand)
	// TODO water material

	if (player->health <= 0)
	{
		float time = player->deathTime + a;
		result /= (1.0f - 500.0f / (time + 500.0f)) * 2.0f + 1.0f;
	}

	return result;
}

float GameRenderer::getFovForItemInHand(float a)
{
	auto &player = mc.player;

	// Item in hand uses base 70 FOV (no fovSetting multiplier) to prevent stretching
	float result = 70.0f;
	
	// TODO: Apply FOV multiplier from item in hand (fovModifierHand)
	// TODO water material

	if (player->health <= 0)
	{
		float time = player->deathTime + a;
		result /= (1.0f - 500.0f / (time + 500.0f)) * 2.0f + 1.0f;
	}

	return result;
}

void GameRenderer::bobHurt(float a)
{
	// Beta: GameRenderer.bobHurt(float var1) - damage tilt camera effect (GameRenderer.java:143-160)
	if (mc.player == nullptr)
		return;
	
	auto &player = *mc.player;
	float hurtTimeRemaining = (float)player.hurtTime - a;  // Beta: float var3 = (float)var2.hurtTime - var1 (GameRenderer.java:145)
	
	// Beta: Death camera tilt (GameRenderer.java:147-150)
	if (player.health <= 0)  // Beta: if (var2.health <= 0) (GameRenderer.java:147)
	{
		float deathTime = (float)player.deathTime + a;  // Beta: float var4 = (float)var2.deathTime + var1 (GameRenderer.java:148)
		glRotatef(40.0f - 8000.0f / (deathTime + 200.0f), 0.0f, 0.0f, 1.0f);  // Beta: GL11.glRotatef(40.0F - 8000.0F / (var4 + 200.0F), 0.0F, 0.0F, 1.0F) (GameRenderer.java:149)
	}
	
	// Beta: Hurt camera tilt (GameRenderer.java:152-159)
	if (hurtTimeRemaining >= 0.0f)  // Beta: if (var3 >= 0.0F) (GameRenderer.java:152)
	{
		hurtTimeRemaining /= (float)player.hurtDuration;  // Beta: var3 /= (float)var2.hurtDuration (GameRenderer.java:153)
		hurtTimeRemaining = Mth::sin(hurtTimeRemaining * hurtTimeRemaining * hurtTimeRemaining * hurtTimeRemaining * Mth::PI);  // Beta: var3 = Mth.sin(var3 * var3 * var3 * var3 * (float)Math.PI) (GameRenderer.java:154)
		float hurtDir = player.hurtDir;  // Beta: float var4 = var2.hurtDir (GameRenderer.java:155)
		glRotatef(-hurtDir, 0.0f, 1.0f, 0.0f);  // Beta: GL11.glRotatef(-var4, 0.0F, 1.0F, 0.0F) (GameRenderer.java:156)
		glRotatef(-hurtTimeRemaining * 14.0f, 0.0f, 0.0f, 1.0f);  // Beta: GL11.glRotatef(-var3 * 14.0F, 0.0F, 0.0F, 1.0F) (GameRenderer.java:157)
		glRotatef(hurtDir, 0.0f, 1.0f, 0.0f);  // Beta: GL11.glRotatef(var4, 0.0F, 1.0F, 0.0F) (GameRenderer.java:158)
	}
}

void GameRenderer::bobView(float a)
{
	if (mc.options.thirdPersonView > 0) return;

	auto &localPlayer = *mc.player;
	float walkDist = localPlayer.walkDist + (localPlayer.walkDist - localPlayer.walkDistO) * a;
	float bob = localPlayer.oBob + (localPlayer.bob - localPlayer.oBob) * a;
	float tilt = localPlayer.oTilt + (localPlayer.tilt - localPlayer.oTilt) * a;
	glTranslatef(Mth::sin(walkDist * Mth::PI) * bob * 0.5F, -std::abs(Mth::cos(walkDist * Mth::PI) * bob), 0.0F);
	glRotatef(Mth::sin(walkDist * Mth::PI) * bob * 3.0F, 0.0F, 0.0F, 1.0F);
	glRotatef(std::abs(Mth::cos(walkDist * Mth::PI + 0.2F) * bob) * 5.0F, 1.0F, 0.0F, 0.0F);
	glRotatef(tilt, 1.0F, 0.0F, 0.0F);
}

void GameRenderer::moveCameraToPlayer(float a)
{
	auto &player = *mc.player;
	double xOff = player.xOld + (player.x - player.xOld) * a;
	double yOff = player.yOld + (player.y - player.yOld) * a;
	double zOff = player.zOld + (player.z - player.zOld) * a;

	if (mc.options.thirdPersonView > 0)
	{
		double distance = 4.0;
		float yRot = player.yRot;
		float xRot = player.xRot;
		if (mc.options.thirdPersonView == 2)  // Alpha: front view - rotate pitch by 180 (EntityRenderer.java:224-226)
		{
			xRot += 180.0f;
			distance += 2.0f;
		}

		double vx = -Mth::sin(yRot / 180.0f * Mth::PI) * Mth::cos(xRot / 180.0f * Mth::PI) * distance;
		double vz = Mth::cos(yRot / 180.0f * Mth::PI) * Mth::cos(xRot / 180.0f * Mth::PI) * distance;
		double vy = -Mth::sin(xRot / 180.0f * Mth::PI) * distance;

		for (int_t i = 0; i < 8; i++)
		{
			float txo = (i & 1) * 2 - 1;
			float tyo = ((i >> 1) & 1) * 2 - 1;
			float tzo = ((i >> 2) & 1) * 2 - 1;
			txo *= 0.1F;
			tyo *= 0.1F;
			tzo *= 0.1F;
			HitResult hr = mc.level->clip(*Vec3::newTemp(xOff + txo, yOff + tyo, zOff + tzo), *Vec3::newTemp(xOff - vx + txo + tzo, yOff - vy + tyo, zOff - vz + tzo));
			if (hr.type != HitResult::Type::NONE)
			{
				double hitd = hr.pos->distanceTo(*Vec3::newTemp(xOff, yOff, zOff));
				if (hitd < distance) distance = hitd;
			}
		}

		if (mc.options.thirdPersonView == 2)  // Alpha: front view - rotate Y by 180 (EntityRenderer.java:230-232)
			glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

		glRotatef(player.xRot - xRot, 1.0f, 0.0f, 0.0f);
		glRotatef(player.yRot - yRot, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, 0.0f, -distance);
		glRotatef(yRot - player.yRot, 0.0f, 1.0f, 0.0f);
		glRotatef(xRot - player.xRot, 1.0f, 0.0f, 0.0f);
	}
	else
	{
		glTranslatef(0.0f, 0.0f, -0.1f);
	}

	glRotatef(player.xRotO + (player.xRot - player.xRotO) * a, 1.0f, 0.0f, 0.0f);
	glRotatef(player.yRotO + (player.yRot - player.yRotO) * a + 180.0f, 0.0f, 1.0f, 0.0f);
}

void GameRenderer::setupCamera(float a, int_t eye)
{
	renderDistance = 256 >> mc.options.viewDistance;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float eyeDist = 0.07f;
	if (mc.options.anaglyph3d)
		glTranslatef(-(eye * 2 - 1) * eyeDist, 0.0f, 0.0f);

	if (zoom != 1.0)
	{
		glTranslatef(zoom_x, -zoom_y, 0.0f);
		glScaled(zoom, zoom, 1.0);
		gluPerspective(getFov(a), static_cast<float>(mc.width) / static_cast<float>(mc.height), 0.05f, renderDistance);
	}
	else
	{
		gluPerspective(getFov(a), static_cast<float>(mc.width) / static_cast<float>(mc.height), 0.05f, renderDistance);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (mc.options.anaglyph3d)
		glTranslatef((eye * 2 - 1) * 0.1f, 0.0f, 0.0f);

	bobHurt(a);
	if (mc.options.bobView)
		bobView(a);

	// TODO portal effect

	moveCameraToPlayer(a);
}

void GameRenderer::renderItemInHand(float a, int_t eye)
{
	glLoadIdentity();
	if (mc.options.anaglyph3d)
		glTranslatef((eye * 2 - 1) * 0.1f, 0.0f, 0.0f);

	glPushMatrix();

	bobHurt(a);
	if (mc.options.bobView)
		bobView(a);
	if (mc.options.thirdPersonView == 0)
		itemInHandRenderer.render(a);

	glPopMatrix();

	if (mc.options.thirdPersonView == 0)
	{
		itemInHandRenderer.renderScreenEffect(a);
		bobHurt(a);
	}
	if (mc.options.bobView)
		bobView(a);
}

void GameRenderer::render(float a)
{
	if (!lwjgl::Display::isActive())
	{
		if (System::currentTimeMillis() - lastActiveTime > 500)
			mc.pauseGame();
	}
	else
	{
		lastActiveTime = System::currentTimeMillis();
	}

	// Mouse movement
	if (mc.mouseGrabbed)
	{
		mc.mouseHandler.poll();
		float sens = mc.options.mouseSensitivity * 0.6f + 0.2f;
		float sens2 = sens * sens * sens * 8.0f;
		float dx = mc.mouseHandler.xd * sens2;
		float dy = mc.mouseHandler.yd * sens2;

		mc.player->turn(dx, dy * (mc.options.invertYMouse ? -1 : 1));
	}

	if (mc.noRender)
		return;

	ScreenSizeCalculator ssc(mc.width, mc.height, mc.options);
	int_t w = ssc.getWidth();
	int_t h = ssc.getHeight();

	int_t xm = lwjgl::Mouse::getX() * w / mc.width;
	int_t ym = h - lwjgl::Mouse::getY() * h / mc.height - 1;

	if (mc.level != nullptr)
	{
		renderLevel(a);
		if (!lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_F1))
			mc.gui.render(a, mc.screen != nullptr, xm, ym);
	}
	else
	{
		glViewport(0, 0, mc.width, mc.height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		setupGuiScreen();
	}

	if (mc.screen != nullptr)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		mc.screen->render(xm, ym, a);
	}
}

void GameRenderer::renderLevel(float a)
{
	pick(a);

	auto &player = mc.player;
	auto &levelRenderer = mc.levelRenderer;
	
	double xOff = player->xOld + (player->x - player->xOld) * a;
	double yOff = player->yOld + (player->y - player->yOld) * a;
	double zOff = player->zOld + (player->z - player->zOld) * a;

	auto chunkSource = mc.level->getChunkSource();
	if (chunkSource->isChunkCache())
	{
		ChunkCache &chunkCache = static_cast<ChunkCache &>(*chunkSource);
		int_t x = Mth::floor(static_cast<float>(static_cast<int_t>(player->x))) >> 4;
		int_t z = Mth::floor(static_cast<float>(static_cast<int_t>(player->z))) >> 4;
		chunkCache.centerOn(x, z);
	}

	for (int_t eye = 0; eye < 2; eye++)
	{
		if (mc.options.anaglyph3d)
		{
			if (eye == 0)
				glColorMask(false, true, true, false);
			else
				glColorMask(true, false, false, false);
		}

		glViewport(0, 0, mc.width, mc.height);
		setupClearColor(a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_CULL_FACE);

		setupCamera(a, eye);
		Frustum::getFrustum();

		if (mc.options.viewDistance < 2)
		{
			setupFog(-1);
			levelRenderer.renderSky(a);
		}

		glEnable(GL_FOG);
		setupFog(1);

		FrustumCuller culler;
		culler.prepare(xOff, yOff, zOff);

		mc.levelRenderer.cull(culler, a);
		mc.levelRenderer.updateDirtyChunks(*player, Minecraft::FLYBY_MODE);
		
		setupFog(0);

		glEnable(GL_FOG);
		glBindTexture(GL_TEXTURE_2D, mc.textures.loadTexture(u"/terrain.png"));

		// Alpha: EntityRenderer.func_4136_b() - two-pass rendering (EntityRenderer.java:439-500)
		// Pass 0: Opaque blocks
		Lighting::turnOff();
		levelRenderer.render(*player, 0, a);

		// Alpha: Render entities between passes (EntityRenderer.java:441-445)
		Lighting::turnOn();
		levelRenderer.renderEntities(*player->getPos(a), culler, a);
		
		// Beta: Render lit particles (GameRenderer.java:405)
		mc.particleEngine.renderLit(*player, a);
		Lighting::turnOff();

		setupFog(0);
		
		// Beta: Render particles (GameRenderer.java:408)
		mc.particleEngine.render(*player, a);

		// Alpha: Water material hit rendering (EntityRenderer.java:446-451)
		if (mc.hitResult.type != HitResult::Type::NONE && false) // && player->isUnderLiquid()
		{
			glDisable(GL_ALPHA_TEST);
			// TODO inventory selected
			levelRenderer.renderHit(*player, mc.hitResult, 0, nullptr, a);
			levelRenderer.renderHitOutline(*player, mc.hitResult, 0, nullptr, a);
			glEnable(GL_ALPHA_TEST);
		}

		// Alpha: Pass 1: Translucent blocks (water, ice) (EntityRenderer.java:478-500)
		// Beta: Lighting stays OFF for translucent rendering (GameRenderer.java:406 - no turnOn() before translucent)
		// Lighting::turnOff() was already called after entity rendering
		
		// Alpha: Setup blending state for translucent pass
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		setupFog(0);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		// Alpha: Depth test enabled for translucent rendering
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, mc.textures.loadTexture(u"/terrain.png"));

		// Alpha: Render pass 1 (translucent) - EntityRenderer.java:485/499
		// Alpha: In fancy graphics mode, uses color mask trick for depth prepass
		if (mc.options.fancyGraphics)
		{
			// Alpha: Depth prepass with color mask disabled (EntityRenderer.java:484-496)
			// Alpha: func_943_a(..., 1, ...) builds render lists and renders depth-only
			glColorMask(false, false, false, false);
			int_t translucentChunks = levelRenderer.render(*player, 1, a);
			glColorMask(true, true, true, true);
		
			// Alpha: Anaglyph 3D color mask (EntityRenderer.java:487-493)
			if (mc.options.anaglyph3d)
			{
				if (eye == 0)
					glColorMask(false, true, true, false);
				else
					glColorMask(true, false, false, false);
			}
		
			// Alpha: Render translucent chunks again with color (EntityRenderer.java:495-497)
			// Alpha: func_944_a(1, var1) renders the lists again (color pass)
			if (translucentChunks > 0)
			{
				levelRenderer.renderSameAsLast(1, a);
			}
		}
		else
		{
			// Alpha: Fast graphics - direct render (EntityRenderer.java:499)
			levelRenderer.render(*player, 1, a);
		}

		// Alpha: Restore GL state after translucent pass (EntityRenderer.java:502-504)
		glDepthMask(true);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);

		// TODO
		// water material
		if (zoom == 1.0 && mc.hitResult.type != HitResult::Type::NONE) // && !player->isUnderLiquid())
		{
			glDisable(GL_ALPHA_TEST);
			levelRenderer.renderHit(*player, mc.hitResult, 0, nullptr, a);
			levelRenderer.renderHitOutline(*player, mc.hitResult, 0, nullptr, a);
			glEnable(GL_ALPHA_TEST);
		}

		// Beta: GameRenderer.java:450-457 - exact order of operations
		glDisable(GL_FOG);  // Beta: GL11.glDisable(2912) (GameRenderer.java:450)
		if (hovered != nullptr)
		{
			// Beta: Empty block (GameRenderer.java:451-452)
		}

		// Beta: Reset MODELVIEW matrix to camera state before cloud rendering
		// Java applies glScalef directly to the current matrix, which should be the camera matrix
		// Since entity rendering uses glPushMatrix/glPopMatrix, the matrix should be restored,
		// but we ensure it's correct by resetting to camera state (matching setupCamera behavior)
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();  // Beta: Reset to identity (setupCamera does this at line 271)
		if (mc.options.anaglyph3d)
			glTranslatef((eye * 2 - 1) * 0.1f, 0.0f, 0.0f);  // Beta: Anaglyph offset (setupCamera line 273)
		bobHurt(a);  // Beta: Camera bob/hurt effects (setupCamera line 275)
		if (mc.options.bobView)
			bobView(a);  // Beta: View bob (setupCamera line 276-277)
		moveCameraToPlayer(a);  // Beta: Move camera to player (setupCamera line 281)

		setupFog(0);  // Beta: this.setupFog(0) (GameRenderer.java:454)
		glEnable(GL_FOG);  // Beta: GL11.glEnable(2912) (GameRenderer.java:455)
		levelRenderer.renderClouds(a);  // Beta: var3.renderClouds(var1) (GameRenderer.java:456)
		glDisable(GL_FOG);  // Beta: GL11.glDisable(2912) (GameRenderer.java:457)

		if (!Minecraft::FLYBY_MODE)
		{
			setupFog(1);
			if (zoom == 1.0)
			{
				// Set up separate projection for item in hand with fixed 70 FOV (no fovSetting)
				// This prevents arm stretching at high FOV (matches apclient behavior)
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				
				float eyeDist = 0.07f;
				if (mc.options.anaglyph3d)
					glTranslatef(-(eye * 2 - 1) * eyeDist, 0.0f, 0.0f);
				
				gluPerspective(getFovForItemInHand(a), static_cast<float>(mc.width) / static_cast<float>(mc.height), 0.05f, renderDistance);
				
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				if (mc.options.anaglyph3d)
					glTranslatef((eye * 2 - 1) * 0.1f, 0.0f, 0.0f);
				
				glClear(GL_DEPTH_BUFFER_BIT);
				renderItemInHand(a, eye);
				
				// Restore previous matrices
				glPopMatrix();
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
			}
		}

		if (!mc.options.anaglyph3d)
			return;
	}

	glColorMask(true, true, true, false);
}

void GameRenderer::setupGuiScreen()
{
	ScreenSizeCalculator ssc(mc.width, mc.height, mc.options);
	int_t w = ssc.getWidth();
	int_t h = ssc.getHeight();

	glClear(GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, w, h, 0.0, 1000.0, 3000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -2000.0f);
}

void GameRenderer::setupClearColor(float a)
{
	auto &level = mc.level;
	auto &player = mc.player;

	float dist = 1.0f / (4 - mc.options.viewDistance);
	dist = 1.0f - std::pow(dist, 0.25);

	Vec3 *skyColor = level->getSkyColor(*mc.player, a);
	float sr = skyColor->x;
	float sg = skyColor->y;
	float sb = skyColor->z;

	Vec3 *fogColor = level->getFogColor(a);
	fr = fogColor->x;
	fg = fogColor->y;
	fb = fogColor->z;

	fr += (sr - fr) * dist;
	fg += (sg - fg) * dist;
	fb += (sb - fb) * dist;

	// Beta: Set underwater/lava fog colors (GameRenderer.java:656-664)
	const Material &waterMat = Material::water;
	const Material &lavaMat = Material::lava;
	if (player->isUnderLiquid(waterMat))
	{
		fr = 0.02f;  // Beta: this.fr = 0.02F (GameRenderer.java:657)
		fg = 0.02f;  // Beta: this.fg = 0.02F (GameRenderer.java:658)
		fb = 0.2f;   // Beta: this.fb = 0.2F (GameRenderer.java:659)
	}
	else if (player->isUnderLiquid(lavaMat))
	{
		fr = 0.6f;   // Beta: this.fr = 0.6F (GameRenderer.java:661)
		fg = 0.1f;   // Beta: this.fg = 0.1F (GameRenderer.java:662)
		fb = 0.0f;   // Beta: this.fb = 0.0F (GameRenderer.java:663)
	}

	float fogBrNow = fogBrO + (fogBr - fogBrO) * a;
	fr *= fogBrNow;
	fg *= fogBrNow;
	fb *= fogBrNow;

	if (mc.options.anaglyph3d)
	{
		float frr = (fr * 30.0f + fg * 59.0f + fb * 11.0f) / 100.0f;
		float fgg = (fr * 30.0f + fg * 70.0f) / 100.0f;
		float fbb = (fr * 30.0f + fb * 70.0f) / 100.0f;
		fr = frr;
		fg = fgg;
		fb = fbb;
	}

	glClearColor(fr, fg, fb, 0.0f);
}

void GameRenderer::setupFog(int_t mode)
{
	// Beta: setupFog(int var1) - exact order of operations (GameRenderer.java:682-729)
	auto &player = mc.player;

	// Beta: Line 684 - Set fog color FIRST using normal fog colors (fr, fg, fb)
	float fog[4] = {fr, fg, fb, 1.0f};
	glFogfv(GL_FOG_COLOR, fog);  // Beta: GL11.glFog(2918, this.getBuffer(this.fr, this.fg, this.fb, 1.0F))
	
	// Beta: Line 685 - glNormal3f
	glNormal3f(0.0f, -1.0f, 0.0f);
	
	// Beta: Line 686 - glColor4f
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// Beta: Lines 687-697 - Underwater fog
	const Material &waterMat = Material::water;
	if (player->isUnderLiquid(waterMat))
	{
		// Beta: Line 688 - Set fog mode to GL_EXP
		glFogi(GL_FOG_MODE, GL_EXP);  // Beta: GL11.glFogi(2917, 2048) - GL_EXP = 2048
		
		// Beta: Line 689 - Set fog density
		glFogf(GL_FOG_DENSITY, 0.1f);  // Beta: GL11.glFogf(2914, 0.1F)
		
		// Beta: Lines 690-696 - Calculate fog colors (but don't apply them - Beta 1.2 doesn't use these)
		float var3 = 0.4f;  // Beta: var3 = 0.4F (GameRenderer.java:690)
		float var4 = 0.4f;  // Beta: var4 = 0.4F (GameRenderer.java:691)
		float var5 = 0.9f;  // Beta: var5 = 0.9F (GameRenderer.java:692)
		
		// Beta: Lines 693-696 - Anaglyph conversion (calculated but not used)
		if (mc.options.anaglyph3d)
		{
			float var6 = (var3 * 30.0f + var4 * 59.0f + var5 * 11.0f) / 100.0f;  // Beta: var6
			float var7 = (var3 * 30.0f + var4 * 70.0f) / 100.0f;  // Beta: var7
			float var8 = (var3 * 30.0f + var5 * 70.0f) / 100.0f;  // Beta: var8
			// Note: Beta 1.2 calculates these but doesn't use them
		}
		// Beta: Fog color is already set to fr, fg, fb (from setupClearColor) - don't override it
	}
	// Beta: Lines 698-708 - Lava fog
	else if (player->isUnderLiquid(Material::lava))
	{
		// Beta: Line 699 - Set fog mode to GL_EXP
		glFogi(GL_FOG_MODE, GL_EXP);  // Beta: GL11.glFogi(2917, 2048) (GameRenderer.java:699)
		
		// Beta: Line 700 - Set fog density
		glFogf(GL_FOG_DENSITY, 2.0f);  // Beta: GL11.glFogf(2914, 2.0F) (GameRenderer.java:700)
		
		// Beta: Lines 701-707 - Calculate fog colors (but don't apply them - Beta 1.2 doesn't use these)
		float var9 = 0.4f;   // Beta: var9 = 0.4F (GameRenderer.java:701)
		float var10 = 0.3f;  // Beta: var10 = 0.3F (GameRenderer.java:702)
		float var11 = 0.3f;  // Beta: var11 = 0.3F (GameRenderer.java:703)
		
		// Beta: Lines 704-707 - Anaglyph conversion (calculated but not used)
		if (mc.options.anaglyph3d)
	{
			float var12 = (var9 * 30.0f + var10 * 59.0f + var11 * 11.0f) / 100.0f;  // Beta: var12
			float var13 = (var9 * 30.0f + var10 * 70.0f) / 100.0f;  // Beta: var13
			float var14 = (var9 * 30.0f + var11 * 70.0f) / 100.0f;  // Beta: var14
			// Note: In Beta 1.2, these variables are calculated but never used
		}
		// Beta: Fog color is already set to fr, fg, fb (from setupClearColor) - don't override it
	}
	// Beta: Lines 709-725 - Normal fog
	else
	{
		// Beta: Line 710 - Set fog mode to GL_LINEAR
		glFogi(GL_FOG_MODE, GL_LINEAR);  // Beta: GL11.glFogi(2917, 9729) - GL_LINEAR = 9729
		
		// Beta: Line 711 - Set fog start
		glFogf(GL_FOG_START, renderDistance * 0.25f);  // Beta: GL11.glFogf(2915, this.renderDistance * 0.25F)
		
		// Beta: Line 712 - Set fog end
		glFogf(GL_FOG_END, renderDistance);  // Beta: GL11.glFogf(2916, this.renderDistance)
		
		// Beta: Lines 713-716 - Mode < 0 check
		if (mode < 0)
		{
			glFogf(GL_FOG_START, 0.0f);  // Beta: GL11.glFogf(2915, 0.0F)
			glFogf(GL_FOG_END, renderDistance * 0.8f);  // Beta: GL11.glFogf(2916, this.renderDistance * 0.8F)
		}

		// Beta: Lines 718-720 - GL_NV_fog_distance check
		if (lwjgl::GLContext::getCapabilities()["GL_NV_fog_distance"])
		{
			glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);  // Beta: GL11.glFogi(34138, 34139)
		}

		// Beta: Lines 722-724 - Dimension foggy check
		if (mc.level->dimension->foggy)
		{
			glFogf(GL_FOG_START, 0.0f);  // Beta: GL11.glFogf(2915, 0.0F)
		}
	}

	// Beta: Line 727 - glEnable(GL_COLOR_MATERIAL)
	glEnable(GL_COLOR_MATERIAL);  // Beta: GL11.glEnable(2903) - GL_COLOR_MATERIAL = 2903
	
	// Beta: Line 728 - glColorMaterial
	glColorMaterial(GL_FRONT, GL_AMBIENT);  // Beta: GL11.glColorMaterial(1028, 4608) - GL_FRONT = 1028, GL_AMBIENT = 4608
}

void GameRenderer::updateAllChunks()
{
	mc.levelRenderer.updateDirtyChunks(*mc.player, true);
}
