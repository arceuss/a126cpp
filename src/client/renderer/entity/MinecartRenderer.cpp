#include "client/renderer/entity/MinecartRenderer.h"

#include "world/entity/item/Minecart.h"
#include "client/model/MinecartModel.h"
#include "client/renderer/TileRenderer.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/ChestTile.h"
#include "world/level/tile/FurnaceTile.h"
#include "world/phys/Vec3.h"
#include "util/Mth.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

MinecartRenderer::MinecartRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
	shadowRadius = 0.5f;
	model = Util::make_shared<MinecartModel>();
}

void MinecartRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	Minecart &cart = static_cast<Minecart &>(entity);
	
	glPushMatrix();
	double xx = cart.xOld + (cart.x - cart.xOld) * a;
	double yy = cart.yOld + (cart.y - cart.yOld) * a;
	double zz = cart.zOld + (cart.z - cart.zOld) * a;
	double r = 0.3f;
	Vec3 *p = cart.getPos(xx, yy, zz);
	float xRot = cart.xRotO + (cart.xRot - cart.xRotO) * a;
	if (p != nullptr)
	{
		Vec3 *p0 = cart.getPosOffs(xx, yy, zz, r);
		Vec3 *p1 = cart.getPosOffs(xx, yy, zz, -r);
		if (p0 == nullptr)
		{
			p0 = p;
		}

		if (p1 == nullptr)
		{
			p1 = p;
		}

		x += p->x - xx;
		y += (p0->y + p1->y) / 2.0 - yy;
		z += p->z - zz;
		Vec3 dirVec(p1->x - p0->x, p1->y - p0->y, p1->z - p0->z);
		if (dirVec.length() != 0.0)
		{
			Vec3 *dir = dirVec.normalize();
			rot = (float)(std::atan2(dir->z, dir->x) * 180.0 / M_PI);
			xRot = (float)(std::atan(dir->y) * 73.0);
		}
	}

	glTranslatef((float)x, (float)y, (float)z);
	glRotatef(180.0f - rot, 0.0f, 1.0f, 0.0f);
	glRotatef(-xRot, 0.0f, 0.0f, 1.0f);
	float hurt = cart.hurtTime - a;
	float dmg = cart.damage - a;
	if (dmg < 0.0f)
	{
		dmg = 0.0f;
	}

	if (hurt > 0.0f)
	{
		glRotatef(Mth::sin(hurt) * hurt * dmg / 10.0f * cart.hurtDir, 1.0f, 0.0f, 0.0f);
	}

	if (cart.type != 0)
	{
		bindTexture(u"/terrain.png");
		float ss = 0.75f;
		glScalef(ss, ss, ss);
		glTranslatef(0.0f, 0.3125f, 0.0f);
		glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		TileRenderer tileRenderer;
		if (cart.type == 1)
		{
			tileRenderer.renderTile(static_cast<Tile&>(Tile::chest), 0);
		}
		else if (cart.type == 2)
		{
			tileRenderer.renderTile(static_cast<Tile&>(Tile::furnace), 0);
		}

		glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, -0.3125f, 0.0f);
		glScalef(1.0f / ss, 1.0f / ss, 1.0f / ss);
	}

	bindTexture(u"/item/cart.png");
	glScalef(-1.0f, -1.0f, 1.0f);
	model->render(0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 0.0625f);
	glPopMatrix();
}
