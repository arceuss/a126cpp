#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Slime;

// Alpha 1.2.6 SlimeRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/SlimeRenderer.java
class SlimeRenderer : public MobRenderer
{
private:
	std::shared_ptr<Model> armor;

public:
	SlimeRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, const std::shared_ptr<Model> &armor, float shadow);

protected:
	virtual bool prepareArmor(Mob &mob, int_t layer, float a) override;
	virtual void scale(Mob &mob, float a) override;
};
