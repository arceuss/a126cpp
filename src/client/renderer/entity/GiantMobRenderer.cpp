#include "client/renderer/entity/GiantMobRenderer.h"

#include "world/entity/monster/Giant.h"
#include "pc/OpenGL.h"

GiantMobRenderer::GiantMobRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, float shadow, float scale) : MobRenderer(entityRenderDispatcher, model, shadow * scale)
{
	this->scaleFactor = scale;
}

void GiantMobRenderer::scale(Mob &mob, float a)
{
	glScalef(this->scaleFactor, this->scaleFactor, this->scaleFactor);
}
