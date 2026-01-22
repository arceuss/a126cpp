#pragma once

#include "client/renderer/entity/EntityRenderer.h"
#include "client/renderer/TileRenderer.h"
#include "java/Random.h"

class Font;
class Textures;
class ItemStack;

class ItemRenderer : public EntityRenderer
{
private:
	TileRenderer tileRenderer;
	Random random;

public:
	ItemRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;
	
	// Beta: ItemRenderer.renderGuiItem() - renders item icon in GUI
	void renderGuiItem(Font &font, Textures &textures, ItemStack &item, int_t x, int_t y);
	
	// Beta: ItemRenderer.renderGuiItemDecorations() - renders count text and durability bar
	void renderGuiItemDecorations(Font &font, Textures &textures, ItemStack &item, int_t x, int_t y);
	
private:
	// Helper method for rendering filled rectangle
	void fillRect(int_t x, int_t y, int_t w, int_t h, int_t color);
	
	// Beta: ItemRenderer.blit() - renders texture region
	void blit(int_t x, int_t y, int_t sx, int_t sy, int_t w, int_t h);
};
