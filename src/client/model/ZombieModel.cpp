#include "client/model/ZombieModel.h"

#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

ZombieModel::ZombieModel() : HumanoidModel()
{
}

void ZombieModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
	HumanoidModel::setupAnim(time, r, bob, yRot, xRot, scale);
	float attack2 = Mth::sin(attackTime * (float)M_PI);
	float attack = Mth::sin((1.0f - (1.0f - attackTime) * (1.0f - attackTime)) * (float)M_PI);
	arm0.zRot = 0.0f;
	arm1.zRot = 0.0f;
	arm0.yRot = -(0.1f - attack2 * 0.6f);
	arm1.yRot = 0.1f - attack2 * 0.6f;
	arm0.xRot = (float)(-M_PI / 2);
	arm1.xRot = (float)(-M_PI / 2);
	arm0.xRot -= attack2 * 1.2f - attack * 0.4f;
	arm1.xRot -= attack2 * 1.2f - attack * 0.4f;
	arm0.zRot = arm0.zRot + (Mth::cos(bob * 0.09f) * 0.05f + 0.05f);
	arm1.zRot = arm1.zRot - (Mth::cos(bob * 0.09f) * 0.05f + 0.05f);
	arm0.xRot = arm0.xRot + Mth::sin(bob * 0.067f) * 0.05f;
	arm1.xRot = arm1.xRot - Mth::sin(bob * 0.067f) * 0.05f;
}
