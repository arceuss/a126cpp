#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Pig;

// newb12: PigRenderer - renderer for pig entities
// Reference: newb12/net/minecraft/client/renderer/entity/PigRenderer.java
class PigRenderer : public MobRenderer
{
public:
	PigRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, const std::shared_ptr<Model> &armor, float shadow);

protected:
	virtual bool prepareArmor(Mob &mob, int_t layer, float a) override;
};
