#include "client/model/SlimeModel.h"

SlimeModel::SlimeModel(int_t vOffs)
{
	cube = new Cube(0, vOffs);
	cube->addBox(-4.0f, 16.0f, -4.0f, 8, 8, 8);
	if (vOffs > 0)
	{
		cube = new Cube(0, vOffs);
		cube->addBox(-3.0f, 17.0f, -3.0f, 6, 6, 6);
		eye0 = new Cube(32, 0);
		eye0->addBox(-3.25f, 18.0f, -3.5f, 2, 2, 2);
		eye1 = new Cube(32, 4);
		eye1->addBox(1.25f, 18.0f, -3.5f, 2, 2, 2);
		mouth = new Cube(32, 8);
		mouth->addBox(0.0f, 21.0f, -3.5f, 1, 1, 1);
	}
	else
	{
		eye0 = nullptr;
		eye1 = nullptr;
		mouth = nullptr;
	}
}

void SlimeModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	setupAnim(time, r, bob, yRot, xRot, scale);
	cube->render(scale);
	if (eye0 != nullptr)
	{
		eye0->render(scale);
		eye1->render(scale);
		mouth->render(scale);
	}
}

void SlimeModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
}
