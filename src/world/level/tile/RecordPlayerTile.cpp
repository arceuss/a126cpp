#include "world/level/tile/RecordPlayerTile.h"
#include "world/level/Level.h"
#include "world/entity/player/Player.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"
#include "Facing.h"
#include "java/String.h"

// Beta: RecordPlayerTile.java
RecordPlayerTile::RecordPlayerTile(int_t id, int_t texture) : Tile(id, texture, Material::wood)
{
	// Beta: Properties set in initTiles()
}

int_t RecordPlayerTile::getTexture(Facing face)
{
	// Beta: RecordPlayerTile.getTexture() (RecordPlayerTile.java:16-18)
	// Face 1 (UP) = tex + 1, others = tex
	if (face == Facing::UP)  // Face 1
		return tex + 1;
	else
		return tex;
}

bool RecordPlayerTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: RecordPlayerTile.use() (RecordPlayerTile.java:21-29)
	// Alpha: BlockJukeBox.blockActivated() (BlockJukeBox.java:12-23)
	// Java: if(var1.multiplayerWorld) { return true; } - in multiplayer, don't eject on client
	if (level.isOnline)
	{
		// In multiplayer, just return true - server will handle ejecting the record
		// The server will send the item entity via packet, so we don't spawn it client-side
		return true;
	}
	
	int_t data = level.getData(x, y, z);
	if (data > 0)
	{
		dropRecording(level, x, y, z, data);
		return true;
	}
	else
	{
		return false;
	}
}

void RecordPlayerTile::dropRecording(Level &level, int_t x, int_t y, int_t z, int_t recordData)
{
	// Beta: RecordPlayerTile.dropRecording() (RecordPlayerTile.java:31-42)
	// Stop music
	level.playStreamingMusic(jstring(), x, y, z);  // Beta: var1.playStreamingMusic(null, var2, var3, var4)
	level.setData(x, y, z, 0);  // Beta: var1.setData(var2, var3, var4, 0)
	
	// Beta: Calculate record item ID: Item.record_01.id + var5 - 1 (RecordPlayerTile.java:34)
	// recordData is 1-based (1 = record_01, 2 = record_02, etc.)
	// newb12: int var6 = Item.record_01.id + var5 - 1;
	int_t recordItemId = 0;
	if (Items::record01 != nullptr)
	{
		recordItemId = Items::record01->getItemID() + recordData - 1;  // Use raw itemID, not shiftedIndex
	}
	
	if (recordItemId > 0)
	{
		// Beta: Spawn position randomization (RecordPlayerTile.java:35-38)
		float var7 = 0.7F;
		double var8 = (double)(level.random.nextFloat() * var7) + (double)(1.0F - var7) * 0.5;
		double var10 = (double)(level.random.nextFloat() * var7) + (double)(1.0F - var7) * 0.2 + 0.6;
		double var12 = (double)(level.random.nextFloat() * var7) + (double)(1.0F - var7) * 0.5;
		
		// Beta: Create ItemEntity and add to world (RecordPlayerTile.java:39-41)
		ItemStack dropStack(recordItemId, 1);
		double entityX = (double)x + var8;
		double entityY = (double)y + var10;
		double entityZ = (double)z + var12;
		auto itemEntity = std::make_shared<EntityItem>(level, entityX, entityY, entityZ, dropStack);
		itemEntity->throwTime = 10;  // Beta: var14.throwTime = 10
		level.addEntity(itemEntity);  // Beta: var1.addEntity(var14)
	}
}

void RecordPlayerTile::spawnResources(Level &level, int_t x, int_t y, int_t z, int_t data, float chance)
{
	// Beta: RecordPlayerTile.spawnResources() (RecordPlayerTile.java:45-53)
	if (!level.isOnline)
	{
		if (data > 0)
		{
			dropRecording(level, x, y, z, data);
		}
		
		Tile::spawnResources(level, x, y, z, data, chance);
	}
}
