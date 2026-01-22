#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"

// Alpha 1.2.6 CreeperModel
// Reference: newb12/net/minecraft/client/model/CreeperModel.java
class CreeperModel : public Model
{
public:
	Cube *head;
	Cube *hair;
	Cube *body;
	Cube *leg0;
	Cube *leg1;
	Cube *leg2;
	Cube *leg3;

public:
	CreeperModel();

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
