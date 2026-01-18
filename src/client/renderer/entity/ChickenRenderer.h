#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Chicken;

// newb12: ChickenRenderer - renderer for chicken entities
// Reference: newb12/net/minecraft/client/renderer/entity/ChickenRenderer.java
class ChickenRenderer : public MobRenderer
{
public:
	ChickenRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, float shadow);

protected:
	float getBob(Mob &mob, float a) override;
};
