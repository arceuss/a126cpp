#include "client/renderer/entity/CreeperRenderer.h"

#include "world/entity/monster/Creeper.h"
#include "client/model/CreeperModel.h"
#include "util/Mth.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"

CreeperRenderer::CreeperRenderer(EntityRenderDispatcher &entityRenderDispatcher) : MobRenderer(entityRenderDispatcher, Util::make_shared<CreeperModel>(), 0.5f)
{
}

void CreeperRenderer::scale(Mob &mob, float a)
{
	Creeper &creeper = static_cast<Creeper &>(mob);
	float g = creeper.getSwelling(a);
	float wobble = 1.0f + Mth::sin(g * 100.0f) * g * 0.01f;
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
	float s = (1.0f + g * 0.4f) * wobble;
	float hs = (1.0f + g * 0.1f) / wobble;
	glScalef(s, hs, s);
}

int_t CreeperRenderer::getOverlayColor(Mob &mob, float br, float a)
{
	Creeper &creeper = static_cast<Creeper &>(mob);
	float step = creeper.getSwelling(a);
	if ((int_t)(step * 10.0f) % 2 == 0)
	{
		return 0;
	}
	else
	{
		int_t _a = (int_t)(step * 0.2f * 255.0f);
		if (_a < 0)
		{
			_a = 0;
		}

		if (_a > 255)
		{
			_a = 255;
		}

		int_t r = 255;
		int_t g = 255;
		int_t b = 255;
		return _a << 24 | r << 16 | g << 8 | b;
	}
}
