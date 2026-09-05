#include <stdexcept>
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "nbt/CompoundTag.h"
#include "nbt/NbtIo.h"
#include "util/ProgressListener.h"
#include "world/level/SaveConverterMcRegion.h"
#include "world/level/chunk/storage/RegionFileCache.h"
#include "world/level/Level.h"
#include "world/level/chunk/ChunkCache.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/dimension/Dimension.h"

class FailingChunkStorage : public ChunkStorage
{
public:
	std::shared_ptr<LevelChunk> load(Level &level, int_t x, int_t z) override
	{
		std::shared_ptr<LevelChunk> chunk = std::make_shared<LevelChunk>(level, x, z);
		chunk->terrainPopulated = true;
		return chunk;
	}
	void save(Level &, LevelChunk &) override { throw std::runtime_error("injected save failure"); }
	void saveEntities(Level &, LevelChunk &) override {}
	void tick() override {}
	void flush() override {}
};

HEADLESS_TEST(storage, failed_save_preserves_dirty_chunk_and_saved_time)
{
	Level level(u"save-failure", Dimension::Id_Normal, 5LL);
	ChunkCache cache(level, new FailingChunkStorage(), nullptr);
	std::shared_ptr<LevelChunk> chunk = cache.getChunk(0, 0);
	chunk->unsaved = true;
	const long_t savedTime = chunk->lastSaveTime;
	level.time = savedTime + 20;
	bool failed = false;
	try { cache.save(true, nullptr); }
	catch (const std::exception &) { failed = true; }
	ctx.check(failed, "owner observes failed save");
	ctx.check(chunk->unsaved, "failed save cannot mark chunk clean");
	ctx.checkEqual(chunk->lastSaveTime, savedTime, "failed save cannot advance saved time");
}

HEADLESS_TEST(storage, failed_eviction_keeps_old_chunk_loaded)
{
	Level level(u"eviction-failure", Dimension::Id_Normal, 6LL);
	ChunkCache cache(level, new FailingChunkStorage(), nullptr);
	std::shared_ptr<LevelChunk> chunk = cache.getChunk(0, 0);
	cache.centerOn(32, 0);
	bool failed = false;
	try { cache.getChunk(32, 0); }
	catch (const std::exception &) { failed = true; }
	ctx.check(failed, "eviction reports save failure");
	ctx.check(chunk->loaded && chunk->unsaved, "unsaved resident chunk is not unloaded after failure");
	cache.centerOn(0, 0);
	ctx.check(cache.getChunk(0, 0) == chunk, "failed eviction retains original cache ownership");
}

class SaveTestProgress : public ProgressListener
{
public:
	void progressStartNoAbort(const jstring &) override {}
	void progressStart(const jstring &) override {}
	void progressStage(const jstring &) override {}
	void progressStagePercentage(int_t) override {}
};

HEADLESS_TEST(storage, failed_conversion_preserves_original_chunk_files)
{
	headless::TempDir directory(ctx, "failed-conversion");
	std::unique_ptr<File> world(File::open(directory.file(), u"ConvertWorld"));
	world->mkdirs();
	std::unique_ptr<File> levelFile(File::open(*world, u"level.dat"));
	CompoundTag root;
	std::shared_ptr<CompoundTag> data = std::make_shared<CompoundTag>();
	data->putLong(u"RandomSeed", 123LL);
	root.put(u"Data", data);
	{
		std::unique_ptr<std::ostream> output(levelFile->toStreamOut());
		NbtIo::writeCompressed(root, *output);
	}
	std::unique_ptr<File> oldFolder(File::open(*world, u"0/0"));
	oldFolder->mkdirs();
	std::unique_ptr<File> oldChunk(File::open(*oldFolder, u"c.0.0.dat"));
	{
		std::unique_ptr<std::ostream> output(oldChunk->toStreamOut());
		*output << "incomplete gzip";
	}
	SaveConverterMcRegion converter(directory.file());
	SaveTestProgress progress;
	bool failed = false;
	try { failed = !converter.convertMapFormat(u"ConvertWorld", progress); }
	catch (const std::exception &) { failed = true; }
	RegionFileCache::clearCache();
	ctx.check(failed, "converter must report a failed chunk");
	std::unique_ptr<std::istream> original(oldChunk->toStreamIn());
	const std::string contents = original ? std::string(std::istreambuf_iterator<char>(*original),
		std::istreambuf_iterator<char>()) : std::string();
	ctx.checkEqual(contents, "incomplete gzip", "failed conversion preserves original chunk bytes");
	ctx.check(converter.isOldMapFormat(u"ConvertWorld"), "failed conversion cannot stamp the new format");
}

HEADLESS_TEST(storage, failed_level_data_open_preserves_previous_save)
{
	headless::TempDir directory(ctx, "failed-level-data");
	// Obtain a normal file-backed world's session lock and initial level.dat.
	{
		Level saved(directory.newHandle(), u"Saved", 123LL);
		saved.saveLevelData();
		std::unique_ptr<File> blocked(File::open(*saved.dir, u"level.dat_new"));
		blocked->mkdir();
		std::unique_ptr<File> previous(File::open(*saved.dir, u"level.dat"));
		std::unique_ptr<std::istream> input(previous->toStreamIn());
		const std::string before((std::istreambuf_iterator<char>(*input)), std::istreambuf_iterator<char>());
		input.reset();
		bool failed = false;
		try { saved.saveLevelData(); }
		catch (const std::exception &) { failed = true; }
		ctx.check(failed, "unwritable temporary level data must report failure");
		input.reset(previous->toStreamIn());
		const std::string after((std::istreambuf_iterator<char>(*input)), std::istreambuf_iterator<char>());
		ctx.check(after == before, "previous level.dat remains intact");
	}
	RegionFileCache::clearCache();
}
