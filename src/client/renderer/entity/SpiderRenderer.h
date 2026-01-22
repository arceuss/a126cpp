#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Spider;

// Alpha 1.2.6 SpiderRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/SpiderRenderer.java
class SpiderRenderer : public MobRenderer
{
public:
	SpiderRenderer(EntityRenderDispatcher &entityRenderDispatcher);

protected:
	virtual float getFlipDegrees(Mob &mob) override;
	virtual bool prepareArmor(Mob &mob, int_t layer, float a) override;
};
