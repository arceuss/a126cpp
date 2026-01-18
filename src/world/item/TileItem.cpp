#include "world/item/TileItem.h"
#include "world/item/ItemStack.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/entity/player/Player.h"
#include "Facing.h"
#include "util/Mth.h"

TileItem::TileItem(int_t baseTileId) : Item(baseTileId)
{
	this->tileId = baseTileId + 256;
	
	// Beta: Set icon from tile texture (TileItem.java:13)
	Tile *tile = Tile::tiles[tileId];
	if (tile != nullptr)
	{
		setIconIndex(tile->getTexture(Facing::NORTH));
	}
}

// Beta: TileItem.useOn() - places block (TileItem.java:20-72)
bool TileItem::useOn(ItemStack &stack, Player &player, Level &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Handle topSnow special case (TileItem.java:22-23)
	// If clicking on topSnow, always place below it (face = DOWN)
	if (level.getTile(x, y, z) == Tile::snow.id)  // Beta: Tile.topSnow.id (TileItem.java:22)
	{
		face = Facing::DOWN;  // Beta: var7 = 0 (TileItem.java:23)
	}
	
	// Beta: Adjust position based on face (TileItem.java:25-47)
	if (face == Facing::DOWN)
		y--;
	else if (face == Facing::UP)
		y++;
	else if (face == Facing::NORTH)
		z--;
	else if (face == Facing::SOUTH)
		z++;
	else if (face == Facing::WEST)
		x--;
	else if (face == Facing::EAST)
		x++;
	
	// Beta: Check if stack is empty (TileItem.java:50-51)
	if (stack.stackSize == 0)
		return false;
	
	// Beta: Check if we can place the block (TileItem.java:53)
	if (level.mayPlace(tileId, x, y, z, false))
	{
		Tile *tile = Tile::tiles[tileId];
		if (tile == nullptr)
			return true;
		
	// Beta: Place block with data (TileItem.java:55)
	int_t data = getLevelDataForAuxValue(stack.getAuxValue());
	if (level.setTileAndData(x, y, z, tileId, data))
	{
		// Beta: Call tile placement callbacks (TileItem.java:56-57)
		// Beta: Note: Java calls Tile.tiles[this.tileId].setPlacedOnFace and Tile.tiles[this.tileId].setPlacedBy
		// This ensures we're calling the method on the correct tile instance from the array
		Tile::tiles[tileId]->setPlacedOnFace(level, x, y, z, face);
		Tile::tiles[tileId]->setPlacedBy(level, x, y, z, player);  // Beta: Tile.setPlacedBy() called after placement
		
		// Alpha 1.2.6: Play placement sound only if NOT multiplayer (ItemBlock.java:49-51)
		// Java: if(!var3.multiplayerWorld) { var3.playSoundEffect(...); }
		if (!level.isOnline)
		{
			const Tile::SoundType *soundType = tile->getSoundType();
			if (soundType != nullptr)
			{
				jstring stepSound = soundType->getStepSound();
				float volume = (soundType->getVolume() + 1.0f) / 2.0f;
				float pitch = soundType->getPitch() * 0.8f;
				level.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, stepSound, volume, pitch);
			}
		}
			
			// Beta: Decrease stack size (TileItem.java:66)
			stack.stackSize--;
		}
	}
	
	return true;
}
