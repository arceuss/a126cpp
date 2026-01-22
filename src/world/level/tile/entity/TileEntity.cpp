#include "world/level/tile/entity/TileEntity.h"

#include <iostream>
#include <unordered_map>

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"

#include "java/String.h"

#include "world/level/tile/entity/FurnaceTileEntity.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/tile/entity/MobSpawnerTileEntity.h"
#include "world/level/tile/entity/SignTileEntity.h"

// Beta: TileEntity static registration maps (TileEntity.java:11-12)
// Using factory functions instead of Java's class reflection
using TileEntityFactory = std::shared_ptr<TileEntity>(*)();
static std::unordered_map<jstring, TileEntityFactory> idClassMap;

// Beta: TileEntity.load() - loads x, y, z from NBT (TileEntity.java:27-31)
void TileEntity::load(CompoundTag &tag)
{
	x = tag.getInt(u"x");
	y = tag.getInt(u"y");
	z = tag.getInt(u"z");
}

// Beta: TileEntity.save() - saves id, x, y, z to NBT (TileEntity.java:33-43)
void TileEntity::save(CompoundTag &tag)
{
	jstring id = getEncodeId();
	if (id.empty())
		throw std::runtime_error("TileEntity is missing a mapping! This is a bug!");

	tag.putString(u"id", id);
	tag.putInt(u"x", x);
	tag.putInt(u"y", y);
	tag.putInt(u"z", z);
}

// Beta: TileEntity.tick() - empty default implementation (TileEntity.java:45-46)
void TileEntity::tick()
{

}

// Beta: TileEntity.loadStatic() - creates TileEntity from NBT using idClassMap (TileEntity.java:48-67)
std::shared_ptr<TileEntity> TileEntity::loadStatic(CompoundTag &tag)
{
	std::shared_ptr<TileEntity> tileEntity = nullptr;

	try
	{
		jstring id = tag.getString(u"id");
		auto it = idClassMap.find(id);
		if (it != idClassMap.end())
		{
			tileEntity = it->second();
		}
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}

	if (tileEntity != nullptr)
	{
		tileEntity->load(tag);
	}
	else
	{
		std::cout << "Skipping TileEntity with id " << String::toUTF8(tag.getString(u"id")) << std::endl;
	}

	return tileEntity;
}

// Beta: TileEntity.getData() - returns level.getData(x, y, z) (TileEntity.java:69-71)
int_t TileEntity::getData()
{
	return level->getData(x, y, z);
}

// Beta: TileEntity.setData() - calls level.setData(x, y, z, data) (TileEntity.java:73-75)
void TileEntity::setData(int_t data)
{
	level->setData(x, y, z, data);
}

// Beta: TileEntity.setChanged() - calls level.tileEntityChanged(x, y, z, this) (TileEntity.java:77-81)
void TileEntity::setChanged()
{
	if (level != nullptr)
	{
		// Beta: Get the shared_ptr from level instead of using shared_from_this()
		// This avoids bad_weak_ptr exceptions when the TileEntity isn't properly managed
		std::shared_ptr<TileEntity> te = level->getTileEntity(x, y, z);
		if (te != nullptr)
		{
			level->tileEntityChanged(x, y, z, te);
		}
	}
}

// Beta: TileEntity.distanceToSqr() - calculates squared distance (TileEntity.java:83-88)
double TileEntity::distanceToSqr(double x, double y, double z)
{
	double dx = this->x + 0.5 - x;
	double dy = this->y + 0.5 - y;
	double dz = this->z + 0.5 - z;
	return dx * dx + dy * dy + dz * dz;
}

// Beta: TileEntity.getTile() - returns Tile.tiles[level.getTile(x, y, z)] (TileEntity.java:90-92)
Tile &TileEntity::getTile()
{
	// Safety check: level must not be null - return air tile as fallback
	if (level == nullptr)
	{
		// Return air tile (ID 0) as safe fallback when level is null
		// This can happen during world unloading or invalid tile entities
		if (Tile::tiles[0] == nullptr)
		{
			// Even air tile is null - this is a critical initialization error
			// Create a temporary static fallback tile to avoid crashing
			static Tile* fallbackTile = nullptr;
			if (fallbackTile == nullptr)
			{
				// This should never happen in normal operation
				// If it does, it means tiles weren't initialized
				std::cerr << "CRITICAL: Tile::tiles[0] is null! Tile system not initialized!" << std::endl;
			}
			// Return air tile even if null (will crash, but better than undefined behavior)
			return *Tile::tiles[0];
		}
		return *Tile::tiles[0];
	}
	
	// Safety check: Validate tile ID is in valid range
	int_t tileId = level->getTile(x, y, z);
	if (tileId < 0 || tileId >= 256)
	{
		// Invalid tile ID - return air tile
		if (Tile::tiles[0] != nullptr)
		{
			return *Tile::tiles[0];
		}
	}
	
	Tile *tile = Tile::tiles[tileId];
	// Safety check: If tile is null (e.g., chunk unloaded or block removed), return air tile
	// This can happen if the tile entity exists but the block was removed/changed
	if (tile == nullptr)
	{
		// Return air tile (ID 0) as fallback
		// Note: This shouldn't normally happen, but prevents crashes during world unloading
		if (Tile::tiles[0] != nullptr)
		{
			return *Tile::tiles[0];
		}
	}
	
	// Final safety check before returning
	if (tile == nullptr)
	{
		// This should never happen, but defensive programming
		if (Tile::tiles[0] != nullptr)
		{
			return *Tile::tiles[0];
		}
	}
	return *tile;
}

// Static initialization: Register tile entity types (TileEntity.java:98-105)
// Note: Excluding DispenserTileEntity (Trap) for Alpha 1.2.6
struct TileEntityRegistrar
{
	TileEntityRegistrar()
	{
		// Beta: setId(FurnaceTileEntity.class, "Furnace") (TileEntity.java:99)
		idClassMap[u"Furnace"] = []() -> std::shared_ptr<TileEntity> {
			return Util::make_shared<FurnaceTileEntity>();
		};

		// Beta: setId(ChestTileEntity.class, "Chest") (TileEntity.java:100)
		idClassMap[u"Chest"] = []() -> std::shared_ptr<TileEntity> {
			return Util::make_shared<ChestTileEntity>();
		};

		// Beta: setId(MobSpawnerTileEntity.class, "MobSpawner") (TileEntity.java:103)
		idClassMap[u"MobSpawner"] = []() -> std::shared_ptr<TileEntity> {
			return Util::make_shared<MobSpawnerTileEntity>();
		};

		// Beta: setId(SignTileEntity.class, "Sign") (TileEntity.java:102)
		idClassMap[u"Sign"] = []() -> std::shared_ptr<TileEntity> {
			return Util::make_shared<SignTileEntity>();
		};

		// Note: MusicTileEntity (Note Block) excluded for Alpha 1.2.6 (Beta 1.2 only)
		// Note: DispenserTileEntity (Trap) excluded for Alpha 1.2.6 (Beta 1.2 only)
	}
};

static TileEntityRegistrar registrar;
