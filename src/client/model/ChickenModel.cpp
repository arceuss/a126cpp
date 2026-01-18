#include "client/model/ChickenModel.h"

#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// newb12: ChickenModel constructor
// Reference: newb12/net/minecraft/client/model/ChickenModel.java:16-42
ChickenModel::ChickenModel() : head(0, 0), body(0, 9), leg0(26, 0), leg1(26, 0), wing0(24, 13), wing1(24, 13), beak(14, 0), redThing(14, 4)
{
	int yo = 16;
	head.addBox(-2.0f, -6.0f, -2.0f, 4, 6, 3, 0.0f);
	head.setPos(0.0f, -1 + yo, -4.0f);
	beak.addBox(-2.0f, -4.0f, -4.0f, 4, 2, 2, 0.0f);
	beak.setPos(0.0f, -1 + yo, -4.0f);
	redThing.addBox(-1.0f, -2.0f, -3.0f, 2, 2, 2, 0.0f);
	redThing.setPos(0.0f, -1 + yo, -4.0f);
	body.addBox(-3.0f, -4.0f, -3.0f, 6, 8, 6, 0.0f);
	body.setPos(0.0f, 0 + yo, 0.0f);
	leg0.addBox(-1.0f, 0.0f, -3.0f, 3, 5, 3);
	leg0.setPos(-2.0f, 3 + yo, 1.0f);
	leg1.addBox(-1.0f, 0.0f, -3.0f, 3, 5, 3);
	leg1.setPos(1.0f, 3 + yo, 1.0f);
	wing0.addBox(0.0f, 0.0f, -3.0f, 1, 4, 6);
	wing0.setPos(-4.0f, -3 + yo, 0.0f);
	wing1.addBox(-1.0f, 0.0f, -3.0f, 1, 4, 6);
	wing1.setPos(4.0f, -3 + yo, 0.0f);
}

// newb12: ChickenModel.render()
// Reference: newb12/net/minecraft/client/model/ChickenModel.java:44-55
void ChickenModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	setupAnim(time, r, bob, yRot, xRot, scale);
	head.render(scale);
	beak.render(scale);
	redThing.render(scale);
	body.render(scale);
	leg0.render(scale);
	leg1.render(scale);
	wing0.render(scale);
	wing1.render(scale);
}

// newb12: ChickenModel.setupAnim()
// Reference: newb12/net/minecraft/client/model/ChickenModel.java:57-70
void ChickenModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
	head.xRot = -(xRot / (180.0f / M_PI));
	head.yRot = yRot / (180.0f / M_PI);
	beak.xRot = head.xRot;
	beak.yRot = head.yRot;
	redThing.xRot = head.xRot;
	redThing.yRot = head.yRot;
	body.xRot = (float)(M_PI / 2);
	leg0.xRot = Mth::cos(time * 0.6662f) * 1.4f * r;
	leg1.xRot = Mth::cos(time * 0.6662f + (float)M_PI) * 1.4f * r;
	wing0.zRot = bob;
	wing1.zRot = -bob;
}
