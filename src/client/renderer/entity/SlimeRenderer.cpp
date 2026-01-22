#include "client/renderer/entity/SlimeRenderer.h"

#include "world/entity/monster/Slime.h"
#include "pc/OpenGL.h"

SlimeRenderer::SlimeRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, const std::shared_ptr<Model> &armor, float shadow) : MobRenderer(entityRenderDispatcher, model, shadow)
{
	this->armor = armor;
}

bool SlimeRenderer::prepareArmor(Mob &mob, int_t layer, float a)
{
	if (layer == 0)
	{
		setArmor(armor);
		glEnable(GL_NORMALIZE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		return true;
	}
	else
	{
		if (layer == 1)
		{
			glDisable(GL_BLEND);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}

		return false;
	}
}

void SlimeRenderer::scale(Mob &mob, float a)
{
	Slime &slime = static_cast<Slime &>(mob);
	float ss = (slime.oSquish + (slime.squish - slime.oSquish) * a) / (slime.size * 0.5f + 1.0f);
	float w = 1.0f / (ss + 1.0f);
	float s = slime.size;
	glScalef(w * s, 1.0f / w * s, w * s);
}
