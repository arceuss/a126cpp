// Layer 4: exact Alpha World entity-update and world-reopen lifecycle.

#include <memory>
#include <unordered_set>

#include "java/File.h"
#include "nbt/CompoundTag.h"
#include "nbt/NbtIo.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "client/multiplayer/MultiPlayerLevel.h"
#include "world/entity/Entity.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/Items.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/Tile.h"

class CountingLifecycleEntity : public Entity
{
public:
	int_t tickCount = 0;
	bool crossChunkOnTick = false;

	explicit CountingLifecycleEntity(Level &level) : Entity(level)
	{
	}

	void tick() override
	{
		++tickCount;
		if (crossChunkOnTick)
			x = 16.5;
	}
};

HEADLESS_TEST(lifecycle, relative_nested_mkdirs_creates_missing_top_level)
{
	std::unique_ptr<File> root(File::open(u"headless-mkdirs-relative-root"));
	headless::removeRecursively(*root);
	std::unique_ptr<File> leaf(File::open(*root, u"nested/leaf"));

	ctx.check(leaf->mkdirs(), "mkdirs creates a missing relative directory tree");
	ctx.check(root->isDirectory(), "mkdirs creates the relative top-level component");
	std::unique_ptr<File> nested(File::open(*root, u"nested"));
	ctx.check(nested->isDirectory(), "mkdirs creates the intermediate component");
	ctx.check(leaf->isDirectory(), "mkdirs creates the leaf component");

	headless::removeRecursively(*root);
}

static void loadChunkSquare(Level &level, int_t minX, int_t maxX, int_t minZ, int_t maxZ)
{
	for (int_t x = minX; x <= maxX; ++x)
	{
		for (int_t z = minZ; z <= maxZ; ++z)
			level.getChunk(x, z);
	}
}

HEADLESS_TEST(lifecycle, ordinary_entities_tick_at_center_and_edge)
{
	headless::initGameRegistries();
	Level level(u"lifecycle-center-edge", Dimension::Id_Normal, 101LL);
	level.getChunk(0, 0);

	std::shared_ptr<CountingLifecycleEntity> center = std::make_shared<CountingLifecycleEntity>(level);
	center->setPos(0.5, 64.0, 0.5);
	level.addEntity(center);
	std::shared_ptr<CountingLifecycleEntity> edge = std::make_shared<CountingLifecycleEntity>(level);
	edge->setPos(15.999, 64.0, 15.999);
	level.addEntity(edge);

	level.tickEntities();
	ctx.checkEqual(center->tickCount, 1, "center entity ticks once");
	ctx.checkEqual(edge->tickCount, 1, "chunk-edge entity ticks once");
}

HEADLESS_TEST(lifecycle, optional_path_uses_16_block_loaded_region)
{
	headless::initGameRegistries();
	Level inside(u"lifecycle-special-inside", Dimension::Id_Normal, 102LL);
	// For an entity at block (0,0), Alpha checks every chunk intersecting
	// [-16,+16] in X/Z (World.java:1012-1015): chunk coordinates -1..1.
	loadChunkSquare(inside, -1, 1, -1, 1);
	std::shared_ptr<CountingLifecycleEntity> admitted = std::make_shared<CountingLifecycleEntity>(inside);
	admitted->setPos(0.5, 64.0, 0.5);
	admitted->xOld = -999.0;
	inside.tick(admitted, false);
	ctx.checkEqual(admitted->tickCount, 0, "non-forced Alpha path does not call Entity.onUpdate");
	ctx.checkEqualBits(admitted->xOld, admitted->x,
		"inside the 16-block gate refreshes interpolation state");

	Level outside(u"lifecycle-special-outside", Dimension::Id_Normal, 103LL);
	outside.getChunk(0, 0); // Required neighboring chunks deliberately absent.
	std::shared_ptr<CountingLifecycleEntity> rejected = std::make_shared<CountingLifecycleEntity>(outside);
	rejected->setPos(0.5, 64.0, 0.5);
	rejected->xOld = -999.0;
	outside.tick(rejected, false);
	ctx.checkEqual(rejected->tickCount, 0, "outside gate does not tick");
	ctx.checkEqualBits(rejected->xOld, -999.0,
		"outside the loaded-region gate returns before interpolation state changes");
}

HEADLESS_TEST(lifecycle, entity_crossing_chunks_has_one_membership)
{
	headless::initGameRegistries();
	Level level(u"lifecycle-crossing", Dimension::Id_Normal, 104LL);
	std::shared_ptr<LevelChunk> oldChunk = level.getChunk(0, 0);
	std::shared_ptr<LevelChunk> newChunk = level.getChunk(1, 0);

	std::shared_ptr<CountingLifecycleEntity> entity = std::make_shared<CountingLifecycleEntity>(level);
	entity->setPos(15.5, 64.0, 0.5);
	entity->crossChunkOnTick = true;
	level.addEntity(entity);
	ctx.checkEqual(oldChunk->countEntities(), 1, "old chunk membership before crossing");

	level.tickEntities();
	ctx.checkEqual(entity->tickCount, 1, "crossing entity tick count");
	ctx.checkEqual(oldChunk->countEntities(), 0, "old chunk membership after crossing");
	ctx.checkEqual(newChunk->countEntities(), 1, "new chunk membership after crossing");
	ctx.checkEqual(entity->xChunk, 1, "recorded x chunk after crossing");
}

HEADLESS_TEST(lifecycle, queued_removal_precedes_entity_ticking)
{
	headless::initGameRegistries();
	Level level(u"lifecycle-remove", Dimension::Id_Normal, 105LL);
	level.getChunk(0, 0);

	std::shared_ptr<CountingLifecycleEntity> entity = std::make_shared<CountingLifecycleEntity>(level);
	entity->setPos(0.5, 64.0, 0.5);
	level.addEntity(entity);
	level.entitiesToRemove.insert(entity);
	level.tickEntities();

	ctx.checkEqual(entity->tickCount, 0, "queued entity must not tick again");
	ctx.check(level.entities.find(entity) == level.entities.end(), "queued entity removed from active set");
	ctx.check(level.entitiesToRemove.empty(), "pending-removal set cleared");
}

HEADLESS_TEST(lifecycle, active_entity_ticks_once_per_level_tick)
{
	headless::initGameRegistries();
	Level level(u"lifecycle-once", Dimension::Id_Normal, 106LL);
	level.getChunk(0, 0);

	std::shared_ptr<CountingLifecycleEntity> entity = std::make_shared<CountingLifecycleEntity>(level);
	entity->setPos(0.5, 64.0, 0.5);
	level.addEntity(entity);
	// Re-adding the same shared object must not create a second active entry.
	level.addEntity(entity);
	level.tickEntities();
	ctx.checkEqual(entity->tickCount, 1, "one object must be ticked exactly once");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "one active set entry");
}

HEADLESS_TEST(lifecycle, reopening_world_preserves_seed_and_spawn)
{
	headless::initGameRegistries();
	headless::TempDir directory(ctx, "lifecycle-reopen-seed-spawn");
	std::unique_ptr<File> world(File::open(directory.file(), u"SavedWorld"));
	world->mkdirs();
	std::unique_ptr<File> levelFile(File::open(*world, u"level.dat"));

	CompoundTag root;
	std::unique_ptr<CompoundTag> data = std::make_unique<CompoundTag>();
	data->putLong(u"RandomSeed", 0x123456789ABCDELL);
	data->putInt(u"SpawnX", 321);
	data->putInt(u"SpawnY", 72);
	data->putInt(u"SpawnZ", -654);
	data->putLong(u"Time", 9001LL);
	data->putLong(u"SizeOnDisk", 42LL);
	root.putCompound(u"Data", std::move(data));
	std::unique_ptr<std::ostream> output(levelFile->toStreamOut());
	NbtIo::writeCompressed(root, *output);
	output.reset();

	Level reopened(directory.newHandle(), u"SavedWorld", 999LL);
	ctx.checkEqual(reopened.seed, 0x123456789ABCDELL, "stored world seed");
	ctx.checkEqual(reopened.xSpawn, 321, "stored SpawnX");
	ctx.checkEqual(reopened.ySpawn, 72, "stored SpawnY");
	ctx.checkEqual(reopened.zSpawn, -654, "stored SpawnZ");
	ctx.checkEqual(reopened.time, 9001LL, "stored world time");
	ctx.check(!reopened.isNew, "existing world must not be marked new");
}

HEADLESS_TEST(lifecycle, world_ticks_saves_and_reloads_production_state)
{
	headless::initGameRegistries();
	headless::TempDir directory(ctx, "lifecycle-save-reload");
	std::unique_ptr<File> worldDirectory(File::open(directory.file(), u"RoundTripWorld"));
	worldDirectory->mkdirs();
	std::unique_ptr<File> levelFile(File::open(*worldDirectory, u"level.dat"));

	// Seed level.dat once so construction skips Alpha's random spawn search.
	// Everything after this point uses the production Level/chunk storage path.
	CompoundTag root;
	std::unique_ptr<CompoundTag> data = std::make_unique<CompoundTag>();
	data->putLong(u"RandomSeed", 123456789LL);
	data->putInt(u"SpawnX", 0);
	data->putInt(u"SpawnY", 64);
	data->putInt(u"SpawnZ", 0);
	data->putLong(u"Time", 0);
	data->putLong(u"SizeOnDisk", 0);
	root.putCompound(u"Data", std::move(data));
	std::unique_ptr<std::ostream> output(levelFile->toStreamOut());
	NbtIo::writeCompressed(root, *output);
	output.reset();

	{
		Level created(directory.newHandle(), u"RoundTripWorld", 1LL);
		ctx.check(created.setTile(1, 100, 1, Tile::rock.id), "place block in generated chunk");
		std::shared_ptr<Pig> pig = std::make_shared<Pig>(created);
		pig->setPos(1.5, 101.0, 1.5);
		created.addEntity(pig);
		std::shared_ptr<EntityItem> drop = std::make_shared<EntityItem>(
			created, 2.5, 101.0, 2.5,
			ItemStack(Items::coal->getShiftedIndex(), 5, 2));
		created.addEntity(drop);
		created.tickEntities();
		drop->age = 77;
		created.time = 1234LL;
		created.save(true, nullptr);
	}

	{
		Level reopened(directory.newHandle(), u"RoundTripWorld", 2LL);
		ctx.checkEqual(reopened.time, 1234LL, "world time after production save/reload");
		ctx.checkEqual(reopened.getTile(1, 100, 1), Tile::rock.id,
			"placed block after production save/reload");
		ctx.checkEqual(static_cast<long long>(reopened.entities.size()), 2,
			"saved entity count after chunk activation");
		bool foundPig = false;
		bool foundDrop = false;
		for (const std::shared_ptr<Entity> &entity : reopened.entities)
		{
			if (entity->getEncodeId() == u"Pig")
				foundPig = true;
			std::shared_ptr<EntityItem> drop = std::dynamic_pointer_cast<EntityItem>(entity);
			if (drop != nullptr && drop->hasValidItem()
				&& drop->item.itemID == Items::coal->getShiftedIndex()
				&& drop->item.stackSize == 5 && drop->item.itemDamage == 2
				&& drop->age == 77)
				foundDrop = true;
		}
		ctx.check(foundPig, "saved Pig subtype after production save/reload");
		ctx.check(foundDrop, "saved dropped-item stack after production save/reload");
	}
}

HEADLESS_TEST(lifecycle, multiplayer_dimension_view_retires_without_disconnect)
{
	headless::initGameRegistries();
	MultiPlayerLevel level(nullptr, 0x12345678LL, Dimension::Id_Hell);

	ctx.check(level.isValidClientView(), "a new multiplayer level is active");
	ctx.check(level.isOnline, "the replacement level remains a multiplayer level");
	ctx.checkEqual(level.dimension->id, Dimension::Id_Hell,
		"the replacement level uses the server-selected dimension");

	// Packet9 replacement must only retire the old client view. Network
	// disconnect is owned by disconnect(), not markInvalid().
	level.markInvalid();
	ctx.check(!level.isValidClientView(), "the old multiplayer view is retired");
}
