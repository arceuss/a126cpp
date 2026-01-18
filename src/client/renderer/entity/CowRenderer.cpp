#include "client/renderer/entity/CowRenderer.h"

// newb12: CowRenderer constructor
// Reference: newb12/net/minecraft/client/renderer/entity/CowRenderer.java:7-9
CowRenderer::CowRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, float shadow)
	: MobRenderer(entityRenderDispatcher, model, shadow)
{
}
