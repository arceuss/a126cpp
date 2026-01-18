#include "client/renderer/entity/SheepRenderer.h"

#include "world/entity/animal/Sheep.h"

// newb12: SheepRenderer constructor
// Reference: newb12/net/minecraft/client/renderer/entity/SheepRenderer.java:8-11
SheepRenderer::SheepRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, const std::shared_ptr<Model> &armor, float shadow)
	: MobRenderer(entityRenderDispatcher, model, shadow)
{
	setArmor(armor);
}

// newb12: SheepRenderer.prepareArmor()
// Reference: newb12/net/minecraft/client/renderer/entity/SheepRenderer.java:13-23
// Note: Alpha 1.2.6 doesn't have colored wool, so we don't need the color logic from newb12
bool SheepRenderer::prepareArmor(Mob &mob, int_t layer, float a)
{
	Sheep &sheep = static_cast<Sheep &>(mob);
	if (layer == 0 && !sheep.isSheared())
	{
		bindTexture(u"/mob/sheep_fur.png");
		// Alpha: No color tinting - sheep are always white
		return true;
	}
	return false;
}
