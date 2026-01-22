#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"
#include <array>

// Alpha 1.2.6 BoatModel
// Reference: newb12/net/minecraft/client/model/BoatModel.java
class BoatModel : public Model
{
public:
	std::array<Cube*, 5> cubes;

public:
	BoatModel();

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
