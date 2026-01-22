#include "client/model/SpiderModel.h"

#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

SpiderModel::SpiderModel()
{
	float g = 0.0f;
	int_t yo = 15;
	head = new Cube(32, 4);
	head->addBox(-4.0f, -4.0f, -8.0f, 8, 8, 8, g);
	head->setPos(0.0f, 0 + yo, -3.0f);
	body0 = new Cube(0, 0);
	body0->addBox(-3.0f, -3.0f, -3.0f, 6, 6, 6, g);
	body0->setPos(0.0f, yo, 0.0f);
	body1 = new Cube(0, 12);
	body1->addBox(-5.0f, -4.0f, -6.0f, 10, 8, 12, g);
	body1->setPos(0.0f, 0 + yo, 9.0f);
	leg0 = new Cube(18, 0);
	leg0->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg0->setPos(-4.0f, 0 + yo, 2.0f);
	leg1 = new Cube(18, 0);
	leg1->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg1->setPos(4.0f, 0 + yo, 2.0f);
	leg2 = new Cube(18, 0);
	leg2->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg2->setPos(-4.0f, 0 + yo, 1.0f);
	leg3 = new Cube(18, 0);
	leg3->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg3->setPos(4.0f, 0 + yo, 1.0f);
	leg4 = new Cube(18, 0);
	leg4->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg4->setPos(-4.0f, 0 + yo, 0.0f);
	leg5 = new Cube(18, 0);
	leg5->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg5->setPos(4.0f, 0 + yo, 0.0f);
	leg6 = new Cube(18, 0);
	leg6->addBox(-15.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg6->setPos(-4.0f, 0 + yo, -1.0f);
	leg7 = new Cube(18, 0);
	leg7->addBox(-1.0f, -1.0f, -1.0f, 16, 2, 2, g);
	leg7->setPos(4.0f, 0 + yo, -1.0f);
}

void SpiderModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	setupAnim(time, r, bob, yRot, xRot, scale);
	head->render(scale);
	body0->render(scale);
	body1->render(scale);
	leg0->render(scale);
	leg1->render(scale);
	leg2->render(scale);
	leg3->render(scale);
	leg4->render(scale);
	leg5->render(scale);
	leg6->render(scale);
	leg7->render(scale);
}

void SpiderModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
	head->yRot = yRot / (180.0f / (float)M_PI);
	head->xRot = xRot / (180.0f / (float)M_PI);
	float sr = (float)(M_PI / 4);
	leg0->zRot = -sr;
	leg1->zRot = sr;
	leg2->zRot = -sr * 0.74f;
	leg3->zRot = sr * 0.74f;
	leg4->zRot = -sr * 0.74f;
	leg5->zRot = sr * 0.74f;
	leg6->zRot = -sr;
	leg7->zRot = sr;
	float ro = -0.0f;
	float ur = (float)(M_PI / 8);
	leg0->yRot = ur * 2.0f + ro;
	leg1->yRot = -ur * 2.0f - ro;
	leg2->yRot = ur * 1.0f + ro;
	leg3->yRot = -ur * 1.0f - ro;
	leg4->yRot = -ur * 1.0f + ro;
	leg5->yRot = ur * 1.0f - ro;
	leg6->yRot = -ur * 2.0f + ro;
	leg7->yRot = ur * 2.0f - ro;
	float c0 = -(Mth::cos(time * 0.6662f * 2.0f + 0.0f) * 0.4f) * r;
	float c1 = -(Mth::cos(time * 0.6662f * 2.0f + (float)M_PI) * 0.4f) * r;
	float c2 = -(Mth::cos(time * 0.6662f * 2.0f + (float)(M_PI / 2)) * 0.4f) * r;
	float c3 = -(Mth::cos(time * 0.6662f * 2.0f + (float)(M_PI * 3.0 / 2.0)) * 0.4f) * r;
	float s0 = std::abs(Mth::sin(time * 0.6662f + 0.0f) * 0.4f) * r;
	float s1 = std::abs(Mth::sin(time * 0.6662f + (float)M_PI) * 0.4f) * r;
	float s2 = std::abs(Mth::sin(time * 0.6662f + (float)(M_PI / 2)) * 0.4f) * r;
	float s3 = std::abs(Mth::sin(time * 0.6662f + (float)(M_PI * 3.0 / 2.0)) * 0.4f) * r;
	leg0->yRot += c0;
	leg1->yRot += -c0;
	leg2->yRot += c1;
	leg3->yRot += -c1;
	leg4->yRot += c2;
	leg5->yRot += -c2;
	leg6->yRot += c3;
	leg7->yRot += -c3;
	leg0->zRot += s0;
	leg1->zRot += -s0;
	leg2->zRot += s1;
	leg3->zRot += -s1;
	leg4->zRot += s2;
	leg5->zRot += -s2;
	leg6->zRot += s3;
	leg7->zRot += -s3;
}
