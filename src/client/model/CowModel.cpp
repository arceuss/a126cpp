#include "client/model/CowModel.h"

#include "util/Mth.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// newb12: CowModel constructor
// Reference: newb12/net/minecraft/client/model/CowModel.java:8-34
CowModel::CowModel() : QuadrupedModel(12, 0.0f), udder(52, 0), horn1(22, 0), horn2(22, 0)
{
	// CowModel overrides head and body cubes
	head.setTexOffs(0, 0);
	head.addBox(-4.0f, -4.0f, -6.0f, 8, 8, 6, 0.0f);
	head.setPos(0.0f, 4.0f, -8.0f);
	horn1.addBox(-4.0f, -5.0f, -4.0f, 1, 3, 1, 0.0f);
	horn1.setPos(0.0f, 3.0f, -7.0f);
	horn2.addBox(4.0f, -5.0f, -4.0f, 1, 3, 1, 0.0f);
	horn2.setPos(0.0f, 3.0f, -7.0f);
	udder.addBox(-2.0f, -3.0f, 0.0f, 4, 6, 2, 0.0f);
	udder.setPos(0.0f, 14.0f, 6.0f);
	udder.xRot = (float)(M_PI / 2);
	body.setTexOffs(18, 4);
	body.addBox(-6.0f, -10.0f, -7.0f, 12, 18, 10, 0.0f);
	body.setPos(0.0f, 5.0f, 2.0f);
	leg0.x--;
	leg1.x++;
	leg0.z += 0.0f;
	leg1.z += 0.0f;
	leg2.x--;
	leg3.x++;
	leg2.z--;
	leg3.z--;
}

// newb12: CowModel.render()
// Reference: newb12/net/minecraft/client/model/CowModel.java:36-42
void CowModel::render(float time, float r, float bob, float yRot, float xRot, float scale)
{
	QuadrupedModel::render(time, r, bob, yRot, xRot, scale);
	horn1.render(scale);
	horn2.render(scale);
	udder.render(scale);
}

// newb12: CowModel.setupAnim()
// Reference: newb12/net/minecraft/client/model/CowModel.java:44-51
void CowModel::setupAnim(float time, float r, float bob, float yRot, float xRot, float scale)
{
	QuadrupedModel::setupAnim(time, r, bob, yRot, xRot, scale);
	horn1.yRot = head.yRot;
	horn1.xRot = head.xRot;
	horn2.yRot = head.yRot;
	horn2.xRot = head.xRot;
}
