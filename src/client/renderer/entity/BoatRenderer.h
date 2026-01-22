#pragma once

#include "client/renderer/entity/EntityRenderer.h"
#include "client/model/Model.h"
#include <memory>

class Boat;

// Alpha 1.2.6 BoatRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/BoatRenderer.java
class BoatRenderer : public EntityRenderer
{
protected:
	std::shared_ptr<Model> model;

public:
	BoatRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;
};
