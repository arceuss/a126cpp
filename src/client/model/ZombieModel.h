#pragma once

#include "client/model/HumanoidModel.h"

// Alpha 1.2.6 ZombieModel
// Reference: newb12/net/minecraft/client/model/ZombieModel.java
class ZombieModel : public HumanoidModel
{
public:
	ZombieModel();

	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
