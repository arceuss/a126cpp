#include "client/model/BoatModel.h"

#include "util/Mth.h"
#include <memory>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

BoatModel::BoatModel()
{
	cubes[0] = new Cube(0, 8);
	cubes[1] = new Cube(0, 0);
	cubes[2] = new Cube(0, 0);
	cubes[3] = new Cube(0, 0);
	cubes[4] = new Cube(0, 0);
	int_t w = 24;
	int_t d = 6;
	int_t h = 20;
	int_t yOff = 4;
	cubes[0]->addBox(-w / 2, -h / 2 + 2, -3.0f, w, h - 4, 4, 0.0f);
	cubes[0]->setPos(0.0f, 0 + yOff, 0.0f);
	cubes[1]->addBox(-w / 2 + 2, -d - 1, -1.0f, w - 4, d, 2, 0.0f);
	cubes[1]->setPos(-w / 2 + 1, 0 + yOff, 0.0f);
	cubes[2]->addBox(-w / 2 + 2, -d - 1, -1.0f, w - 4, d, 2, 0.0f);
	cubes[2]->setPos(w / 2 - 1, 0 + yOff, 0.0f);
	cubes[3]->addBox(-w / 2 + 2, -d - 1, -1.0f, w - 4, d, 2, 0.0f);
	cubes[3]->setPos(0.0f, 0 + yOff, -h / 2 + 1);
	cubes[4]->addBox(-w / 2 + 2, -d - 1, -1.0f, w - 4, d, 2, 0.0f);
	cubes[4]->setPos(0.0f, 0 + yOff, h / 2 - 1);
	cubes[0]->xRot = (float)(M_PI / 2);
	cubes[1]->yRot = (float)(M_PI * 3.0 / 2.0);
	cubes[2]->yRot = (float)(M_PI / 2);
	cubes[3]->yRot = (float)M_PI;
}

void BoatModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	for (int_t i = 0; i < 5; i++)
	{
		cubes[i]->render(scale);
	}
}

void BoatModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
}
