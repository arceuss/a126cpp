#pragma once

#include "client/renderer/TileRenderer.h"
#include "world/item/ItemStack.h"

#include "java/Type.h"

class Minecraft;

class ItemInHandRenderer
{
private:
	Minecraft &mc;

	ItemStack selectedItem;  // Beta: selectedItem - tracks selected item (ItemInHandRenderer.java:18) - stored as value to preserve data during animation
	float height = 0.0f;
	float oHeight = 0.0f;
	TileRenderer tileRenderer;

	int_t lastSlot = -1;  // Beta: lastSlot - tracks last slot (ItemInHandRenderer.java:22)

public:
	// Beta: renderItem() - renders item in hand (ItemInHandRenderer.java:28-130)
	// Made public for third-person rendering
	void renderItem(ItemStack &item);

public:
	ItemInHandRenderer(Minecraft &mc);

	void render(float a);
	void renderScreenEffect(float a);
	void renderTex(float a, int_t tex);
	void renderWater(float a);
	void renderFire(float a);

	void tick();
	
	void itemPlaced();
	void itemUsed();
};
