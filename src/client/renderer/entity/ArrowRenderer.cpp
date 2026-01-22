#include "client/renderer/entity/ArrowRenderer.h"

#include "world/entity/projectile/Arrow.h"
#include "client/renderer/Tesselator.h"
#include "util/Mth.h"
#include "pc/OpenGL.h"

#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 32826
#endif

ArrowRenderer::ArrowRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
}

void ArrowRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	Arrow &arrow = static_cast<Arrow &>(entity);
	
	bindTexture(u"/item/arrows.png");
	glPushMatrix();
	glTranslatef((float)x, (float)y, (float)z);
	glRotatef(arrow.yRotO + (arrow.yRot - arrow.yRotO) * a - 90.0f, 0.0f, 1.0f, 0.0f);
	glRotatef(arrow.xRotO + (arrow.xRot - arrow.xRotO) * a, 0.0f, 0.0f, 1.0f);
	Tesselator &t = Tesselator::instance;
	int_t type = 0;
	float u0 = 0.0f;
	float u1 = 0.5f;
	float v0 = (0 + type * 10) / 32.0f;
	float v1 = (5 + type * 10) / 32.0f;
	float u02 = 0.0f;
	float u12 = 0.15625f;
	float v02 = (5 + type * 10) / 32.0f;
	float v12 = (10 + type * 10) / 32.0f;
	float ss = 0.05625f;
	glEnable(GL_RESCALE_NORMAL);
	float shake = arrow.shakeTime - a;
	if (shake > 0.0f)
	{
		float pow = -Mth::sin(shake * 3.0f) * shake;
		glRotatef(pow, 0.0f, 0.0f, 1.0f);
	}

	glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
	glScalef(ss, ss, ss);
	glTranslatef(-4.0f, 0.0f, 0.0f);
	glNormal3f(ss, 0.0f, 0.0f);
	t.begin();
	t.vertexUV(-7.0, -2.0, -2.0, u02, v02);
	t.vertexUV(-7.0, -2.0, 2.0, u12, v02);
	t.vertexUV(-7.0, 2.0, 2.0, u12, v12);
	t.vertexUV(-7.0, 2.0, -2.0, u02, v12);
	t.end();
	glNormal3f(-ss, 0.0f, 0.0f);
	t.begin();
	t.vertexUV(-7.0, 2.0, -2.0, u02, v02);
	t.vertexUV(-7.0, 2.0, 2.0, u12, v02);
	t.vertexUV(-7.0, -2.0, 2.0, u12, v12);
	t.vertexUV(-7.0, -2.0, -2.0, u02, v12);
	t.end();

	for (int_t i = 0; i < 4; i++)
	{
		glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
		glNormal3f(0.0f, 0.0f, ss);
		t.begin();
		t.vertexUV(-8.0, -2.0, 0.0, u0, v0);
		t.vertexUV(8.0, -2.0, 0.0, u1, v0);
		t.vertexUV(8.0, 2.0, 0.0, u1, v1);
		t.vertexUV(-8.0, 2.0, 0.0, u0, v1);
		t.end();
	}

	glDisable(GL_RESCALE_NORMAL);
	glPopMatrix();
}
