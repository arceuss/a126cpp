#pragma once

#include "client/renderer/entity/EntityRenderer.h"
#include "client/renderer/TileRenderer.h"

class FallingTile;

// Alpha 1.2.6 FallingTileRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/FallingTileRenderer.java
class FallingTileRenderer : public EntityRenderer
{
private:
	TileRenderer tileRenderer;

public:
	FallingTileRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;
};
