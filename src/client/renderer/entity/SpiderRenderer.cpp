#include "client/renderer/entity/SpiderRenderer.h"

#include "world/entity/monster/Spider.h"
#include "client/model/SpiderModel.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"

SpiderRenderer::SpiderRenderer(EntityRenderDispatcher &entityRenderDispatcher) : MobRenderer(entityRenderDispatcher, Util::make_shared<SpiderModel>(), 1.0f)
{
	setArmor(Util::make_shared<SpiderModel>());
}

float SpiderRenderer::getFlipDegrees(Mob &mob)
{
	return 180.0f;
}

bool SpiderRenderer::prepareArmor(Mob &mob, int_t layer, float a)
{
	if (layer != 0)
	{
		return false;
	}
	else if (layer != 0)
	{
		return false;
	}
	else
	{
		Spider &spider = static_cast<Spider &>(mob);
		bindTexture(u"/mob/spider_eyes.png");
		float br = (1.0f - spider.getBrightness(1.0f)) * 0.5f;
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(1.0f, 1.0f, 1.0f, br);
		return true;
	}
}
