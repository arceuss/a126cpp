#include "world/level/chunk/ChunkCache.h"

#include "world/level/Level.h"

#include "world/level/chunk/EmptyLevelChunk.h"
#include "world/level/levelgen/ChunkGenWorker.h"
#include "world/level/levelgen/RandomLevelSource.h"

ChunkCache::ChunkCache(Level &level, ChunkStorage *storage, ChunkSource *source) : level(level)
{
	emptyChunk = Util::make_shared<EmptyLevelChunk>(level, 0, 0);

	this->source = std::unique_ptr<ChunkSource>(source);
	this->storage = std::unique_ptr<ChunkStorage>(storage);

	// Start async chunk generation worker if we have a RandomLevelSource
	RandomLevelSource *rls = dynamic_cast<RandomLevelSource *>(source);
	if (rls)
	{
		genWorker = std::unique_ptr<ChunkGenWorker>(new ChunkGenWorker(*rls, level));
		genWorker->start();
	}
}

ChunkCache::~ChunkCache()
{
	if (genWorker)
	{
		genWorker->stop();
		genWorker.reset();
	}
}

void ChunkCache::centerOn(int_t x, int_t y)
{
	xCenter = x;
	yCenter = y;

	// Prefetch chunks around the new center
	if (genWorker)
		prefetchChunks(x, y, 3);
}

bool ChunkCache::fits(int_t x, int_t y)
{
	byte_t b = CHUNK_CACHE_WIDTH / 2 - 1;
	return (x >= (xCenter - b)) && (y >= (yCenter - b)) && (x <= (xCenter + b)) && (y <= (yCenter + b));
}

bool ChunkCache::hasChunk(int_t x, int_t z)
{
	if (!fits(x, z))
		return false;
	if (x == xLast && z == zLast && last != nullptr)
		return true;

	int_t rx = x & (CHUNK_CACHE_WIDTH - 1);
	int_t rz = z & (CHUNK_CACHE_WIDTH - 1);
	int_t ri = rx + rz * CHUNK_CACHE_WIDTH;

	return chunks[ri] != nullptr && (chunks[ri] == emptyChunk || chunks[ri]->isAt(x, z));
}

std::shared_ptr<LevelChunk> ChunkCache::getChunk(int_t x, int_t z)
{
	if (x == xLast && z == zLast && last != nullptr)
		return last;
	if (!level.isFindingSpawn && !fits(x, z))
		return emptyChunk;

	int_t rx = x & (CHUNK_CACHE_WIDTH - 1);
	int_t rz = z & (CHUNK_CACHE_WIDTH - 1);
	int_t ri = rx + rz * CHUNK_CACHE_WIDTH;

	if (!hasChunk(x, z))
	{
		if (chunks[ri] != nullptr)
		{
			chunks[ri]->unload();
			save(*chunks[ri]);
			saveEntities(*chunks[ri]);
		}

		std::shared_ptr<LevelChunk> chunk = load(x, z);
		if (chunk == nullptr)
		{
			// Check if the async worker already generated this chunk
			if (genWorker)
				chunk = genWorker->pollCompleted(x, z);

			if (chunk != nullptr)
			{
				// Worker generated terrain but didn't run recalcHeightmap (needs Level access)
				// 4J: "removed & moved into its own method from getChunk, so we can call
				// recalcHeightmap after the chunk is added into the cache"
				chunk->recalcHeightmap();
			}
			else
			{
				// Fallback: synchronous generation
				if (source == nullptr)
					chunk = emptyChunk;
				else
					chunk = source->getChunk(x, z);
			}
		}

		chunks[ri] = chunk;
		chunk->lightLava();

		if (chunks[ri] != nullptr)
			chunks[ri]->load();

		if (!chunks[ri]->terrainPopulated && hasChunk(x + 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x + 1, z))
			postProcess(*this, x, z);
		if (hasChunk(x - 1, z) && !(getChunk(x - 1, z))->terrainPopulated && hasChunk(x - 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x - 1, z))
			postProcess(*this, x - 1, z);
		if (hasChunk(x, z - 1) && !(getChunk(x, z - 1))->terrainPopulated && hasChunk(x + 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x + 1, z))
			postProcess(*this, x, z - 1);
		if (hasChunk(x - 1, z - 1) && !(getChunk(x - 1, z - 1))->terrainPopulated && hasChunk(x - 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x - 1, z))
			postProcess(*this, x - 1, z - 1);

		// Prefetch nearby chunks for the player's direction of travel
		if (genWorker)
			prefetchChunks(x, z, 2);
	}

	xLast = x;
	zLast = z;
	last = chunks[ri];
	return chunks[ri];
}

std::shared_ptr<LevelChunk> ChunkCache::load(int_t x, int_t z)
{
	if (storage == nullptr) return emptyChunk;

	std::shared_ptr<LevelChunk> chunk = storage->load(level, x, z);
	if (chunk != nullptr)
		chunk->lastSaveTime = level.time;
	return chunk;
}

void ChunkCache::saveEntities(LevelChunk &chunk)
{
	if (storage == nullptr) return;
	storage->saveEntities(level, chunk);
}

void ChunkCache::save(LevelChunk &chunk)
{
	if (storage == nullptr) return;
	chunk.lastSaveTime = level.time;
	storage->save(level, chunk);
}

void ChunkCache::postProcess(ChunkSource &parent, int_t x, int_t z)
{
	std::shared_ptr<LevelChunk> chunk = getChunk(x, z);
	if (!chunk->terrainPopulated)
	{
		chunk->terrainPopulated = true;
		if (source != nullptr)
		{
			source->postProcess(parent, x, z);
			chunk->markUnsaved();
		}
	}
}

bool ChunkCache::save(bool force, std::shared_ptr<ProgressListener> progressListener)
{
	int_t throttle = 0;

	int_t chunksToSave = 0;
	if (progressListener != nullptr)
	{
		for (auto &c : chunks)
		{
			if (c != nullptr && c->shouldSave(force))
				chunksToSave++;
		}
	}

	int_t i = 0;

	for (auto &c : chunks)
	{
		if (c != nullptr)
		{
			if (force && !c->dontSave)
				saveEntities(*c);
			if (c->shouldSave(force))
			{
				save(*c);
				c->unsaved = false;

				if (++throttle == 2 && !force)
					return false;

				if (progressListener != nullptr && (++i % 10) == 0)
					progressListener->progressStagePercentage(i * 100 / chunksToSave);
			}
		}
	}

	if (force)
	{
		if (storage == nullptr)
			return true;
		storage->flush();
	}

	return true;
}

bool ChunkCache::tick()
{
	return false;
}

bool ChunkCache::shouldSave()
{
	return true;
}

jstring ChunkCache::gatherStats()
{
	// TODO
	return u"ChunkCache: " + String::toString((int)chunks.size());
}

void ChunkCache::prefetchChunks(int_t cx, int_t cz, int_t radius)
{
	if (!genWorker)
		return;

	for (int_t dx = -radius; dx <= radius; dx++)
	{
		for (int_t dz = -radius; dz <= radius; dz++)
		{
			int_t x = cx + dx;
			int_t z = cz + dz;

			if (!fits(x, z))
				continue;

			// Don't request if already cached or already in-flight
			if (hasChunk(x, z))
				continue;

			// Don't request if there's a saved chunk on disk
			// (we can't check cheaply, so only prefetch uncached coords)
			genWorker->requestChunk(x, z);
		}
	}
}
