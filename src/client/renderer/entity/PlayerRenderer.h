#pragma once

#include "client/renderer/entity/MobRenderer.h"
#include "client/renderer/TileRenderer.h"

class PlayerRenderer : public MobRenderer
{
private:
	std::shared_ptr<HumanoidModel> humanoidModel;
	std::shared_ptr<HumanoidModel> armorParts1;
	std::shared_ptr<HumanoidModel> armorParts2;
	TileRenderer tileRenderer;  // For rendering blocks in third-person view

public:
	PlayerRenderer(EntityRenderDispatcher &entityRenderDispatcher);

	void render(Entity &entity, double x, double y, double z, float rot, float a) override;

protected:
	void scale(Mob &mob, float a) override;
	void additionalRendering(Mob &mob, float a) override;
	bool prepareArmor(Mob &mob, int_t layer, float a) override;

public:
	void renderHand();

private:
	// Alpha 1.2.6: Render nametag above player (PlayerRenderer.java:60-103)
	void renderNameTag(Player &player, double x, double y, double z, float a);
};
