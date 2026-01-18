#include "client/model/SheepFurModel.h"

// newb12: SheepFurModel constructor
// Reference: newb12/net/minecraft/client/model/SheepFurModel.java:4-25
SheepFurModel::SheepFurModel() : QuadrupedModel(12, 0.0f)
{
	// SheepFurModel overrides head, body, and leg cubes
	head.setTexOffs(0, 0);
	head.addBox(-3.0f, -4.0f, -4.0f, 6, 6, 6, 0.6f);
	head.setPos(0.0f, 6.0f, -8.0f);
	body.setTexOffs(28, 8);
	body.addBox(-4.0f, -10.0f, -7.0f, 8, 16, 6, 1.75f);
	body.setPos(0.0f, 5.0f, 2.0f);
	float g = 0.5f;
	leg0.setTexOffs(0, 16);
	leg0.addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg0.setPos(-3.0f, 12.0f, 7.0f);
	leg1.setTexOffs(0, 16);
	leg1.addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg1.setPos(3.0f, 12.0f, 7.0f);
	leg2.setTexOffs(0, 16);
	leg2.addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg2.setPos(-3.0f, 12.0f, -5.0f);
	leg3.setTexOffs(0, 16);
	leg3.addBox(-2.0f, 0.0f, -2.0f, 4, 6, 4, g);
	leg3.setPos(3.0f, 12.0f, -5.0f);
}
