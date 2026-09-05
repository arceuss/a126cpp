#include "world/level/chunk/storage/RegionFile.h"

#include <cstring>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <stdexcept>

#include "zlib.h"

const byte_t RegionFile::emptySector[4096] = {};

// Big-endian int helpers for fstream
static void writeBE32(std::fstream &f, int_t value)
{
	byte_t buf[4];
	buf[0] = (value >> 24) & 0xFF;
	buf[1] = (value >> 16) & 0xFF;
	buf[2] = (value >> 8) & 0xFF;
	buf[3] = value & 0xFF;
	f.write(reinterpret_cast<char *>(buf), 4);
}

static int_t readBE32(std::fstream &f)
{
	byte_t buf[4] = {};
	if (!f.read(reinterpret_cast<char *>(buf), 4))
		throw std::runtime_error("Short region integer read");
	return (static_cast<int_t>(buf[0] & 0xFF) << 24) |
	       (static_cast<int_t>(buf[1] & 0xFF) << 16) |
	       (static_cast<int_t>(buf[2] & 0xFF) << 8) |
	       static_cast<int_t>(buf[3] & 0xFF);
}

// The stored path is UTF-8. On Windows a narrow fstream open would go through
// the ANSI code page, while File opens wide paths, so route through
// std::filesystem::u8path: a no-op on POSIX, UTF-8 to wide on MSVC.
static void openRegionStream(std::fstream &f, const std::string &utf8Path, std::ios::openmode mode)
{
	f.open(std::filesystem::u8path(utf8Path), mode);
}

RegionFile::RegionFile(const std::string &path)
{
	filePath = path;
	sizeDelta = 0;

	const bool existed = std::filesystem::exists(std::filesystem::u8path(path));
	if (!existed)
	{
		std::ofstream created(std::filesystem::u8path(path), std::ios::binary);
		if (!created)
			throw std::runtime_error("Failed to create region file: " + path);
	}
	openRegionStream(dataFile, path, std::ios::in | std::ios::out | std::ios::binary);
	if (!dataFile.is_open())
		throw std::runtime_error("Failed to open region file: " + path);
	dataFile.exceptions(std::ios::badbit | std::ios::failbit);

	// Get file size
	dataFile.seekg(0, std::ios::end);
	long_t fileLength = static_cast<long_t>(dataFile.tellg());

	if (existed && fileLength < 8192)
		throw std::runtime_error("Truncated region header: " + path);
	if (!existed)
	{
		dataFile.seekp(0, std::ios::beg);
		for (int_t i = 0; i < 1024; ++i)
			writeBE32(dataFile, 0);
		for (int_t i = 0; i < 1024; ++i)
			writeBE32(dataFile, 0);

		sizeDelta += 8192;
		dataFile.flush();

		dataFile.seekg(0, std::ios::end);
		fileLength = static_cast<long_t>(dataFile.tellg());
	}

	// Ignore an unreferenced partial tail left by a failed append. Referenced
	// sectors must still fit completely; opening a save never rewrites it.

	// Build sector free list
	int_t totalSectors = static_cast<int_t>(fileLength / 4096);
	sectorFree.resize(totalSectors, true);

	// Sectors 0 and 1 are the header
	sectorFree[0] = false;
	if (totalSectors > 1)
		sectorFree[1] = false;

	// Read offset table
	dataFile.seekg(0, std::ios::beg);
	for (int_t i = 0; i < 1024; ++i)
	{
		int_t offset = readBE32(dataFile);
		offsets[i] = offset;
		if (offset != 0)
		{
			const uint_t sectorStart = static_cast<uint_t>(offset) >> 8;
			const uint_t sectorCount = static_cast<uint_t>(offset) & 255U;
			if (sectorStart < 2 || sectorCount == 0 ||
				sectorStart + sectorCount > sectorFree.size())
				throw std::runtime_error("Invalid region sector allocation: " + path);
			for (uint_t s = 0; s < sectorCount; ++s)
			{
				if (!sectorFree[sectorStart + s])
					throw std::runtime_error("Overlapping region sector allocation: " + path);
				sectorFree[sectorStart + s] = false;
			}
		}
	}

	// Read timestamp table
	for (int_t i = 0; i < 1024; ++i)
		timestamps[i] = readBE32(dataFile);
}

RegionFile::~RegionFile()
{
	try { close(); }
	catch (const std::exception &error)
	{
		std::cerr << "Failed to close region " << filePath << ": " << error.what() << '\n';
	}
}

void RegionFile::close()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	if (dataFile.is_open())
	{
		dataFile.flush();
		dataFile.close();
	}
}

int_t RegionFile::getSizeDelta()
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	int_t delta = sizeDelta;
	sizeDelta = 0;
	return delta;
}

bool RegionFile::outOfBounds(int_t x, int_t z)
{
	return x < 0 || x >= 32 || z < 0 || z >= 32;
}

int_t RegionFile::getOffset(int_t x, int_t z)
{
	return offsets[x + z * 32];
}

bool RegionFile::hasChunk(int_t x, int_t z)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	if (outOfBounds(x, z))
		return false;
	return getOffset(x, z) != 0;
}

std::vector<byte_t> RegionFile::getChunkData(int_t x, int_t z)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	if (outOfBounds(x, z))
		throw std::out_of_range("Region chunk coordinates");
	const int_t offset = getOffset(x, z);
	if (offset == 0)
		return {};

	const uint_t sectorNumber = static_cast<uint_t>(offset) >> 8;
	const uint_t numSectors = static_cast<uint_t>(offset) & 255U;
	const std::string location = filePath + " chunk " + std::to_string(x) + "," + std::to_string(z);
	if (sectorNumber < 2 || numSectors == 0 || sectorNumber + numSectors > sectorFree.size())
		throw std::runtime_error("Invalid region allocation: " + location);
	dataFile.seekg(static_cast<long_t>(sectorNumber) * 4096);
	const int_t length = readBE32(dataFile);
	if (length <= 1 || static_cast<uint_t>(length) > 4096 * numSectors - 4)
		throw std::runtime_error("Invalid region record length: " + location);

	byte_t compressionType = 0;
	dataFile.read(reinterpret_cast<char *>(&compressionType), 1);
	if (compressionType != 1 && compressionType != 2)
		throw std::runtime_error("Unsupported region compression: " + location);
	std::vector<byte_t> compressedData(static_cast<std::size_t>(length - 1));
	dataFile.read(reinterpret_cast<char *>(compressedData.data()), length - 1);

	z_stream stream = {};
	stream.next_in = reinterpret_cast<Bytef *>(compressedData.data());
	stream.avail_in = static_cast<uInt>(compressedData.size());
	if (inflateInit2(&stream, compressionType == 1 ? MAX_WBITS + 16 : MAX_WBITS) != Z_OK)
		throw std::runtime_error("Region inflate initialization failed: " + location);
	struct InflateEnd
	{
		z_stream &stream;
		~InflateEnd() { inflateEnd(&stream); }
	} cleanup{stream};

	std::vector<byte_t> result;
	byte_t output[16384];
	for (;;)
	{
		stream.next_out = reinterpret_cast<Bytef *>(output);
		stream.avail_out = sizeof(output);
		const uInt inputBefore = stream.avail_in;
		const int status = inflate(&stream, Z_NO_FLUSH);
		const std::size_t produced = sizeof(output) - stream.avail_out;
		if (status == Z_NEED_DICT)
			throw std::runtime_error("Region stream requires a dictionary: " + location);
		if (status != Z_OK && status != Z_STREAM_END && status != Z_BUF_ERROR)
			throw std::runtime_error("Corrupt region compressed stream: " + location);
		if (produced > MAX_CHUNK_BYTES - result.size())
			throw std::runtime_error("Region decoded chunk exceeds size limit: " + location);
		result.insert(result.end(), output, output + produced);
		if (status == Z_STREAM_END)
		{
			if (result.empty())
				throw std::runtime_error("Empty region chunk payload: " + location);
			return result;
		}
		if (stream.avail_in == inputBefore && produced == 0)
			throw std::runtime_error("Truncated region compressed stream: " + location);
	}
}

void RegionFile::writeChunkData(int_t x, int_t z, const byte_t *data, int_t length)
{
	std::lock_guard<std::recursive_mutex> lock(mutex);
	if (outOfBounds(x, z))
		throw std::out_of_range("Region chunk coordinates");
	if (!dataFile.is_open() || !dataFile)
		throw std::runtime_error("Region stream is not writable: " + filePath);
	if (length <= 0 || data == nullptr || length > 255 * 4096 - 5)
		throw std::runtime_error("Invalid or oversized compressed region chunk: " + filePath);

	const uint_t oldOffset = static_cast<uint_t>(getOffset(x, z));
	const uint_t oldSector = oldOffset >> 8;
	const uint_t oldCount = oldOffset & 255U;
	const int_t neededSectors = (length + 5 + 4095) / 4096;

	// Keep the old record allocated until the replacement is written and flushed.
	int_t runStart = -1;
	int_t runLength = 0;
	for (int_t i = 2; i < static_cast<int_t>(sectorFree.size()); ++i)
	{
		if (sectorFree[i])
		{
			if (runLength == 0)
				runStart = i;
			if (++runLength == neededSectors)
				break;
		}
		else
			runLength = 0;
	}
	int_t sectorNumber = runLength == neededSectors ? runStart : static_cast<int_t>(sectorFree.size());
	if (sectorNumber > 0xFFFFFF)
		throw std::runtime_error("Region sector offset exceeds format limit: " + filePath);
	if (runLength != neededSectors)
	{
		// Allocate memory before writing so allocation failure cannot follow publication.
		sectorFree.resize(static_cast<std::size_t>(sectorNumber) + neededSectors, true);
		dataFile.seekp(static_cast<long_t>(sectorNumber) * 4096);
		for (int_t i = 0; i < neededSectors; ++i)
			dataFile.write(reinterpret_cast<const char *>(emptySector), 4096);
		sizeDelta += 4096 * neededSectors;
	}

	writeSectors(sectorNumber, data, length);
	dataFile.flush();
	const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	const int_t timestamp = static_cast<int_t>(
		std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
	setTimestamp(x, z, timestamp);
	setOffset(x, z, static_cast<int_t>((static_cast<uint_t>(sectorNumber) << 8) | neededSectors));
	for (int_t i = 0; i < neededSectors; ++i)
		sectorFree[sectorNumber + i] = false;
	for (uint_t i = 0; i < oldCount; ++i)
		sectorFree[oldSector + i] = true;
}

void RegionFile::writeSectors(int_t sectorNumber, const byte_t *data, int_t length)
{
	dataFile.seekp(static_cast<long_t>(sectorNumber) * 4096);

	// Write length + 1 (for compression type byte)
	byte_t header[5];
	int_t totalLen = length + 1;
	header[0] = (totalLen >> 24) & 0xFF;
	header[1] = (totalLen >> 16) & 0xFF;
	header[2] = (totalLen >> 8) & 0xFF;
	header[3] = totalLen & 0xFF;
	header[4] = 2; // Zlib compression
	dataFile.write(reinterpret_cast<char *>(header), 5);
	dataFile.write(reinterpret_cast<const char *>(data), length);
}

void RegionFile::setOffset(int_t x, int_t z, int_t val)
{
	dataFile.seekp(static_cast<long_t>((x + z * 32)) * 4);
	writeBE32(dataFile, val);
	dataFile.flush();
	offsets[x + z * 32] = val;
}

void RegionFile::setTimestamp(int_t x, int_t z, int_t val)
{
	dataFile.seekp(4096 + static_cast<long_t>((x + z * 32)) * 4);
	writeBE32(dataFile, val);
	dataFile.flush();
	timestamps[x + z * 32] = val;
}
