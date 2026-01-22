#include "client/renderer/entity/TntRenderer.h"

#include "world/entity/PrimedTnt.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/TntTile.h"
#include "pc/OpenGL.h"

TntRenderer::TntRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
	shadowRadius = 0.5f;
}

void TntRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	PrimedTnt &tnt = static_cast<PrimedTnt &>(entity);
	
	glPushMatrix();
	glTranslatef((float)x, (float)y, (float)z);
	if (tnt.life - a + 1.0f < 10.0f)
	{
		float g = 1.0f - (tnt.life - a + 1.0f) / 10.0f;
		if (g < 0.0f)
		{
			g = 0.0f;
		}

		if (g > 1.0f)
		{
			g = 1.0f;
		}

		g *= g;
		g *= g;
		float s = 1.0f + g * 0.3f;
		glScalef(s, s, s);
	}

	float br = (1.0f - (tnt.life - a + 1.0f) / 100.0f) * 0.8f;
	bindTexture(u"/terrain.png");
	tileRenderer.renderTile(static_cast<Tile&>(Tile::tnt), 0);
	if (tnt.life / 5 % 2 == 0)
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 1.0f, 1.0f, br);
		tileRenderer.renderTile(static_cast<Tile&>(Tile::tnt), 0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
	}

	glPopMatrix();
}
