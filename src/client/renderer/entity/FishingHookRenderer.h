#pragma once

#include "client/renderer/entity/EntityRenderer.h"

class FishingHook;

// Beta 1.2: FishingHookRenderer.java
class FishingHookRenderer : public EntityRenderer
{
public:
	FishingHookRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;
};
