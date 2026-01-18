#pragma once

#include <memory>

#include "client/renderer/ItemInHandRenderer.h"

#include "world/entity/Entity.h"

#include "java/Type.h"
#include "java/System.h"
#include "java/Random.h"
#include "world/phys/ChunkCoordinates.h"
#include "network/Packet63Digging.h"
#include <unordered_map>

class Minecraft;

class GameRenderer
{
private:
	Minecraft &mc;

	float renderDistance = 0.0f;

public:
	ItemInHandRenderer itemInHandRenderer;

	int_t ticks = 0;
	std::shared_ptr<Entity> hovered;
	
	// Alpha 1.2.6: EntityRenderer.mpDigging - stores block breaking progress for rendering
	// Java: public HashMap<ChunkCoordinates, Packet63Digging> mpDigging = new HashMap();
	std::unordered_map<ChunkCoordinates, Packet63Digging*> mpDigging;

	double zoom = 1.0;
	double zoom_x = 0.0;
	double zoom_y = 0.0;

	long_t lastActiveTime = System::currentTimeMillis();

	Random random = Random();

public:
	volatile int xMod = 0, yMod = 0;

	float fr = 0.0f, fg = 0.0f, fb = 0.0f;
private:
	float fogBrO = 0.0f, fogBr = 0.0f;

public:
	GameRenderer(Minecraft &mc);

	void tick();
	void pick(float a);

private:
	float getFov(float a);
	float getFovForItemInHand(float a);  // Returns base 70 FOV (no fovSetting multiplier) to prevent arm stretching

	void bobHurt(float a);
	void bobView(float a);
	void moveCameraToPlayer(float a);

	void setupCamera(float a, int_t eye);
	void renderItemInHand(float a, int_t eye);

public:
	void render(float a);
	void renderLevel(float a);

	void setupGuiScreen();
private:
	void setupClearColor(float a);
	void setupFog(int_t mode);

public:
	void updateAllChunks();
};
