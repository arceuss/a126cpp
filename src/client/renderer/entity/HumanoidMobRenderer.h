#pragma once

#include "client/renderer/entity/MobRenderer.h"
#include "client/model/HumanoidModel.h"

// Alpha 1.2.6 HumanoidMobRenderer
// Reference: newb12/net/minecraft/client/renderer/entity/HumanoidMobRenderer.java
class HumanoidMobRenderer : public MobRenderer
{
protected:
	std::shared_ptr<HumanoidModel> humanoidModel;

public:
	HumanoidMobRenderer(EntityRenderDispatcher &entityRenderDispatcher, const std::shared_ptr<HumanoidModel> &humanoidModel, float shadow);

protected:
	virtual void additionalRendering(Mob &mob, float a) override;
};
