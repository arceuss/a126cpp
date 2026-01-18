#pragma once

#include "client/renderer/entity/MobRenderer.h"

class Creeper;

// Alpha 1.2.6 CreeperRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/CreeperRenderer.java
class CreeperRenderer : public MobRenderer
{
public:
	CreeperRenderer(EntityRenderDispatcher &entityRenderDispatcher);

protected:
	virtual void scale(Mob &mob, float a) override;
	virtual int_t getOverlayColor(Mob &mob, float br, float b) override;
};
