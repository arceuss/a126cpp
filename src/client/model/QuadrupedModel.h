#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"

// newb12: QuadrupedModel - base model for four-legged animals
// Reference: newb12/net/minecraft/client/model/QuadrupedModel.java
class QuadrupedModel : public Model
{
public:
	Cube head;
	Cube body;
	Cube leg0;
	Cube leg1;
	Cube leg2;
	Cube leg3;

public:
	QuadrupedModel(int legSize, float g);

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;

	// newb12: QuadrupedModel.render(QuadrupedModel, float) - renders another model's state
	// Reference: newb12/net/minecraft/client/model/QuadrupedModel.java:56-73
	void render(QuadrupedModel &model, float scale);
};
