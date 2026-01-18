#include "client/renderer/entity/EntityRenderer.h"

#include "client/renderer/entity/EntityRenderDispatcher.h"

#include "client/renderer/Tesselator.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FireTile.h"
#include "pc/OpenGL.h"

EntityRenderer::EntityRenderer(EntityRenderDispatcher &entityRenderDispatcher) : entityRenderDispatcher(entityRenderDispatcher)
{

}

void EntityRenderer::bindTexture(const jstring &resourceName)
{
	Textures &t = *entityRenderDispatcher.textures;
	t.bind(t.loadTexture(resourceName));
}

void EntityRenderer::bindTexture(const jstring &urlTexture, const jstring &backupTexture)
{
	// newb12: EntityRenderer.bindTexture(String, String) - loads HTTP texture with backup (EntityRenderer.java:18-24)
	Textures &t = *entityRenderDispatcher.textures;
	if (!urlTexture.empty())
	{
		int_t id = t.loadHttpTexture(urlTexture, &backupTexture);
		if (id >= 0)
		{
			t.bind(id);
			return;
		}
	}
	// Fallback to backup texture if URL texture fails or is empty
	bindTexture(backupTexture);
}

void EntityRenderer::renderFlame(Entity &e, double x, double y, double z, float a)
{
	// newb12: EntityRenderer.renderFlame() - renders fire overlay on entities (EntityRenderer.java:41-79)
	glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896) (EntityRenderer.java:42)
	int tex = Tile::fire.tex;  // Beta: int tex = Tile.fire.tex (EntityRenderer.java:43)
	int xt = (tex & 15) << 4;  // Beta: int xt = (tex & 15) << 4 (EntityRenderer.java:44)
	int yt = tex & 240;  // Beta: int yt = tex & 240 (EntityRenderer.java:45)
	float u0 = xt / 256.0F;  // Beta: float u0 = xt / 256.0F (EntityRenderer.java:46)
	float u1 = (xt + 15.99F) / 256.0F;  // Beta: float u1 = (xt + 15.99F) / 256.0F (EntityRenderer.java:47)
	float v0 = yt / 256.0F;  // Beta: float v0 = yt / 256.0F (EntityRenderer.java:48)
	float v1 = (yt + 15.99F) / 256.0F;  // Beta: float v1 = (yt + 15.99F) / 256.0F (EntityRenderer.java:49)
	glPushMatrix();  // Beta: GL11.glPushMatrix() (EntityRenderer.java:50)
	glTranslatef((float)x, (float)y, (float)z);  // Beta: GL11.glTranslatef((float)x, (float)y, (float)z) (EntityRenderer.java:51)
	float s = e.bbWidth * 1.4F;  // Beta: float s = e.bbWidth * 1.4F (EntityRenderer.java:52)
	glScalef(s, s, s);  // Beta: GL11.glScalef(s, s, s) (EntityRenderer.java:53)
	bindTexture(u"/terrain.png");  // Beta: this.bindTexture("/terrain.png") (EntityRenderer.java:54)
	Tesselator &t = Tesselator::instance;  // Beta: Tesselator t = Tesselator.instance (EntityRenderer.java:55)
	float r = 1.0F;  // Beta: float r = 1.0F (EntityRenderer.java:56)
	float xo = 0.5F;  // Beta: float xo = 0.5F (EntityRenderer.java:57)
	float yo = 0.0F;  // Beta: float yo = 0.0F (EntityRenderer.java:58)
	float h = e.bbHeight / e.bbWidth;  // Beta: float h = e.bbHeight / e.bbWidth (EntityRenderer.java:59)
	glRotatef(-entityRenderDispatcher.playerRotY, 0.0F, 1.0F, 0.0F);  // Beta: GL11.glRotatef(-this.entityRenderDispatcher.playerRotY, 0.0F, 1.0F, 0.0F) (EntityRenderer.java:60)
	glTranslatef(0.0F, 0.0F, -0.4F + (int)h * 0.02F);  // Beta: GL11.glTranslatef(0.0F, 0.0F, -0.4F + (int)h * 0.02F) (EntityRenderer.java:61)
	glColor4f(1.0F, 1.0F, 1.0F, 1.0F);  // Beta: GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F) (EntityRenderer.java:62)
	t.begin();  // Beta: t.begin() (EntityRenderer.java:63)

	while (h > 0.0F) {  // Beta: while (h > 0.0F) (EntityRenderer.java:65)
		t.vertexUV(r - xo, 0.0F - yo, 0.0, u1, v1);  // Beta: t.vertexUV(r - xo, 0.0F - yo, 0.0, u1, v1) (EntityRenderer.java:66)
		t.vertexUV(0.0F - xo, 0.0F - yo, 0.0, u0, v1);  // Beta: t.vertexUV(0.0F - xo, 0.0F - yo, 0.0, u0, v1) (EntityRenderer.java:67)
		t.vertexUV(0.0F - xo, 1.4F - yo, 0.0, u0, v0);  // Beta: t.vertexUV(0.0F - xo, 1.4F - yo, 0.0, u0, v0) (EntityRenderer.java:68)
		t.vertexUV(r - xo, 1.4F - yo, 0.0, u1, v0);  // Beta: t.vertexUV(r - xo, 1.4F - yo, 0.0, u1, v0) (EntityRenderer.java:69)
		h--;  // Beta: h-- (EntityRenderer.java:70)
		yo--;  // Beta: yo-- (EntityRenderer.java:71)
		r *= 0.9F;  // Beta: r *= 0.9F (EntityRenderer.java:72)
		glTranslatef(0.0F, 0.0F, -0.04F);  // Beta: GL11.glTranslatef(0.0F, 0.0F, -0.04F) (EntityRenderer.java:73)
	}

	t.end();  // Beta: t.end() (EntityRenderer.java:76)
	glPopMatrix();  // Beta: GL11.glPopMatrix() (EntityRenderer.java:77)
	glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896) (EntityRenderer.java:78)
}

void EntityRenderer::renderShadow(Entity &e, double x, double y, double z, float pow, float a)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Textures &textures = *entityRenderDispatcher.textures;
	textures.bind(textures.loadTexture(u"%clamp%/misc/shadow.png"));

	Level &level = getLevel();
	
	glDepthMask(false);
	float r = shadowRadius;

	double ex = e.xOld + (e.x - e.xOld) * a;
	double ey = e.yOld + (e.y - e.yOld) * a + e.getShadowHeightOffs();
	double ez = e.zOld + (e.z - e.zOld) * a;

	int_t x0 = Mth::floor(ex - r);
	int_t x1 = Mth::floor(ex + r);
	int_t y0 = Mth::floor(ey - r);
	int_t y1 = Mth::floor(ey);
	int_t z0 = Mth::floor(ez - r);
	int_t z1 = Mth::floor(ez + r);

	double xo = x - ex;
	double yo = y - ey;
	double zo = z - ez;

	Tesselator &tt = Tesselator::instance;
	tt.begin();
	for (int_t xt = x0; xt <= x1; xt++)
	{
		for (int_t yt = y0; yt <= y1; yt++)
		{
			for (int_t zt = z0; zt <= z1; zt++)
			{
				int_t t = level.getTile(xt, yt - 1, zt);
				if (t > 0 && level.getRawBrightness(xt, yt, zt) > 3)
					renderTileShadow(*Tile::tiles[t], x, y + e.getShadowHeightOffs(), z, xt, yt, zt, pow, r, xo, yo + e.getShadowHeightOffs(), zo);
			}
		}
	}
	tt.end();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);
	glDepthMask(true);
}

Level &EntityRenderer::getLevel()
{
	return *entityRenderDispatcher.level;
}

void EntityRenderer::renderTileShadow(Tile &tt, double x, double y, double z, int_t xt, int_t yt, int_t zt, float pow, float r, double xo, double yo, double zo)
{
	Tesselator &t = Tesselator::instance;
	if (!tt.isCubeShaped()) return;

	double a = (pow - (y - (yt + yo)) / 2.0) * 0.5 * getLevel().getBrightness(xt, yt, zt);
	if (a < 0.0) return;
	if (a > 1.0) a = 1.0;
	t.color(1.0f, 1.0f, 1.0f, (float)a);
	
	double x0 = xt + tt.xx0 + xo;
	double x1 = xt + tt.xx1 + xo;
	double y0 = yt + tt.yy0 + yo + (1.0 / 64.0);
	double z0 = zt + tt.zz0 + zo;
	double z1 = zt + tt.zz1 + zo;
	
	float u0 = (x - x0) / 2.0 / r + 0.5;
	float u1 = (x - x1) / 2.0 / r + 0.5;
	float v0 = (z - z0) / 2.0 / r + 0.5;
	float v1 = (z - z1) / 2.0 / r + 0.5;
	
	t.vertexUV(x0, y0, z0, u0, v0);
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z1, u1, v1);
	t.vertexUV(x1, y0, z0, u1, v0);
}

void EntityRenderer::render(AABB &bb, double xo, double yo, double zo)
{
	glDisable(GL_TEXTURE_2D);
	Tesselator &t = Tesselator::instance;
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	t.begin();
	t.offset(xo, yo, zo);
	t.normal(0.0f, 0.0f, -1.0f);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);
	
	t.normal(0.0f, 0.0f, 1.0f);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	
	t.normal(0.0f, -1.0f, 0.0f);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y0, bb.z1);
	
	t.normal(0.0f, 1.0f, 0.0f);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y1, bb.z0);
	
	t.normal(-1.0f, 0.0f, 0.0f);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);
	
	t.normal(1.0f, 0.0f, 0.0f);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.offset(0.0, 0.0, 0.0);
	t.end();
	glEnable(GL_TEXTURE_2D);
}

void EntityRenderer::renderFlat(AABB &bb)
{
	Tesselator &t = Tesselator::instance;
	t.begin();
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.end();
}


void EntityRenderer::postRender(Entity &entity, double x, double y, double z, float rot, float a)
{
	if (entityRenderDispatcher.options->fancyGraphics)
	{
		double dist = entityRenderDispatcher.distanceToSqr(entity.x, entity.y, entity.z);
		float pow = (1.0 - dist / 256.0) * shadowStrength;
		if (pow > 0.0f)
			renderShadow(entity, x, y, z, pow, a);
	}

	if (entity.isOnFire())
		renderFlame(entity, x, y, z, a);
}

Font &EntityRenderer::getFont()
{
	return entityRenderDispatcher.getFont();
}
