#pragma once

#include "client/renderer/entity/EntityRenderer.h"
#include "java/Random.h"

class Painting;

// Alpha 1.2.6 PaintingRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/PaintingRenderer.java
class PaintingRenderer : public EntityRenderer
{
private:
	Random random;

public:
	PaintingRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;

private:
	void renderPainting(Painting &painting, int w, int h, int uo, int vo);
	void setBrightness(Painting &painting, float ss, float ya);
};
