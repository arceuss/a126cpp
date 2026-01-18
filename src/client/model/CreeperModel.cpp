#include "client/model/CreeperModel.h"

#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

CreeperModel::CreeperModel()
{
	float g = 0.0f;
	int_t yo = 4;
	head = new Cube(0, 0);
	head->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, g);
	head->setPos(0.0f, yo, 0.0f);
	hair = new Cube(32, 0);
	hair->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, g + 0.5f);
	hair->setPos(0.0f, yo, 0.0f);
	body = new Cube(16, 16);
	body->addBox(-4.0f, 0.0f, -2.0f, 8, 12, 4, g);
	body->setPos(0.0f, yo, 0.0f);
	leg0 = new Cube(0, 16);
	leg0->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg0->setPos(-2.0f, 12 + yo, 4.0f);
	leg1 = new Cube(0, 16);
	leg1->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg1->setPos(2.0f, 12 + yo, 4.0f);
	leg2 = new Cube(0, 16);
	leg2->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg2->setPos(-2.0f, 12 + yo, -4.0f);
	leg3 = new Cube(0, 16);
	leg3->addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg3->setPos(2.0f, 12 + yo, -4.0f);
}

void CreeperModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	setupAnim(time, r, bob, yRot, xRot, scale);
	head->render(scale);
	body->render(scale);
	leg0->render(scale);
	leg1->render(scale);
	leg2->render(scale);
	leg3->render(scale);
}

void CreeperModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
	head->yRot = yRot / (180.0f / (float)M_PI);
	head->xRot = xRot / (180.0f / (float)M_PI);
	leg0->xRot = Mth::cos(time * 0.6662f) * 1.4f * r;
	leg1->xRot = Mth::cos(time * 0.6662f + (float)M_PI) * 1.4f * r;
	leg2->xRot = Mth::cos(time * 0.6662f + (float)M_PI) * 1.4f * r;
	leg3->xRot = Mth::cos(time * 0.6662f) * 1.4f * r;
}
