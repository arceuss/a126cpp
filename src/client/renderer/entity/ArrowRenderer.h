#pragma once

#include "client/renderer/entity/EntityRenderer.h"

class Arrow;

// Alpha 1.2.6 ArrowRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/ArrowRenderer.java
class ArrowRenderer : public EntityRenderer
{
public:
	ArrowRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;
};
