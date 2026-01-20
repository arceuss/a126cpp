#include "client/gamemode/SurvivalMode.h"
#include "world/item/ItemStack.h"

#include "client/Minecraft.h"
#include "client/player/LocalPlayer.h"
#include "client/player/ControllerInput.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include <cmath>
#include <algorithm>

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
	{
		// Instantly breakable - no rumble needed
		destroyBlock(x, y, z, face);
	}
	else if (t > 0)
	{
		// Start breaking - start rumble
		xDestroyBlock = x;
		yDestroyBlock = y;
		zDestroyBlock = z;
		startBlockBreakRumble(x, y, z);
	}
}

void SurvivalMode::stopDestroyBlock()
{
	destroyProgress = 0.0f;
	destroyDelay = 0;
	
	// Stop block break rumble (Controlify-style)
	stopBlockBreakRumble();
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
			// Stop block break rumble before destroying block (Controlify-style)
			stopBlockBreakRumble();
			
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
	
	// Update block break rumble every tick while breaking (Controlify-style)
	if (destroyProgress > 0.0f && blockBreakRumbleStrength > 0.0f)
	{
		updateBlockBreakRumble();
	}
}

void SurvivalMode::startBlockBreakRumble(int_t x, int_t y, int_t z)
{
	// Only rumble for LocalPlayer with controller input
	LocalPlayer* localPlayer = dynamic_cast<LocalPlayer*>(minecraft.player.get());
	if (localPlayer == nullptr)
		return;
	
	ControllerInput* controllerInput = dynamic_cast<ControllerInput*>(localPlayer->input.get());
	if (controllerInput == nullptr)
		return;
	
	// Get block properties to calculate rumble strength
	Level &level = *minecraft.level;
	int_t blockId = level.getTile(x, y, z);
	if (blockId == 0)
		return;
	
	Tile *tile = Tile::tiles[blockId];
	if (tile == nullptr)
		return;
	
	// Controlify formula: 0.02f + easeInQuad(min(1, destroyTime/20)) * 0.25f
	// Note: destroySpeed is actually the destroy time (hardness), not speed
	float destroyTime = tile->getDestroySpeed();  // This is the actual destroy time in seconds
	
	// Handle edge cases: unbreakable blocks (-1.0f) and instant-break blocks (0.0f)
	if (destroyTime < 0.0f)
		destroyTime = 20.0f;  // Treat unbreakable as max hardness
	else if (destroyTime < 0.01f)
		destroyTime = 0.01f;  // Minimum for instant-break blocks
	
	destroyTime = (std::min)(20.0f, destroyTime);  // Cap at 20 seconds (use parentheses to avoid Windows.h macro)
	
	// Controlify uses easeInQuad: t * t
	float t = destroyTime / 20.0f;
	float easeInQuad = t * t;
	
	// Calculate rumble strength: 0.02f + easeInQuad * 0.25f
	// Clamp to valid range (0.0-1.0) - though Gamepad.cpp already does this, it's good practice
	blockBreakRumbleStrength = (std::max)(0.0f, (std::min)(1.0f, 0.02f + easeInQuad * 0.25f));
	blockBreakRumbleTicks = 0;
	
	// Start rumble with constant strength (Controlify uses 5000ms duration, refreshed every tick)
	// 5000ms = 100 ticks at 20 tps
	controllerInput->playRumble(blockBreakRumbleStrength, 0.01f, 100);  // 100 ticks = 5000ms, matches Controlify
}

void SurvivalMode::updateBlockBreakRumble()
{
	// Only rumble for LocalPlayer with controller input
	LocalPlayer* localPlayer = dynamic_cast<LocalPlayer*>(minecraft.player.get());
	if (localPlayer == nullptr)
		return;
	
	ControllerInput* controllerInput = dynamic_cast<ControllerInput*>(localPlayer->input.get());
	if (controllerInput == nullptr)
		return;
	
	if (blockBreakRumbleStrength <= 0.0f || destroyProgress <= 0.0f)
		return;
	
	// Refresh rumble every tick with constant strength (Controlify uses 5000ms duration, refreshed every tick)
	// 5000ms = 100 ticks at 20 tps
	controllerInput->playRumble(blockBreakRumbleStrength, 0.01f, 100);  // 100 ticks = 5000ms, matches Controlify
	blockBreakRumbleTicks++;
}

void SurvivalMode::stopBlockBreakRumble()
{
	blockBreakRumbleStrength = 0.0f;
	blockBreakRumbleTicks = 0;
	
	// Stop rumble by sending zero intensity
	LocalPlayer* localPlayer = dynamic_cast<LocalPlayer*>(minecraft.player.get());
	if (localPlayer == nullptr)
		return;
	
	ControllerInput* controllerInput = dynamic_cast<ControllerInput*>(localPlayer->input.get());
	if (controllerInput == nullptr)
		return;
	
	// Stop rumble with zero intensity
	controllerInput->playRumble(0.0f, 0.0f, 1);
}
