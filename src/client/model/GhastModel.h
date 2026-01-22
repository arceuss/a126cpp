#pragma once

#include "client/model/Model.h"
#include "client/model/Cube.h"
#include <array>

// Alpha 1.2.6 GhastModel
// Reference: newb12/net/minecraft/client/model/GhastModel.java
class GhastModel : public Model
{
public:
	Cube *body;
	std::array<Cube*, 9> tentacles;

public:
	GhastModel();

	virtual void render(float time, float r, float bob, float yRot, float xRot, float scale) override;
	virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) override;
};
