#include "client/gamemode/GameMode.h"

#include "client/Minecraft.h"
#include "world/level/Level.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "Facing.h"

GameMode::GameMode(Minecraft &minecraft) : minecraft(minecraft)
{

}

void GameMode::initLevel(std::shared_ptr<Level> level)
{

}

void GameMode::startDestroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	destroyBlock(x, y, z, face);
}

bool GameMode::destroyBlock(int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Spawn block break particles (GameMode.java:27)
	minecraft.particleEngine.destroy(x, y, z);

	Level &level = *minecraft.level;
	Tile *oldTile = Tile::tiles[level.getTile(x, y, z)];
	int_t data = level.getData(x, y, z);
	bool changed = level.setTile(x, y, z, 0);
	if (oldTile != nullptr && changed)
	{
		// Beta: Play break sound (GameMode.java:33-42)
		const Tile::SoundType *soundType = oldTile->getSoundType();
		if (soundType != nullptr)
		{
			jstring breakSound = soundType->getBreakSound();
			float volume = (soundType->getVolume() + 1.0f) / 2.0f;
			float pitch = soundType->getPitch() * 0.8f;
			minecraft.soundEngine.play(breakSound, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, volume, pitch);
		}
		oldTile->destroy(level, x, y, z, data);
	}
	return changed;  // BUG FIX: Was returning false, should return changed
}

void GameMode::continueDestroyBlock(int_t x, int_t y, int_t z, Facing face)
{

}

void GameMode::stopDestroyBlock()
{

}

void GameMode::render(float a)
{

}

float GameMode::getPickRange()
{
	return 5.0f;
}

void GameMode::initPlayer(std::shared_ptr<Player> player)
{

}

void GameMode::tick()
{

}

bool GameMode::canHurtPlayer()
{
	return true;
}

void GameMode::adjustPlayer(std::shared_ptr<Player> player)
{

}

// Beta: GameMode.useItem() - uses item (GameMode.java:62-75)
bool GameMode::useItem(std::shared_ptr<Player> &player, std::shared_ptr<Level> &level, ItemStack *item)
{
	if (item == nullptr || item->isEmpty())
		return false;
	
	// Beta: int oldCount = item.count (GameMode.java:63)
	int_t oldCount = item->stackSize;
	
	// Beta: ItemInstance itemInstance = item.use(level, player) (GameMode.java:64)
	ItemStack itemInstance = item->use(*level, *player);
	
	// Beta: Check if item changed (GameMode.java:65-71)
	// Beta: if (itemInstance != item || itemInstance != null && itemInstance.count != oldCount)
	// Since we return by value, check if count changed or item is different
	if (itemInstance.stackSize != oldCount || itemInstance.itemID != item->itemID)
	{
		// Beta: player.inventory.items[player.inventory.selected] = itemInstance (GameMode.java:66)
		ItemStack *selected = player->inventory.getSelected();
		if (selected != nullptr)
		{
			*selected = itemInstance;
			// Beta: if (itemInstance.count == 0) player.inventory.items[player.inventory.selected] = null (GameMode.java:67-68)
			if (itemInstance.isEmpty())
			{
				*selected = ItemStack();  // Clear stack
			}
		}
		return true;
	}
	
	return false;
}

// Beta: GameMode.useItemOn() - uses item on block (GameMode.java:90-97)
bool GameMode::useItemOn(std::shared_ptr<Player> &player, std::shared_ptr<Level> &level, ItemStack *item, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: int t = level.getTile(x, y, z) (GameMode.java:91)
	int_t t = level->getTile(x, y, z);
	
	// Beta: if (t > 0 && Tile.tiles[t].use(level, x, y, z, player)) (GameMode.java:92-93)
	if (t > 0 && Tile::tiles[t] != nullptr)
	{
		// Beta: Call Tile.use() to handle block interactions (doors, buttons, etc.)
		if (Tile::tiles[t]->use(*level, x, y, z, *player))
			return true;  // Beta: return true if tile handled the interaction (GameMode.java:93)
	}
	
	// Beta: return item == null ? false : item.useOn(player, level, x, y, z, face) (GameMode.java:95)
	if (item == nullptr || item->isEmpty())
		return false;
	
	return item->useOn(*player, *level, x, y, z, face);
}

std::shared_ptr<Player> GameMode::createPlayer(Level &level)
{
	return Util::make_shared<LocalPlayer>(minecraft, level, minecraft.user.get(), level.dimension->id);
}

void GameMode::interact(std::shared_ptr<Player> &player, std::shared_ptr<Entity> &entity)
{
	player->interact(entity);
}

void GameMode::attack(std::shared_ptr<Player> &player, std::shared_ptr<Entity> &entity)
{
	player->attack(entity);
}
