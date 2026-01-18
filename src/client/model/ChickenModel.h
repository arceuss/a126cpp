#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"

// newb12: ChickenModel - model for chicken entities
// Reference: newb12/net/minecraft/client/model/ChickenModel.java
class ChickenModel : public Model
{
public:
	Cube head;
	Cube body;
	Cube leg0;
	Cube leg1;
	Cube wing0;
	Cube wing1;
	Cube beak;
	Cube redThing;

public:
	ChickenModel();

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
