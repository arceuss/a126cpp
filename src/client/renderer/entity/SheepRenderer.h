#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Sheep;

// newb12: SheepRenderer - renderer for sheep entities
// Reference: newb12/net/minecraft/client/renderer/entity/SheepRenderer.java
class SheepRenderer : public MobRenderer
{
public:
	SheepRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, const std::shared_ptr<Model> &armor, float shadow);

protected:
	virtual bool prepareArmor(Mob &mob, int_t layer, float a) override;
};
