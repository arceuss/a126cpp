#include "client/renderer/entity/FishingHookRenderer.h"

#include "world/entity/projectile/FishingHook.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "client/renderer/Tesselator.h"
#include "util/Mth.h"
#include "pc/OpenGL.h"
#include <cmath>

#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 32826
#endif

#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 3553
#endif

#ifndef GL_LIGHTING
#define GL_LIGHTING 2896
#endif

#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003  // OpenGL constant for line strip
#endif

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

FishingHookRenderer::FishingHookRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
}

// Beta: render() - renders fishing hook and line (FishingHookRenderer.java:9-76)
void FishingHookRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	FishingHook &hook = static_cast<FishingHook &>(entity);
	
	// Beta: Render hook sprite (FishingHookRenderer.java:10-35)
	glPushMatrix();  // Beta: GL11.glPushMatrix() (FishingHookRenderer.java:10)
	glTranslatef((float)x, (float)y, (float)z);  // Beta: GL11.glTranslatef(...) (FishingHookRenderer.java:11)
	glEnable(GL_RESCALE_NORMAL);  // Beta: GL11.glEnable(32826) (FishingHookRenderer.java:12)
	glScalef(0.5f, 0.5f, 0.5f);  // Beta: GL11.glScalef(0.5F, 0.5F, 0.5F) (FishingHookRenderer.java:13)
	int_t xi = 1;  // Beta: int xi = 1 (FishingHookRenderer.java:14)
	int_t yi = 2;  // Beta: int yi = 2 (FishingHookRenderer.java:15)
	bindTexture(u"/particles.png");  // Beta: this.bindTexture("/particles.png") (FishingHookRenderer.java:16)
	Tesselator &t = Tesselator::instance;  // Beta: Tesselator t = Tesselator.instance (FishingHookRenderer.java:17)
	float u0 = (xi * 8 + 0) / 128.0f;  // Beta: float u0 = (xi * 8 + 0) / 128.0F (FishingHookRenderer.java:18)
	float u1 = (xi * 8 + 8) / 128.0f;  // Beta: float u1 = (xi * 8 + 8) / 128.0F (FishingHookRenderer.java:19)
	float v0 = (yi * 8 + 0) / 128.0f;  // Beta: float v0 = (yi * 8 + 0) / 128.0F (FishingHookRenderer.java:20)
	float v1 = (yi * 8 + 8) / 128.0f;  // Beta: float v1 = (yi * 8 + 8) / 128.0F (FishingHookRenderer.java:21)
	float r = 1.0f;  // Beta: float r = 1.0F (FishingHookRenderer.java:22)
	float xo = 0.5f;  // Beta: float xo = 0.5F (FishingHookRenderer.java:23)
	float yo = 0.5f;  // Beta: float yo = 0.5F (FishingHookRenderer.java:24)
	glRotatef(180.0f - entityRenderDispatcher.playerRotY, 0.0f, 1.0f, 0.0f);  // Beta: GL11.glRotatef(...) (FishingHookRenderer.java:25)
	glRotatef(-entityRenderDispatcher.playerRotX, 1.0f, 0.0f, 0.0f);  // Beta: GL11.glRotatef(...) (FishingHookRenderer.java:26)
	t.begin();  // Beta: t.begin() (FishingHookRenderer.java:27)
	t.normal(0.0f, 1.0f, 0.0f);  // Beta: t.normal(...) (FishingHookRenderer.java:28)
	t.vertexUV(0.0f - xo, 0.0f - yo, 0.0, u0, v1);  // Beta: t.vertexUV(...) (FishingHookRenderer.java:29)
	t.vertexUV(r - xo, 0.0f - yo, 0.0, u1, v1);  // Beta: t.vertexUV(...) (FishingHookRenderer.java:30)
	t.vertexUV(r - xo, 1.0f - yo, 0.0, u1, v0);  // Beta: t.vertexUV(...) (FishingHookRenderer.java:31)
	t.vertexUV(0.0f - xo, 1.0f - yo, 0.0, u0, v0);  // Beta: t.vertexUV(...) (FishingHookRenderer.java:32)
	t.end();  // Beta: t.end() (FishingHookRenderer.java:33)
	glDisable(GL_RESCALE_NORMAL);  // Beta: GL11.glDisable(32826) (FishingHookRenderer.java:34)
	glPopMatrix();  // Beta: GL11.glPopMatrix() (FishingHookRenderer.java:35)
	
	// Beta: Render fishing line (FishingHookRenderer.java:36-74)
	if (hook.owner != nullptr)
	{
		// Beta: Calculate owner hand position (FishingHookRenderer.java:37-45)
		float rr = (hook.owner->yRotO + (hook.owner->yRot - hook.owner->yRotO) * a) * (float)M_PI / 180.0f;  // Beta: float rr = ... (FishingHookRenderer.java:37)
		float rr2 = (hook.owner->xRotO + (hook.owner->xRot - hook.owner->xRotO) * a) * (float)M_PI / 180.0f;  // Beta: float rr2 = ... (FishingHookRenderer.java:38)
		double ss = Mth::sin(rr);  // Beta: double ss = Mth.sin(rr) (FishingHookRenderer.java:39)
		double cc = Mth::cos(rr);  // Beta: double cc = Mth.cos(rr) (FishingHookRenderer.java:40)
		double ss2 = Mth::sin(rr2);  // Beta: double ss2 = Mth.sin(rr2) (FishingHookRenderer.java:41)
		double cc2 = Mth::cos(rr2);  // Beta: double cc2 = Mth.cos(rr2) (FishingHookRenderer.java:42)
		double xp = hook.owner->xo + (hook.owner->x - hook.owner->xo) * a - cc * 0.7 - ss * 0.5 * cc2;  // Beta: double xp = ... (FishingHookRenderer.java:43)
		double yp = hook.owner->yo + (hook.owner->y - hook.owner->yo) * a - ss2 * 0.5;  // Beta: double yp = ... (FishingHookRenderer.java:44)
		double zp = hook.owner->zo + (hook.owner->z - hook.owner->zo) * a - ss * 0.7 + cc * 0.5 * cc2;  // Beta: double zp = ... (FishingHookRenderer.java:45)
		
		// Beta: Third person view adjustment (FishingHookRenderer.java:46-52)
		if (entityRenderDispatcher.options->thirdPersonView)
		{
			// Beta: Use yRot for body rotation (yBodyRot may not be in Beta 1.2 player)
			rr = (hook.owner->yRotO + (hook.owner->yRot - hook.owner->yRotO) * a) * (float)M_PI / 180.0f;  // Beta: rr = ... (FishingHookRenderer.java:47)
			ss = Mth::sin(rr);  // Beta: ss = Mth.sin(rr) (FishingHookRenderer.java:48)
			cc = Mth::cos(rr);  // Beta: cc = Mth.cos(rr) (FishingHookRenderer.java:49)
			xp = hook.owner->xo + (hook.owner->x - hook.owner->xo) * a - cc * 0.35 - ss * 0.85;  // Beta: xp = ... (FishingHookRenderer.java:50)
			yp = hook.owner->yo + (hook.owner->y - hook.owner->yo) * a - 0.45;  // Beta: yp = ... (FishingHookRenderer.java:51)
			zp = hook.owner->zo + (hook.owner->z - hook.owner->zo) * a - ss * 0.35 + cc * 0.85;  // Beta: zp = ... (FishingHookRenderer.java:52)
		}

		// Beta: Calculate hook position and line (FishingHookRenderer.java:55-74)
		double xh = hook.xo + (hook.x - hook.xo) * a;  // Beta: double xh = ... (FishingHookRenderer.java:55)
		double yh = hook.yo + (hook.y - hook.yo) * a + 0.25;  // Beta: double yh = ... (FishingHookRenderer.java:56)
		double zh = hook.zo + (hook.z - hook.zo) * a;  // Beta: double zh = ... (FishingHookRenderer.java:57)
		double xa = (float)(xp - xh);  // Beta: double xa = (float)(xp - xh) (FishingHookRenderer.java:58)
		double ya = (float)(yp - yh);  // Beta: double ya = (float)(yp - yh) (FishingHookRenderer.java:59)
		double za = (float)(zp - zh);  // Beta: double za = (float)(zp - zh) (FishingHookRenderer.java:60)
		glDisable(GL_TEXTURE_2D);  // Beta: GL11.glDisable(3553) (FishingHookRenderer.java:61)
		glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896) (FishingHookRenderer.java:62)
		t.begin(GL_LINE_STRIP);  // Beta: t.begin(3) - GL_LINE_STRIP (0x0003) (FishingHookRenderer.java:63)
		t.color(0);  // Beta: t.color(0) - black color (0 = RGB black) (FishingHookRenderer.java:64)
		int_t steps = 16;  // Beta: int steps = 16 (FishingHookRenderer.java:65)

		for (int_t i = 0; i <= steps; i++)  // Beta: for (int i = 0; i <= steps; i++) (FishingHookRenderer.java:67)
		{
			float aa = (float)i / steps;  // Beta: float aa = (float)i / steps (FishingHookRenderer.java:68)
			t.vertex(x + xa * aa, y + ya * (aa * aa + aa) * 0.5f + 0.25f, z + za * aa);  // Beta: t.vertex(...) (FishingHookRenderer.java:69)
		}

		t.end();  // Beta: t.end() (FishingHookRenderer.java:72)
		glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896) (FishingHookRenderer.java:73)
		glEnable(GL_TEXTURE_2D);  // Beta: GL11.glEnable(3553) (FishingHookRenderer.java:74)
	}
}
