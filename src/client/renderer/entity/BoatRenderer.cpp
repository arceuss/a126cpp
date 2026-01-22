#include "client/renderer/entity/BoatRenderer.h"

#include "world/entity/item/Boat.h"
#include "client/model/BoatModel.h"
#include "util/Mth.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"

BoatRenderer::BoatRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
	shadowRadius = 0.5f;
	model = Util::make_shared<BoatModel>();
}

void BoatRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	Boat &boat = static_cast<Boat &>(entity);
	
	glPushMatrix();
	glTranslatef((float)x, (float)y, (float)z);
	glRotatef(180.0f - rot, 0.0f, 1.0f, 0.0f);
	float hurt = boat.hurtTime - a;
	float dmg = boat.damage - a;
	if (dmg < 0.0f)
	{
		dmg = 0.0f;
	}

	if (hurt > 0.0f)
	{
		glRotatef(Mth::sin(hurt) * hurt * dmg / 10.0f * boat.hurtDir, 1.0f, 0.0f, 0.0f);
	}

	bindTexture(u"/terrain.png");
	float ss = 0.75f;
	glScalef(ss, ss, ss);
	glScalef(1.0f / ss, 1.0f / ss, 1.0f / ss);
	bindTexture(u"/item/boat.png");
	glScalef(-1.0f, -1.0f, 1.0f);
	model->render(0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 0.0625f);
	glPopMatrix();
}
