#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace vulkanbackend
{

static const std::size_t PIPELINE_CACHE_UUID_SIZE = 16;
static const std::size_t PIPELINE_CACHE_MAX_PAYLOAD_SIZE = 64 * 1024 * 1024;

struct PipelineCacheIdentity
{
	std::uint32_t apiVersion = 0;
	std::uint32_t keySchemaVersion = 0;
	std::uint64_t shaderABI = 0;
	std::uint32_t vendorID = 0;
	std::uint32_t deviceID = 0;
	std::uint32_t driverVersion = 0;
	unsigned char uuid[PIPELINE_CACHE_UUID_SIZE] = {};
};

enum class PipelineCacheFileStatus
{
	Accepted,
	Missing,
	ReadError,
	WriteError,
	Truncated,
	InvalidHeader,
	IdentityMismatch,
	PayloadTooLarge,
	ChecksumMismatch
};

struct PipelineCacheFileLoad
{
	PipelineCacheFileStatus status = PipelineCacheFileStatus::Missing;
	std::vector<unsigned char> payload;
};

const char *pipelineCacheFileStatusName(PipelineCacheFileStatus status);
std::uint64_t pipelineCacheChecksum(const unsigned char *data, std::size_t size);
PipelineCacheFileLoad loadPipelineCacheFile(const std::filesystem::path &path,
	const PipelineCacheIdentity &identity,
	std::size_t maximumPayloadSize = PIPELINE_CACHE_MAX_PAYLOAD_SIZE);
PipelineCacheFileStatus savePipelineCacheFile(const std::filesystem::path &path,
	const PipelineCacheIdentity &identity, const unsigned char *payload, std::size_t payloadSize,
	std::size_t maximumPayloadSize = PIPELINE_CACHE_MAX_PAYLOAD_SIZE);

}
