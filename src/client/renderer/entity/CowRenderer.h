#pragma once

#include "client/renderer/entity/MobRenderer.h"

// newb12: CowRenderer - renderer for cow entities
// Reference: newb12/net/minecraft/client/renderer/entity/CowRenderer.java
class CowRenderer : public MobRenderer
{
public:
	CowRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, float shadow);
};
