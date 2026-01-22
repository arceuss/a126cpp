#include "client/model/GhastModel.h"

#include "util/Mth.h"
#include "java/Random.h"
#include <memory>

GhastModel::GhastModel()
{
	int_t yoffs = -16;
	body = new Cube(0, 0);
	body->addBox(-8.0f, -8.0f, -8.0f, 16, 16, 16);
	body->y += 24 + yoffs;
	Random random(1660L);

	for (int_t i = 0; i < tentacles.size(); i++)
	{
		tentacles[i] = new Cube(0, 0);
		float xo = ((i % 3 - i / 3 % 2 * 0.5f + 0.25f) / 2.0f * 2.0f - 1.0f) * 5.0f;
		float yo = (i / 3 / 2.0f * 2.0f - 1.0f) * 5.0f;
		int_t len = random.nextInt(7) + 8;
		tentacles[i]->addBox(-1.0f, 0.0f, -1.0f, 2, len, 2);
		tentacles[i]->x = xo;
		tentacles[i]->z = yo;
		tentacles[i]->y = 31 + yoffs;
	}
}

void GhastModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	setupAnim(time, r, bob, yRot, xRot, scale);
	body->render(scale);

	for (int_t i = 0; i < tentacles.size(); i++)
	{
		tentacles[i]->render(scale);
	}
}

void GhastModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
	for (int_t i = 0; i < tentacles.size(); i++)
	{
		tentacles[i]->xRot = 0.2f * Mth::sin(bob * 0.3f + i) + 0.4f;
	}
}
