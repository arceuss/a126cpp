#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"

// Alpha 1.2.6 SlimeModel
// Reference: newb12/net/minecraft/client/model/SlimeModel.java
class SlimeModel : public Model
{
public:
	Cube *cube;
	Cube *eye0;
	Cube *eye1;
	Cube *mouth;

public:
	SlimeModel(int_t vOffs);

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
