#include "client/gamemode/SurvivalMode.h"
#include "world/item/ItemStack.h"

#include "client/Minecraft.h"

SurvivalMode::SurvivalMode(Minecraft &minecraft) : GameMode(minecraft)
{

}

void SurvivalMode::initPlayer(std::shared_ptr<Player> player)
{
	player->yRot = -180.0f;
}

void SurvivalMode::init()
{

}

bool SurvivalMode::destroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	int_t t = minecraft.level->getTile(x, y, z);
	int_t data = minecraft.level->getData(x, y, z);
	
	bool changed = GameMode::destroyBlock(x, y, z, face);
	bool couldDestroy = minecraft.player->canDestroy(*Tile::tiles[t]);

	// Alpha: Consume durability when breaking blocks (PlayerControllerSP.java:28-34)
	ItemStack *held = minecraft.player->inventory.getCurrentItem();
	if (held != nullptr && changed)
	{
		// Alpha: hitBlock(blockID, x, y, z) damages item by 1 (ItemTool.java:37-39, PlayerControllerSP.java:29)
		held->hitBlock(t, x, y, z);
		if (held->isEmpty())
		{
			// Alpha: When stackSize reaches 0, destroy item (PlayerControllerSP.java:30-33)
			// Clear inventory slot when item breaks
			minecraft.player->inventory.mainInventory[minecraft.player->inventory.currentItem] = ItemStack();
		}
	}

	if (changed && couldDestroy)
		Tile::tiles[t]->playerDestroy(*minecraft.level, x, y, z, data);

	return changed;
}

void SurvivalMode::startDestroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	int_t t = minecraft.level->getTile(x, y, z);
	if (t > 0 && destroyProgress == 0.0f)
		Tile::tiles[t]->attack(*minecraft.level, x, y, z, *minecraft.player);
	
	if (t > 0 && Tile::tiles[t]->getDestroyProgress(*minecraft.player) >= 1.0f)
		destroyBlock(x, y, z, face);
}

void SurvivalMode::stopDestroyBlock()
{
	destroyProgress = 0.0f;
	destroyDelay = 0;
}

void SurvivalMode::continueDestroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	if (destroyDelay > 0)
	{
		destroyDelay--;
		return;
	}

	if (x == xDestroyBlock && y == yDestroyBlock && z == zDestroyBlock)
	{
		int_t t = minecraft.level->getTile(x, y, z);
		if (t == 0) return;

		Tile &tile = *Tile::tiles[t];
		destroyProgress += tile.getDestroyProgress(*minecraft.player);
		if (std::fmod(destroyTicks, 4.0f) == 0.0f)
		{
			// Beta: Play mining sound during block breaking (SurvivalMode.java:83-88)
			const Tile::SoundType *soundType = tile.getSoundType();
			if (soundType != nullptr)
			{
				jstring stepSound = soundType->getStepSound();
				float volume = (soundType->getVolume() + 1.0f) / 8.0f;
				float pitch = soundType->getPitch() * 0.5f;
				minecraft.soundEngine.play(stepSound, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, volume, pitch);
			}
		}

		destroyTicks++;

		if (destroyProgress >= 1.0f)
		{
			destroyBlock(x, y, z, face);
			destroyProgress = 0.0f;
			oDestroyProgress = 0.0f;
			destroyTicks = 0.0f;
			destroyDelay = 5;
		}
	}
	else
	{
		destroyProgress = 0.0f;
		oDestroyProgress = 0.0f;
		destroyTicks = 0.0f;
		xDestroyBlock = x;
		yDestroyBlock = y;
		zDestroyBlock = z;
	}
}

void SurvivalMode::render(float a)
{
	// TODO
	if (destroyProgress <= 0.0f)
	{
		minecraft.gui.progress = 0.0f;
		minecraft.levelRenderer.destroyProgress = 0.0f;
	}
	else
	{
		float dp = oDestroyProgress + (destroyProgress - oDestroyProgress) * a;
		minecraft.gui.progress = dp;
		minecraft.levelRenderer.destroyProgress = dp;
	}
}

float SurvivalMode::getPickRange()
{
	return 4.0f;
}

void SurvivalMode::initLevel(std::shared_ptr<Level> level)
{
	GameMode::initLevel(level);
}

void SurvivalMode::tick()
{
	oDestroyProgress = destroyProgress;
}
