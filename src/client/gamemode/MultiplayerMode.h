#pragma once

#include "client/gamemode/GameMode.h"

class NetClientHandler;

class MultiplayerMode : public GameMode
{
private:
	int_t currentBlockX = -1;
	int_t currentBlockY = -1;
	int_t currentBlockZ = -1;
	Facing currentBlockFace = Facing::NONE;  // Alpha 1.2.6: Store face when starting to break
	float curBlockDamageMP = 0.0f;
	float prevBlockDamageMP = 0.0f;
	float field_9441_h = 0.0f;
	int_t blockHitDelay = 0;
	bool isHittingBlock = false;
	NetClientHandler* netHandler;
	int_t field_1075_l = 0;  // Last synced current item index

public:
	MultiplayerMode(Minecraft &minecraft, NetClientHandler* netHandler);

	void initPlayer(std::shared_ptr<Player> player) override;

	void startDestroyBlock(int_t x, int_t y, int_t z, Facing face) override;
	bool destroyBlock(int_t x, int_t y, int_t z, Facing face) override;
	void continueDestroyBlock(int_t x, int_t y, int_t z, Facing face) override;
	void stopDestroyBlock() override;

	void render(float a) override;

	float getPickRange() override;

	bool useItem(std::shared_ptr<Player> &player, std::shared_ptr<Level> &level, ItemStack *item) override;
	bool useItemOn(std::shared_ptr<Player> &player, std::shared_ptr<Level> &level, ItemStack *item, int_t x, int_t y, int_t z, Facing face) override;

	void interact(std::shared_ptr<Player> &player, std::shared_ptr<Entity> &entity) override;
	void attack(std::shared_ptr<Player> &player, std::shared_ptr<Entity> &entity) override;

	void tick() override;

	void initLevel(std::shared_ptr<Level> level) override;

	std::shared_ptr<Player> createPlayer(Level &level) override;

	// Alpha 1.2.6: PlayerControllerMP.func_27174_a - handle window clicks
	// If itemBefore is provided, uses it; otherwise captures the current state
	ItemStack handleWindowClick(int windowId, int slot, int mouseClick, bool shiftClick, std::shared_ptr<Player> player, std::unique_ptr<ItemStack> itemBefore = nullptr);

private:
	void syncCurrentPlayItem();
	void clickBlock(int_t x, int_t y, int_t z, Facing face);
};
