#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"

// Alpha 1.2.6 SpiderModel
// Reference: newb12/net/minecraft/client/model/SpiderModel.java
class SpiderModel : public Model
{
public:
	Cube *head;
	Cube *body0;
	Cube *body1;
	Cube *leg0;
	Cube *leg1;
	Cube *leg2;
	Cube *leg3;
	Cube *leg4;
	Cube *leg5;
	Cube *leg6;
	Cube *leg7;

public:
	SpiderModel();

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
