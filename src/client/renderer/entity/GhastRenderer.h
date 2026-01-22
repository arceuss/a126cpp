#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Ghast;

// Alpha 1.2.6 GhastRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/GhastRenderer.java
class GhastRenderer : public MobRenderer
{
public:
	GhastRenderer(EntityRenderDispatcher &entityRenderDispatcher);

protected:
	virtual void scale(Mob &mob, float a) override;
};
