#pragma once

#include "client/renderer/entity/EntityRenderer.h"
#include "client/model/Model.h"
#include "client/renderer/TileRenderer.h"
#include <memory>

class Minecart;

// Alpha 1.2.6 MinecartRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/MinecartRenderer.java
class MinecartRenderer : public EntityRenderer
{
protected:
	std::shared_ptr<Model> model;

public:
	MinecartRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;
};
