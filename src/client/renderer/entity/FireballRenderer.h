#pragma once

#include "client/renderer/entity/EntityRenderer.h"

// Alpha 1.2.6 RenderFireball - draws a ghast fireball as a scaled-up snowball
// icon billboarded at the camera.
// Reference: apclient/minecraft/src/net/minecraft/src/RenderFireball.java
class FireballRenderer : public EntityRenderer
{
public:
	FireballRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	virtual void render(Entity &entity, double x, double y, double z, float rot, float a) override;
};
