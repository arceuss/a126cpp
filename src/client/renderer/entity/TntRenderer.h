#pragma once

#include "client/renderer/entity/EntityRenderer.h"
#include "client/renderer/TileRenderer.h"

class PrimedTnt;

// Alpha 1.2.6 TntRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/TntRenderer.java
class TntRenderer : public EntityRenderer
{
private:
	TileRenderer tileRenderer;

public:
	TntRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;
};
