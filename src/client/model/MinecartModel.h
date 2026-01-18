#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"
#include <array>

// Alpha 1.2.6 MinecartModel
// Reference: newb12/net/minecraft/client/model/MinecartModel.java
class MinecartModel : public Model
{
public:
	std::array<Cube*, 7> cubes;

public:
	MinecartModel();

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
