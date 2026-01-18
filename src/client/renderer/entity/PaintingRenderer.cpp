#include "client/renderer/entity/PaintingRenderer.h"

#include "world/entity/Painting.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "client/renderer/Tesselator.h"
#include "util/Mth.h"
#include "pc/OpenGL.h"

#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 32826
#endif

PaintingRenderer::PaintingRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
}

void PaintingRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	Painting &painting = static_cast<Painting &>(entity);
	
	random.setSeed(187L);
	glPushMatrix();
	glTranslatef((float)x, (float)y, (float)z);
	glRotatef(rot, 0.0f, 1.0f, 0.0f);
	glEnable(GL_RESCALE_NORMAL);
	bindTexture(u"/art/kz.png");
	Painting::MotiveData &motiveData = Painting::getMotiveData(painting.motive);
	float s = 0.0625f;
	glScalef(s, s, s);
	renderPainting(painting, motiveData.w, motiveData.h, motiveData.uo, motiveData.vo);
	glDisable(GL_RESCALE_NORMAL);
	glPopMatrix();
}

void PaintingRenderer::renderPainting(Painting &painting, int w, int h, int uo, int vo)
{
	float xx0 = -w / 2.0f;
	float yy0 = -h / 2.0f;
	float z0 = -0.5f;
	float z1 = 0.5f;

	for (int xs = 0; xs < w / 16; xs++)
	{
		for (int ys = 0; ys < h / 16; ys++)
		{
			float x0 = xx0 + (xs + 1) * 16;
			float x1 = xx0 + xs * 16;
			float y0 = yy0 + (ys + 1) * 16;
			float y1 = yy0 + ys * 16;
			setBrightness(painting, (x0 + x1) / 2.0f, (y0 + y1) / 2.0f);
			float fu0 = (uo + w - xs * 16) / 256.0f;
			float fu1 = (uo + w - (xs + 1) * 16) / 256.0f;
			float fv0 = (vo + h - ys * 16) / 256.0f;
			float fv1 = (vo + h - (ys + 1) * 16) / 256.0f;
			float bu0 = 0.75f;
			float bu1 = 0.8125f;
			float bv0 = 0.0f;
			float bv1 = 0.0625f;
			float uu0 = 0.75f;
			float uu1 = 0.8125f;
			float uv0 = 0.001953125f;
			float uv1 = 0.001953125f;
			float su0 = 0.7519531f;
			float su1 = 0.7519531f;
			float sv0 = 0.0f;
			float sv1 = 0.0625f;
			Tesselator &t = Tesselator::instance;
			t.begin();
			t.normal(0.0f, 0.0f, -1.0f);
			t.vertexUV(x0, y1, z0, fu1, fv0);
			t.vertexUV(x1, y1, z0, fu0, fv0);
			t.vertexUV(x1, y0, z0, fu0, fv1);
			t.vertexUV(x0, y0, z0, fu1, fv1);
			t.normal(0.0f, 0.0f, 1.0f);
			t.vertexUV(x0, y0, z1, bu0, bv0);
			t.vertexUV(x1, y0, z1, bu1, bv0);
			t.vertexUV(x1, y1, z1, bu1, bv1);
			t.vertexUV(x0, y1, z1, bu0, bv1);
			t.normal(0.0f, -1.0f, 0.0f);
			t.vertexUV(x0, y0, z0, uu0, uv0);
			t.vertexUV(x1, y0, z0, uu1, uv0);
			t.vertexUV(x1, y0, z1, uu1, uv1);
			t.vertexUV(x0, y0, z1, uu0, uv1);
			t.normal(0.0f, 1.0f, 0.0f);
			t.vertexUV(x0, y1, z1, uu0, uv0);
			t.vertexUV(x1, y1, z1, uu1, uv0);
			t.vertexUV(x1, y1, z0, uu1, uv1);
			t.vertexUV(x0, y1, z0, uu0, uv1);
			t.normal(-1.0f, 0.0f, 0.0f);
			t.vertexUV(x0, y0, z1, su1, sv0);
			t.vertexUV(x0, y1, z1, su1, sv1);
			t.vertexUV(x0, y1, z0, su0, sv1);
			t.vertexUV(x0, y0, z0, su0, sv0);
			t.normal(1.0f, 0.0f, 0.0f);
			t.vertexUV(x1, y0, z0, su1, sv0);
			t.vertexUV(x1, y1, z0, su1, sv1);
			t.vertexUV(x1, y1, z1, su0, sv1);
			t.vertexUV(x1, y0, z1, su0, sv0);
			t.end();
		}
	}
}

void PaintingRenderer::setBrightness(Painting &painting, float ss, float ya)
{
	int_t x = Mth::floor(painting.x);
	int_t y = Mth::floor(painting.y + ya / 16.0f);
	int_t z = Mth::floor(painting.z);
	if (painting.dir == 0)
	{
		x = Mth::floor(painting.x + ss / 16.0f);
	}

	if (painting.dir == 1)
	{
		z = Mth::floor(painting.z - ss / 16.0f);
	}

	if (painting.dir == 2)
	{
		x = Mth::floor(painting.x - ss / 16.0f);
	}

	if (painting.dir == 3)
	{
		z = Mth::floor(painting.z + ss / 16.0f);
	}

	float br = entityRenderDispatcher.level->getBrightness(x, y, z);
	glColor3f(br, br, br);
}
