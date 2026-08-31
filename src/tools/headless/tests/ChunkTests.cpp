// Layer 4: chunk membership lifecycle.
//
// Alpha reference:
//   Chunk.java:430-436   onChunkLoad  registers tile entities and every entity slice
//   Chunk.java:438-444   onChunkUnload performs the inverse
//   Chunk.java:339-376   addEntity / func_1015_b / func_1016_a slice bookkeeping
//
// The invariant under test is that activation and deactivation are exact
// inverses and that repeating them never accumulates duplicates.

#include <memory>
#include <string>
#include <vector>

#include "client/multiplayer/MultiPlayerChunkCache.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"

#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/animal/Pig.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/chunk/storage/OldChunkStorage.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/entity/SignTileEntity.h"

// Counts how often one entity appears in the level's active set. The set type
// makes more than one impossible, but the count still distinguishes "present"
// from "silently dropped".
static int activeEntityCount(Level &level, const std::shared_ptr<Entity> &entity)
{
	return static_cast<int>(level.entities.count(entity));
}

static int activeTileEntityCount(Level &level, const std::shared_ptr<TileEntity> &tileEntity)
{
	return static_cast<int>(level.tileEntityList.count(tileEntity));
}

static CompoundTag makeStoredChunk(Level &level, int entityCount)
{
	CompoundTag chunkTag;
	chunkTag.putInt(u"xPos", 0);
	chunkTag.putInt(u"zPos", 0);
	chunkTag.putLong(u"LastUpdate", 0);
	chunkTag.putByteArray(u"Blocks", std::vector<byte_t>(16 * 128 * 16, 0));
	chunkTag.putByteArray(u"Data", std::vector<byte_t>(16 * 128 * 16 / 2, 0));
	chunkTag.putByteArray(u"SkyLight", std::vector<byte_t>(16 * 128 * 16 / 2, 0));
	chunkTag.putByteArray(u"BlockLight", std::vector<byte_t>(16 * 128 * 16 / 2, 0));
	chunkTag.putByteArray(u"HeightMap", std::vector<byte_t>(16 * 16, 0));
	chunkTag.putBoolean(u"TerrainPopulated", true);

	std::shared_ptr<ListTag> entityTags = std::make_shared<ListTag>();
	for (int_t i = 0; i < entityCount; ++i)
	{
		std::shared_ptr<Pig> pig = std::make_shared<Pig>(level);
		pig->setPos(0.5 + static_cast<double>(i % 15),
			1.0 + static_cast<double>(i % 126),
			0.5 + static_cast<double>((i / 15) % 15));

		std::shared_ptr<CompoundTag> entityTag = std::make_shared<CompoundTag>();
		if (pig->save(*entityTag))
			entityTags->add(entityTag);
	}
	chunkTag.put(u"Entities", entityTags);
	chunkTag.put(u"TileEntities", std::make_shared<ListTag>());
	return chunkTag;
}

HEADLESS_TEST(chunks, activation_registers_entities_and_tile_entities)
{
	headless::initGameRegistries();

	Level level(u"chunk-activation", Dimension::Id_Normal, 4242LL);
	std::shared_ptr<LevelChunk> chunk = std::make_shared<LevelChunk>(level, 0, 0);

	std::shared_ptr<Pig> pig = std::make_shared<Pig>(level);
	pig->setPos(8.5, 64.0, 8.5);
	chunk->addEntity(pig);

	// A sign needs an entity tile at the target position before the chunk will
	// accept it, exactly as in Alpha.
	chunk->setTile(4, 64, 4, Tile::sign.id);
	std::shared_ptr<SignTileEntity> sign = std::make_shared<SignTileEntity>();
	sign->x = 4;
	sign->y = 64;
	sign->z = 4;
	chunk->setTileEntity(4, 64, 4, sign);

	ctx.checkEqual(activeEntityCount(level, pig), 0,
		"an inactive chunk must not publish its entities");
	ctx.checkEqual(activeTileEntityCount(level, sign), 0,
		"an inactive chunk must not publish its tile entities");

	chunk->load();
	ctx.check(chunk->loaded, "load() must mark the chunk active");
	ctx.checkEqual(activeEntityCount(level, pig), 1, "entity count after activation");
	ctx.checkEqual(activeTileEntityCount(level, sign), 1, "tile entity count after activation");

	chunk->unload();
	ctx.check(!chunk->loaded, "unload() must mark the chunk inactive");
	ctx.checkEqual(activeTileEntityCount(level, sign), 0, "tile entity count after deactivation");
	ctx.checkEqual(static_cast<long long>(level.entitiesToRemove.count(pig)), 1,
		"deactivation must queue the entity for removal");
}

HEADLESS_TEST(chunks, repeated_activation_does_not_duplicate)
{
	headless::initGameRegistries();

	Level level(u"chunk-reactivation", Dimension::Id_Normal, 99LL);
	std::shared_ptr<LevelChunk> chunk = std::make_shared<LevelChunk>(level, 0, 0);

	chunk->setTile(1, 70, 1, Tile::sign.id);
	std::shared_ptr<SignTileEntity> sign = std::make_shared<SignTileEntity>();
	sign->x = 1;
	sign->y = 70;
	sign->z = 1;
	chunk->setTileEntity(1, 70, 1, sign);

	for (int cycle = 0; cycle < 20; ++cycle)
	{
		chunk->load();
		if (!ctx.checkEqual(activeTileEntityCount(level, sign), 1,
			"tile entity count after activation cycle " + std::to_string(cycle)))
			return;
		ctx.checkEqual(static_cast<long long>(level.tileEntityList.size()), 1,
			"total active tile entities after activation cycle " + std::to_string(cycle));

		chunk->unload();
		if (!ctx.checkEqual(activeTileEntityCount(level, sign), 0,
			"tile entity count after deactivation cycle " + std::to_string(cycle)))
			return;
	}
}

HEADLESS_TEST(chunks, entity_removal_uses_recorded_slice)
{
	headless::initGameRegistries();

	Level level(u"chunk-slice", Dimension::Id_Normal, 7LL);
	std::shared_ptr<LevelChunk> chunk = std::make_shared<LevelChunk>(level, 0, 0);

	std::shared_ptr<Pig> pig = std::make_shared<Pig>(level);
	pig->setPos(8.5, 20.0, 8.5); // slice 1
	chunk->addEntity(pig);
	ctx.checkEqual(pig->yChunk, 1, "entity should be filed in slice 1");
	ctx.checkEqual(chunk->countEntities(), 1, "one entity after add");

	// Moving the entity without re-filing it must not make removal miss.
	pig->setPos(8.5, 100.0, 8.5); // would compute slice 6
	chunk->removeEntity(pig);
	ctx.checkEqual(chunk->countEntities(), 0,
		"removal must use the slice the entity was filed under");
}

HEADLESS_TEST(chunks, tile_entity_replacement_keeps_one_registration)
{
	headless::initGameRegistries();

	Level level(u"chunk-te-replace", Dimension::Id_Normal, 11LL);
	std::shared_ptr<LevelChunk> chunk = std::make_shared<LevelChunk>(level, 0, 0);
	chunk->setTile(2, 65, 2, Tile::sign.id);
	chunk->load();

	std::shared_ptr<SignTileEntity> first = std::make_shared<SignTileEntity>();
	first->x = 2;
	first->y = 65;
	first->z = 2;
	chunk->setTileEntity(2, 65, 2, first);

	std::shared_ptr<SignTileEntity> second = std::make_shared<SignTileEntity>();
	second->x = 2;
	second->y = 65;
	second->z = 2;
	chunk->setTileEntity(2, 65, 2, second);

	// Alpha removes the tile entity it is replacing from the level list
	// (Chunk.java:414-420).
	ctx.checkEqual(activeTileEntityCount(level, first), 0,
		"the replaced tile entity must be unregistered");
	ctx.checkEqual(activeTileEntityCount(level, second), 1,
		"the replacement tile entity must be registered");
	ctx.checkEqual(static_cast<long long>(level.tileEntityList.size()), 1,
		"exactly one active tile entity after replacement");

	chunk->removeTileEntity(2, 65, 2);
	ctx.checkEqual(static_cast<long long>(level.tileEntityList.size()), 0,
		"removal must unregister the tile entity");
}

HEADLESS_TEST(chunks, multiplayer_cache_reuses_and_releases_chunk_ownership)
{
	headless::initGameRegistries();
	Level level(u"multiplayer-chunk-cache", Dimension::Id_Normal, 1776LL);
	MultiPlayerChunkCache cache(level);

	std::shared_ptr<LevelChunk> first = cache.create(4, -7);
	std::shared_ptr<LevelChunk> duplicate = cache.create(4, -7);
	ctx.check(first == duplicate,
		"repeated visibility for one coordinate must reuse its loaded chunk");
	ctx.checkEqual(static_cast<long long>(cache.loadedChunkCount()), 1,
		"duplicate visibility must retain one chunk");

	for (int cycle = 0; cycle < 8; cycle++)
	{
		for (int_t x = -16; x < 16; x++)
			cache.create(x, cycle);
		ctx.checkEqual(static_cast<long long>(cache.loadedChunkCount()), 33,
			"travel strip resident count before unload");
		for (int_t x = -16; x < 16; x++)
			cache.drop(x, cycle);
		ctx.checkEqual(static_cast<long long>(cache.loadedChunkCount()), 1,
			"travel strip resident count after unload");
	}

	// Beta resolves an unloaded coordinate to the shared blank chunk
	// (MultiPlayerChunkCache.java:52-57). Querying must never register a
	// chunk: the client asks about coordinates outside the loaded set
	// constantly, and retaining one chunk per query is what made a server
	// session grow without bound.
	for (int_t x = 0; x < 512; x++)
	{
		std::shared_ptr<LevelChunk> queried = cache.getChunk(x, 4096);
		if (!ctx.check(queried != nullptr && queried->isEmpty(),
			"an unloaded coordinate must resolve to the blank chunk"))
			return;
		if (!ctx.check(!cache.hasChunk(x, 4096),
			"querying an unloaded coordinate must not register it"))
			return;
	}
	ctx.checkEqual(static_cast<long long>(cache.loadedChunkCount()), 1,
		"512 queries of unloaded coordinates must retain nothing");

	cache.drop(4, -7);
	ctx.checkEqual(static_cast<long long>(cache.loadedChunkCount()), 0,
		"all multiplayer chunks released");
	cache.drop(999, 999);
	ctx.checkEqual(static_cast<long long>(cache.loadedChunkCount()), 0,
		"unloading an absent coordinate must not allocate a chunk");
}

static void checkStoredEntityCount(headless::TestContext &ctx, int count)
{
	Level level(u"chunk-loaded-entities", Dimension::Id_Normal, 2468LL);
	CompoundTag tag = makeStoredChunk(level, count);

	std::shared_ptr<LevelChunk> chunk = OldChunkStorage::load(level, tag);
	if (!ctx.check(chunk != nullptr, "stored chunk must decode"))
		return;

	ctx.checkEqual(chunk->countEntities(), count,
		"decoded entity count for list size " + std::to_string(count));
	ctx.check(chunk->lastSaveHadEntities,
		"a chunk with an Entities tag must retain the has-entities flag");

	chunk->load();
	ctx.checkEqual(static_cast<long long>(level.entities.size()), count,
		"active entity count after loading list size " + std::to_string(count));
	chunk->unload();
	ctx.checkEqual(static_cast<long long>(level.entitiesToRemove.size()), count,
		"queued removal count after unloading list size " + std::to_string(count));
}

HEADLESS_TEST(chunks, storage_retains_one_entity)
{
	headless::initGameRegistries();
	checkStoredEntityCount(ctx, 1);
}

HEADLESS_TEST(chunks, storage_retains_many_entities)
{
	headless::initGameRegistries();
	checkStoredEntityCount(ctx, 32);
}

HEADLESS_TEST(chunks, storage_retains_more_than_127_entities)
{
	headless::initGameRegistries();
	// The pre-fix byte_t loop counter wrapped at 128 and never finished.
	checkStoredEntityCount(ctx, 130);
}
