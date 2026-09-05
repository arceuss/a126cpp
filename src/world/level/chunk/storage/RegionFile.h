#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <mutex>

#include "java/Type.h"

class RegionFile
{
private:
	static const int SECTOR_BYTES = 4096;
	static const int SECTOR_INTS = 1024;

	std::string filePath;
	std::fstream dataFile;
	int_t offsets[1024] = {};
	int_t timestamps[1024] = {};
	std::vector<bool> sectorFree;
	int_t sizeDelta = 0;

	std::recursive_mutex mutex;

	static const byte_t emptySector[4096];

	bool outOfBounds(int_t x, int_t z);
	int_t getOffset(int_t x, int_t z);
	void setOffset(int_t x, int_t z, int_t val);
	void setTimestamp(int_t x, int_t z, int_t val);
	void writeSectors(int_t sectorNumber, const byte_t *data, int_t length);

public:
	// Resource limit for decoded chunk NBT, not a McRegion format limit.
	static const std::size_t MAX_CHUNK_BYTES = 64 * 1024 * 1024;

	RegionFile(const std::string &path);
	~RegionFile();

	// Empty only when absent. Corrupt data and I/O failures throw std::runtime_error.
	std::vector<byte_t> getChunkData(int_t x, int_t z);

	// Returns only after a checked write and flush; failures throw.
	void writeChunkData(int_t x, int_t z, const byte_t *data, int_t length);

	bool hasChunk(int_t x, int_t z);

	int_t getSizeDelta();

	void close();
};
