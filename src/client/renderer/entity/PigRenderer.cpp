#include "client/renderer/entity/PigRenderer.h"

#include "world/entity/animal/Pig.h"
#include "java/String.h"

// newb12: PigRenderer constructor
// Reference: newb12/net/minecraft/client/renderer/entity/PigRenderer.java:7-10
PigRenderer::PigRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, const std::shared_ptr<Model> &armor, float shadow)
	: MobRenderer(entityRenderDispatcher, model, shadow)
{
	setArmor(armor);
}

// newb12: PigRenderer.prepareArmor()
// Reference: newb12/net/minecraft/client/renderer/entity/PigRenderer.java:12-15
bool PigRenderer::prepareArmor(Mob &mob, int_t layer, float a)
{
	Pig &pig = static_cast<Pig &>(mob);
	if (layer == 0 && pig.hasSaddle())
	{
		bindTexture(u"/mob/saddle.png");
		return true;
	}
	return false;
}
