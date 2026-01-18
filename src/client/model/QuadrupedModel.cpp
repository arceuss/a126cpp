#include "client/model/QuadrupedModel.h"

#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// newb12: QuadrupedModel constructor
// Reference: newb12/net/minecraft/client/model/QuadrupedModel.java:14-32
QuadrupedModel::QuadrupedModel(int legSize, float g)
	: head(0, 0), body(28, 8), leg0(0, 16), leg1(0, 16), leg2(0, 16), leg3(0, 16)
{
	head.addBox(-4.0f, -4.0f, -8.0f, 8, 8, 8, g);
	head.setPos(0.0f, 18.0f - legSize, -6.0f);
	body.addBox(-5.0f, -10.0f, -7.0f, 10, 16, 8, g);
	body.setPos(0.0f, 17.0f - legSize, 2.0f);
	leg0.addBox(-2.0f, 0.0f, -2.0f, 4, legSize, 4, g);
	leg0.setPos(-3.0f, 24.0f - legSize, 7.0f);
	leg1.addBox(-2.0f, 0.0f, -2.0f, 4, legSize, 4, g);
	leg1.setPos(3.0f, 24.0f - legSize, 7.0f);
	leg2.addBox(-2.0f, 0.0f, -2.0f, 4, legSize, 4, g);
	leg2.setPos(-3.0f, 24.0f - legSize, -5.0f);
	leg3.addBox(-2.0f, 0.0f, -2.0f, 4, legSize, 4, g);
	leg3.setPos(3.0f, 24.0f - legSize, -5.0f);
}

// newb12: QuadrupedModel.render()
// Reference: newb12/net/minecraft/client/model/QuadrupedModel.java:34-43
void QuadrupedModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	setupAnim(time, r, bob, yRot, xRot, scale);
	head.render(scale);
	body.render(scale);
	leg0.render(scale);
	leg1.render(scale);
	leg2.render(scale);
	leg3.render(scale);
}

// newb12: QuadrupedModel.setupAnim()
// Reference: newb12/net/minecraft/client/model/QuadrupedModel.java:45-54
void QuadrupedModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
	head.xRot = -(xRot / (180.0f / M_PI));
	head.yRot = yRot / (180.0f / M_PI);
	body.xRot = (float)(M_PI / 2);
	leg0.xRot = Mth::cos(time * 0.6662f) * 1.4f * r;
	leg1.xRot = Mth::cos(time * 0.6662f + (float)M_PI) * 1.4f * r;
	leg2.xRot = Mth::cos(time * 0.6662f + (float)M_PI) * 1.4f * r;
	leg3.xRot = Mth::cos(time * 0.6662f) * 1.4f * r;
}

// newb12: QuadrupedModel.render(QuadrupedModel, float)
// Reference: newb12/net/minecraft/client/model/QuadrupedModel.java:56-73
void QuadrupedModel::render(QuadrupedModel &model, float scale)
{
	head.yRot = model.head.yRot;
	head.xRot = model.head.xRot;
	head.y = model.head.y;
	head.x = model.head.x;
	body.yRot = model.body.yRot;
	body.xRot = model.body.xRot;
	leg0.xRot = model.leg0.xRot;
	leg1.xRot = model.leg1.xRot;
	leg2.xRot = model.leg2.xRot;
	leg3.xRot = model.leg3.xRot;
	head.render(scale);
	body.render(scale);
	leg0.render(scale);
	leg1.render(scale);
	leg2.render(scale);
	leg3.render(scale);
}
