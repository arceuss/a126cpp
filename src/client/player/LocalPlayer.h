#pragma once

#include "client/player/Input.h"

#include "world/entity/player/Player.h"

#include "nbt/CompoundTag.h"

#include "java/Type.h"

class Minecraft;
class User;

class LocalPlayer : public Player
{
public:
	std::unique_ptr<Input> input;

protected:
	Minecraft &minecraft;

public:
	int_t changingDimensionDelay = 20;

	float portalTime = 0.0f;
	float oPortalTime = 0.0f;

private:
	bool isInsidePortal = false;
	bool wasInWaterLastTick = false;  // Track water entry for rumble (Controlify-style)

public:
	LocalPlayer(Minecraft &minecraft, Level &level, User *user, int_t dimension);

	void updateAi() override;
	void aiStep() override;

	void releaseAllKeys();
	void setKey(int_t eventKey, bool eventKeyState);

	void addAdditionalSaveData(CompoundTag &tag) override;
	void readAdditionalSaveData(CompoundTag &tag) override;

	void closeContainer() override;

	void take(Entity &entity, int_t count) override;  // Beta: LocalPlayer.take() - adds TakeAnimationParticle (LocalPlayer.java:139-141)

	// Beta: LocalPlayer.startCrafting() - opens workbench screen (LocalPlayer.java:124-126)
	void startCrafting(int_t x, int_t y, int_t z);
	
	// Beta: LocalPlayer.openContainer() - opens chest screen (LocalPlayer.java:119-121)
	void openContainer(std::shared_ptr<class ChestTileEntity> chestEntity);
	void openContainer(std::shared_ptr<class CompoundContainer> compoundContainer);
	
	// Beta: LocalPlayer.openFurnace() - opens furnace screen (LocalPlayer.java:129-131)
	void openFurnace(std::shared_ptr<class FurnaceTileEntity> furnaceEntity);
	
	// Beta: LocalPlayer.openTextEdit() - opens sign edit screen (LocalPlayer.java:114-116)
	void openTextEdit(std::shared_ptr<class SignTileEntity> signEntity) override;
	
	void prepareForTick();

	bool isSneaking() override;
	
	// Controller input switching (Controlify-style)
	void updatePlayerInput();
	void ensureCorrectInput();
	static bool shouldBeControllerInput();
	
	// Beta 1.2: LocalPlayer.handleInsidePortal() - matches newb12 LocalPlayer.java:158-165
	void handleInsidePortal() override;
	
	// Beta 1.2: LocalPlayer.hurtTo() - matches newb12 LocalPlayer.java:167-178
	void hurtTo(int_t newHealth);
	
	// Beta 1.2: LocalPlayer.chat() - empty method (LocalPlayer.java:147-148), overridden in MultiplayerLocalPlayer
	virtual void chat(const jstring& message);

	void respawn() override;
	
	// Override actuallyHurt to add rumble support (Controlify-style)
	void actuallyHurt(int_t dmg) override;
};
