#include "world/level/chunk/storage/McRegionChunkStorage.h"

#include "world/level/chunk/storage/OldChunkStorage.h"
#include "world/level/chunk/storage/RegionFileCache.h"
#include "world/level/chunk/storage/RegionFile.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/Level.h"

#include "nbt/NbtIo.h"
#include "nbt/CompoundTag.h"

#include "java/File.h"
#include "java/String.h"

#include "util/Memory.h"

#include <sstream>
#include <iostream>
#include <stdexcept>
#include <limits>

#include "zlib.h"

// Minimal memory-backed istream for reading NBT from a byte buffer
namespace {

class MemBuf : public std::basic_streambuf<char>
{
public:
	MemBuf(const char *p, size_t l)
	{
		setg(const_cast<char *>(p), const_cast<char *>(p), const_cast<char *>(p) + l);
	}
};

class MemStream : public std::istream
{
public:
	MemStream(const char *p, size_t l) : std::istream(&buffer), buffer(p, l)
	{
		rdbuf(&buffer);
	}
private:
	MemBuf buffer;
};

} // anonymous namespace

McRegionChunkStorage::McRegionChunkStorage(std::shared_ptr<File> dir, bool create)
{
	baseDir = String::toUTF8(dir->toString());

	if (create)
	{
		if (!dir->exists())
			dir->mkdirs();
	}
}

std::shared_ptr<LevelChunk> McRegionChunkStorage::load(Level &level, int_t x, int_t z)
{
	// Get decompressed chunk data from region file
	std::vector<byte_t> data = RegionFileCache::getRegionFile(baseDir, x, z)->getChunkData(x & 31, z & 31);
	if (data.empty())
		return nullptr;

	// Parse NBT from decompressed data
	MemStream ms(reinterpret_cast<const char *>(data.data()), data.size());
	std::unique_ptr<CompoundTag> rootTag(NbtIo::read(ms));

	if (!rootTag->contains(u"Level"))
	{
		throw std::runtime_error("Region chunk is missing Level data at " + std::to_string(x) + "," + std::to_string(z));
	}
	if (!rootTag->getCompound(u"Level")->contains(u"Blocks"))
	{
		throw std::runtime_error("Region chunk is missing Blocks data at " + std::to_string(x) + "," + std::to_string(z));
	}

	// Reuse OldChunkStorage's NBT-to-chunk deserialization; the chunk compound
	// is identical between the file-per-chunk and McRegion layouts.
	std::shared_ptr<LevelChunk> chunk = OldChunkStorage::load(level, *rootTag->getCompound(u"Level"));
	if (!chunk->isAt(x, z))
	{
		std::cout << "Chunk file at " << x << "," << z << " is in the wrong location; relocating. (Expected "
		          << x << ", " << z << ", got " << chunk->x << ", " << chunk->z << ")\n";
		rootTag->getCompound(u"Level")->putInt(u"xPos", x);
		rootTag->getCompound(u"Level")->putInt(u"zPos", z);
		chunk = OldChunkStorage::load(level, *rootTag->getCompound(u"Level"));
	}

	return chunk;
}

void McRegionChunkStorage::save(Level &level, LevelChunk &chunk)
{
	level.checkSession();

	// Serialize chunk to NBT
	std::unique_ptr<CompoundTag> rootTag = std::make_unique<CompoundTag>();
	std::shared_ptr<CompoundTag> levelTag = Util::make_shared<CompoundTag>();
	rootTag->put(u"Level", levelTag);

	// Serialize chunk NBT using shared format helpers
	OldChunkStorage::save(chunk, level, *levelTag);

	// Write NBT to buffer
	std::stringstream ss;
	ss.exceptions(std::ios::badbit | std::ios::failbit);
	NbtIo::write(*rootTag, ss);
	std::string nbtData = ss.str();
	if (nbtData.size() > RegionFile::MAX_CHUNK_BYTES)
		throw std::runtime_error("Chunk NBT exceeds region decoded size limit");

	// Zlib compress
	uLongf compBound = compressBound(static_cast<uLong>(nbtData.size()));
	std::vector<byte_t> compressed(compBound);
	uLongf compSize = compBound;

	int ret = compress2(
		reinterpret_cast<Bytef *>(compressed.data()), &compSize,
		reinterpret_cast<const Bytef *>(nbtData.data()), static_cast<uLong>(nbtData.size()),
		Z_DEFAULT_COMPRESSION
	);

	if (ret != Z_OK)
	{
		throw std::runtime_error("Failed to compress chunk at " + std::to_string(chunk.x) + "," + std::to_string(chunk.z));
	}
	if (compSize > static_cast<uLongf>(std::numeric_limits<int_t>::max()))
		throw std::runtime_error("Compressed chunk exceeds region size limit");

	// Write to region file
	auto regionFile = RegionFileCache::getRegionFile(baseDir, chunk.x, chunk.z);
	regionFile->writeChunkData(chunk.x & 31, chunk.z & 31, compressed.data(), static_cast<int_t>(compSize));

	// Update sizeOnDisk
	level.sizeOnDisk += static_cast<long_t>(RegionFileCache::getSizeDelta(baseDir, chunk.x, chunk.z));
}

void McRegionChunkStorage::saveEntities(Level &level, LevelChunk &chunk)
{
	// Not used in McRegion format (entities are part of chunk data)
}

void McRegionChunkStorage::tick()
{
	// No background processing needed
}

void McRegionChunkStorage::flush()
{
	RegionFileCache::clearCache();
}
