#include "backends/Vulkan/PipelineCacheFile.h"

#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace vulkanbackend
{

static const unsigned char PIPELINE_CACHE_MAGIC[8] = {
	'A', '1', '2', '6', 'V', 'K', 'P', 'C'
};
static const std::uint32_t PIPELINE_CACHE_FILE_VERSION = 1;
static const std::size_t PIPELINE_CACHE_HEADER_SIZE = 76;
static const std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ull;
static const std::uint64_t FNV_PRIME = 1099511628211ull;

static void appendU32(std::vector<unsigned char> &bytes, std::uint32_t value)
{
	for (int i = 0; i < 4; i++)
		bytes.push_back(static_cast<unsigned char>((value >> (i * 8)) & 0xff));
}

static void appendU64(std::vector<unsigned char> &bytes, std::uint64_t value)
{
	for (int i = 0; i < 8; i++)
		bytes.push_back(static_cast<unsigned char>((value >> (i * 8)) & 0xff));
}

static bool readU32(const std::vector<unsigned char> &bytes, std::size_t &offset,
	std::uint32_t &value)
{
	if (bytes.size() - offset < 4)
		return false;
	value = 0;
	for (int i = 0; i < 4; i++)
		value |= static_cast<std::uint32_t>(bytes[offset++]) << (i * 8);
	return true;
}

static bool readU64(const std::vector<unsigned char> &bytes, std::size_t &offset,
	std::uint64_t &value)
{
	if (bytes.size() - offset < 8)
		return false;
	value = 0;
	for (int i = 0; i < 8; i++)
		value |= static_cast<std::uint64_t>(bytes[offset++]) << (i * 8);
	return true;
}

static bool sameIdentity(const PipelineCacheIdentity &left, const PipelineCacheIdentity &right)
{
	return left.apiVersion == right.apiVersion &&
		left.keySchemaVersion == right.keySchemaVersion &&
		left.shaderABI == right.shaderABI &&
		left.vendorID == right.vendorID &&
		left.deviceID == right.deviceID &&
		left.driverVersion == right.driverVersion &&
		std::memcmp(left.uuid, right.uuid, PIPELINE_CACHE_UUID_SIZE) == 0;
}

static std::vector<unsigned char> makeHeader(const PipelineCacheIdentity &identity,
	std::size_t payloadSize, std::uint64_t checksum)
{
	std::vector<unsigned char> header;
	header.reserve(PIPELINE_CACHE_HEADER_SIZE);
	header.insert(header.end(), PIPELINE_CACHE_MAGIC,
		PIPELINE_CACHE_MAGIC + sizeof(PIPELINE_CACHE_MAGIC));
	appendU32(header, PIPELINE_CACHE_FILE_VERSION);
	appendU32(header, static_cast<std::uint32_t>(PIPELINE_CACHE_HEADER_SIZE));
	appendU32(header, identity.apiVersion);
	appendU32(header, identity.keySchemaVersion);
	appendU64(header, identity.shaderABI);
	appendU32(header, identity.vendorID);
	appendU32(header, identity.deviceID);
	appendU32(header, identity.driverVersion);
	header.insert(header.end(), identity.uuid, identity.uuid + PIPELINE_CACHE_UUID_SIZE);
	appendU64(header, static_cast<std::uint64_t>(payloadSize));
	appendU64(header, checksum);
	return header;
}

static bool replaceFile(const std::filesystem::path &temporary,
	const std::filesystem::path &destination)
{
#ifdef _WIN32
	return MoveFileExW(temporary.c_str(), destination.c_str(),
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
	std::error_code error;
	std::filesystem::rename(temporary, destination, error);
	return !error;
#endif
}

const char *pipelineCacheFileStatusName(PipelineCacheFileStatus status)
{
	switch (status)
	{
		case PipelineCacheFileStatus::Accepted: return "accepted";
		case PipelineCacheFileStatus::Missing: return "missing";
		case PipelineCacheFileStatus::ReadError: return "read-error";
		case PipelineCacheFileStatus::WriteError: return "write-error";
		case PipelineCacheFileStatus::Truncated: return "truncated";
		case PipelineCacheFileStatus::InvalidHeader: return "invalid-header";
		case PipelineCacheFileStatus::IdentityMismatch: return "identity-mismatch";
		case PipelineCacheFileStatus::PayloadTooLarge: return "payload-too-large";
		case PipelineCacheFileStatus::ChecksumMismatch: return "checksum-mismatch";
	}
	return "unknown";
}

std::uint64_t pipelineCacheChecksum(const unsigned char *data, std::size_t size)
{
	std::uint64_t hash = FNV_OFFSET_BASIS;
	for (std::size_t i = 0; i < size; i++)
	{
		hash ^= data[i];
		hash *= FNV_PRIME;
	}
	return hash;
}

PipelineCacheFileLoad loadPipelineCacheFile(const std::filesystem::path &path,
	const PipelineCacheIdentity &identity, std::size_t maximumPayloadSize)
{
	PipelineCacheFileLoad result;
	std::error_code fileError;
	if (!std::filesystem::exists(path, fileError))
	{
		result.status = fileError ? PipelineCacheFileStatus::ReadError :
			PipelineCacheFileStatus::Missing;
		return result;
	}
	const std::uintmax_t fileSize = std::filesystem::file_size(path, fileError);
	if (fileError)
	{
		result.status = PipelineCacheFileStatus::ReadError;
		return result;
	}
	if (fileSize < PIPELINE_CACHE_HEADER_SIZE)
	{
		result.status = PipelineCacheFileStatus::Truncated;
		return result;
	}
	if (fileSize - PIPELINE_CACHE_HEADER_SIZE > maximumPayloadSize ||
		fileSize > static_cast<std::uintmax_t>(std::numeric_limits<std::size_t>::max()))
	{
		result.status = PipelineCacheFileStatus::PayloadTooLarge;
		return result;
	}

	std::ifstream input(path, std::ios::in | std::ios::binary);
	if (!input)
	{
		result.status = PipelineCacheFileStatus::ReadError;
		return result;
	}
	std::vector<unsigned char> bytes(static_cast<std::size_t>(fileSize));
	input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	if (!input || input.gcount() != static_cast<std::streamsize>(bytes.size()))
	{
		result.status = PipelineCacheFileStatus::ReadError;
		return result;
	}

	std::size_t offset = 0;
	if (std::memcmp(bytes.data(), PIPELINE_CACHE_MAGIC, sizeof(PIPELINE_CACHE_MAGIC)) != 0)
	{
		result.status = PipelineCacheFileStatus::InvalidHeader;
		return result;
	}
	offset += sizeof(PIPELINE_CACHE_MAGIC);
	std::uint32_t fileVersion = 0;
	std::uint32_t headerSize = 0;
	PipelineCacheIdentity stored;
	std::uint64_t payloadSize = 0;
	std::uint64_t checksum = 0;
	if (!readU32(bytes, offset, fileVersion) || !readU32(bytes, offset, headerSize) ||
		!readU32(bytes, offset, stored.apiVersion) ||
		!readU32(bytes, offset, stored.keySchemaVersion) ||
		!readU64(bytes, offset, stored.shaderABI) ||
		!readU32(bytes, offset, stored.vendorID) ||
		!readU32(bytes, offset, stored.deviceID) ||
		!readU32(bytes, offset, stored.driverVersion) ||
		bytes.size() - offset < PIPELINE_CACHE_UUID_SIZE)
	{
		result.status = PipelineCacheFileStatus::Truncated;
		return result;
	}
	std::memcpy(stored.uuid, bytes.data() + offset, PIPELINE_CACHE_UUID_SIZE);
	offset += PIPELINE_CACHE_UUID_SIZE;
	if (!readU64(bytes, offset, payloadSize) || !readU64(bytes, offset, checksum))
	{
		result.status = PipelineCacheFileStatus::Truncated;
		return result;
	}
	if (fileVersion != PIPELINE_CACHE_FILE_VERSION || headerSize != PIPELINE_CACHE_HEADER_SIZE ||
		offset != PIPELINE_CACHE_HEADER_SIZE)
	{
		result.status = PipelineCacheFileStatus::InvalidHeader;
		return result;
	}
	if (!sameIdentity(stored, identity))
	{
		result.status = PipelineCacheFileStatus::IdentityMismatch;
		return result;
	}
	if (payloadSize > maximumPayloadSize)
	{
		result.status = PipelineCacheFileStatus::PayloadTooLarge;
		return result;
	}
	if (payloadSize != bytes.size() - PIPELINE_CACHE_HEADER_SIZE)
	{
		result.status = PipelineCacheFileStatus::Truncated;
		return result;
	}
	if (pipelineCacheChecksum(bytes.data() + PIPELINE_CACHE_HEADER_SIZE,
		static_cast<std::size_t>(payloadSize)) != checksum)
	{
		result.status = PipelineCacheFileStatus::ChecksumMismatch;
		return result;
	}

	result.payload.assign(bytes.begin() + PIPELINE_CACHE_HEADER_SIZE, bytes.end());
	result.status = PipelineCacheFileStatus::Accepted;
	return result;
}

PipelineCacheFileStatus savePipelineCacheFile(const std::filesystem::path &path,
	const PipelineCacheIdentity &identity, const unsigned char *payload, std::size_t payloadSize,
	std::size_t maximumPayloadSize)
{
	if (payloadSize > maximumPayloadSize)
		return PipelineCacheFileStatus::PayloadTooLarge;
	if (payloadSize != 0 && payload == nullptr)
		return PipelineCacheFileStatus::WriteError;

	const std::vector<unsigned char> header = makeHeader(identity, payloadSize,
		pipelineCacheChecksum(payload, payloadSize));
	std::filesystem::path temporary = path;
	temporary += ".tmp";
	std::error_code error;
	if (path.has_parent_path())
		std::filesystem::create_directories(path.parent_path(), error);
	if (error)
		return PipelineCacheFileStatus::WriteError;

	std::ofstream output(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!output)
		return PipelineCacheFileStatus::WriteError;
	output.write(reinterpret_cast<const char *>(header.data()),
		static_cast<std::streamsize>(header.size()));
	if (payloadSize != 0)
		output.write(reinterpret_cast<const char *>(payload),
			static_cast<std::streamsize>(payloadSize));
	output.flush();
	output.close();
	if (!output)
	{
		std::filesystem::remove(temporary, error);
		return PipelineCacheFileStatus::WriteError;
	}
	if (!replaceFile(temporary, path))
	{
		std::filesystem::remove(temporary, error);
		return PipelineCacheFileStatus::WriteError;
	}
	return PipelineCacheFileStatus::Accepted;
}

}
