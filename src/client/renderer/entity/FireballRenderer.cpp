#include "client/renderer/entity/FireballRenderer.h"

#include "client/renderer/Tesselator.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/entity/Entity.h"
#include "world/item/Item.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"

#include "pc/OpenGL.h"

FireballRenderer::FireballRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{

}

// Alpha: RenderFireball.a() (RenderFireball.java:15-42)
void FireballRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	glPushMatrix();
	glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	glEnable(GL_RESCALE_NORMAL);

	float scale = 2.0f;
	glScalef(scale / 1.0f, scale / 1.0f, scale / 1.0f);

	int_t icon = Items::snowball->getIconIndex(ItemStack(Items::snowball->getShiftedIndex(), 1, 0));
	bindTexture(u"/gui/items.png");

	float u0 = static_cast<float>(icon % 16 * 16 + 0) / 256.0f;
	float u1 = static_cast<float>(icon % 16 * 16 + 16) / 256.0f;
	float v0 = static_cast<float>(icon / 16 * 16 + 0) / 256.0f;
	float v1 = static_cast<float>(icon / 16 * 16 + 16) / 256.0f;

	float size = 1.0f;
	float xo = 0.5f;
	float yo = 0.25f;

	glRotatef(180.0f - entityRenderDispatcher.playerRotY, 0.0f, 1.0f, 0.0f);
	glRotatef(-entityRenderDispatcher.playerRotX, 1.0f, 0.0f, 0.0f);

	Tesselator &t = Tesselator::instance;
	t.begin();
	t.normal(0.0f, 1.0f, 0.0f);
	t.vertexUV(0.0f - xo, 0.0f - yo, 0.0, u0, v1);
	t.vertexUV(size - xo, 0.0f - yo, 0.0, u1, v1);
	t.vertexUV(size - xo, 1.0f - yo, 0.0, u1, v0);
	t.vertexUV(0.0f - xo, 1.0f - yo, 0.0, u0, v0);
	t.end();

	glDisable(GL_RESCALE_NORMAL);
	glPopMatrix();
}
