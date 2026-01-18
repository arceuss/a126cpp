#include "client/renderer/entity/GhastRenderer.h"

#include "world/entity/monster/Ghast.h"
#include "client/model/GhastModel.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"

GhastRenderer::GhastRenderer(EntityRenderDispatcher &entityRenderDispatcher) : MobRenderer(entityRenderDispatcher, Util::make_shared<GhastModel>(), 0.5f)
{
}

void GhastRenderer::scale(Mob &mob, float a)
{
	Ghast &ghast = static_cast<Ghast &>(mob);
	float ss = (ghast.oCharge + (ghast.charge - ghast.oCharge) * a) / 20.0f;
	if (ss < 0.0f)
	{
		ss = 0.0f;
	}

	ss = 1.0f / (ss * ss * ss * ss * ss * 2.0f + 1.0f);
	float s = (8.0f + ss) / 2.0f;
	float hs = (8.0f + 1.0f / ss) / 2.0f;
	glScalef(hs, s, hs);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
