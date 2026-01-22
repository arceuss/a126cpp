#include "client/renderer/entity/HumanoidMobRenderer.h"

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
	ItemStack *item = mob.getCarriedItem();
	if (item != nullptr)
	{
		glPushMatrix();
		humanoidModel->arm0.translateTo(0.0625f);
		glTranslatef(-0.0625f, 0.4375f, 0.0625f);
		if (item->itemID < 256 && TileRenderer::canRender(Tile::tiles[item->itemID]->getRenderShape()))
		{
			float s = 0.5f;
			glTranslatef(0.0f, 0.1875f, -0.3125f);
			s *= 0.75f;
			glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
			glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
			glScalef(s, -s, s);
		}
		else if (Item::itemsList[item->itemID] != nullptr && Item::itemsList[item->itemID]->isHandEquipped())
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

		// TODO: Render item in hand
		// this->entityRenderDispatcher.itemInHandRenderer.renderItem(item);
		glPopMatrix();
	}
}
