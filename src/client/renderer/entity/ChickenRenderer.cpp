#include "client/renderer/entity/ChickenRenderer.h"

#include "world/entity/animal/Chicken.h"
#include "util/Mth.h"

// newb12: ChickenRenderer constructor
// Reference: newb12/net/minecraft/client/renderer/entity/ChickenRenderer.java:7-9
ChickenRenderer::ChickenRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<Model> &model, float shadow)
	: MobRenderer(entityRenderDispatcher, model, shadow)
{
}

// newb12: ChickenRenderer.getBob()
// Reference: newb12/net/minecraft/client/renderer/entity/ChickenRenderer.java:16-20
float ChickenRenderer::getBob(Mob &mob, float a)
{
	Chicken &chicken = static_cast<Chicken &>(mob);
	float flap = chicken.oFlap + (chicken.flap - chicken.oFlap) * a;
	float flapSpeed = chicken.oFlapSpeed + (chicken.flapSpeed - chicken.oFlapSpeed) * a;
	return (Mth::sin(flap) + 1.0f) * flapSpeed;
}
