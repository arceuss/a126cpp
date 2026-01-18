#pragma once

#include "client/model/QuadrupedModel.h"

// newb12: CowModel - model for cow entities
// Reference: newb12/net/minecraft/client/model/CowModel.java
class CowModel : public QuadrupedModel
{
public:
	Cube udder;
	Cube horn1;
	Cube horn2;

public:
	CowModel();

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
