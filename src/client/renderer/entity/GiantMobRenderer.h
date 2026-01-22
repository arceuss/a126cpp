#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Giant;

// Alpha 1.2.6 GiantMobRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/GiantMobRenderer.java
class GiantMobRenderer : public MobRenderer
{
private:
	float scaleFactor;

public:
	GiantMobRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, float shadow, float scale);

protected:
	virtual void scale(Mob &mob, float a) override;
};
