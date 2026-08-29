#include "client/renderer/entity/HumanoidMobRenderer.h"
#include "client/renderer/ItemInHandRenderer.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"

#include "world/entity/Mob.h"
#include "world/item/ItemStack.h"
#include "world/item/Item.h"
#include "client/renderer/TileRenderer.h"
#include "world/level/tile/Tile.h"
#include "pc/OpenGL.h"

HumanoidMobRenderer::HumanoidMobRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<HumanoidModel> &humanoidModel, float shadow) : MobRenderer(entityRenderDispatcher, humanoidModel, shadow)
{
	this->humanoidModel = std::static_pointer_cast<HumanoidModel>(model);
}

void HumanoidMobRenderer::additionalRendering(Mob &mob, float a)
{
	(void)a;
	ItemStack *item = mob.getCarriedItem();
	if (item == nullptr || item->isEmpty() || entityRenderDispatcher.itemInHandRenderer == nullptr)
		return;

	glPushMatrix();
	humanoidModel->arm0.translateTo(0.0625f);
	glTranslatef(-0.0625f, 0.4375f, 0.0625f);

	Tile *tile = item->itemID >= 0 && item->itemID < static_cast<int_t>(Tile::tiles.size())
		? Tile::tiles[item->itemID] : nullptr;
	if (tile != nullptr && TileRenderer::canRender(tile->getRenderShape()))
	{
		float s = 0.5f;
		glTranslatef(0.0f, 0.1875f, -0.3125f);
		s *= 0.75f;
		glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
		glScalef(s, -s, s);
	}
	else if (item->getItem() != nullptr && item->getItem()->isHandEquipped())
	{
		float s = 0.625f;
		glTranslatef(0.0f, 0.1875f, 0.0f);
		glScalef(s, -s, s);
		glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		float s = 0.375f;
		glTranslatef(0.25f, 0.1875f, -0.1875f);
		glScalef(s, s, s);
		glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
		glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
	}

	// Alpha: RenderBiped delegates the actual item mesh to RenderManager's
	// item renderer after applying the mob-hand transform
	// (RenderBiped.java:24-52).
	entityRenderDispatcher.itemInHandRenderer->renderItem(*item);
	glPopMatrix();
}
