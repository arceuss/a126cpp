#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include "backends/Vulkan/PipelineCacheFile.h"
#include "tools/headless/TestFramework.h"

static vulkanbackend::PipelineCacheIdentity testPipelineCacheIdentity()
{
	vulkanbackend::PipelineCacheIdentity identity;
	identity.apiVersion = 0x00401000;
	identity.keySchemaVersion = 3;
	identity.shaderABI = 0x0123456789abcdefull;
	identity.vendorID = 0x10de;
	identity.deviceID = 0x2c02;
	identity.driverVersion = 0x12345678;
	for (std::size_t i = 0; i < vulkanbackend::PIPELINE_CACHE_UUID_SIZE; i++)
		identity.uuid[i] = static_cast<unsigned char>(i * 7 + 1);
	return identity;
}

HEADLESS_TEST(pipeline_cache_file, round_trip_and_atomic_replace)
{
	const std::filesystem::path path = "a126cpp-pipeline-cache-file-test.bin";
	const std::filesystem::path temporary = "a126cpp-pipeline-cache-file-test.bin.tmp";
	std::remove(path.string().c_str());
	std::remove(temporary.string().c_str());
	const vulkanbackend::PipelineCacheIdentity identity = testPipelineCacheIdentity();
	const unsigned char first[] = { 1, 2, 3, 4, 5 };
	const unsigned char second[] = { 8, 13, 21 };

	ctx.check(vulkanbackend::savePipelineCacheFile(path, identity, first, sizeof(first)) ==
		vulkanbackend::PipelineCacheFileStatus::Accepted, "first cache save succeeds");
	vulkanbackend::PipelineCacheFileLoad loaded =
		vulkanbackend::loadPipelineCacheFile(path, identity);
	ctx.check(loaded.status == vulkanbackend::PipelineCacheFileStatus::Accepted,
		"saved cache is accepted");
	ctx.check(loaded.payload == std::vector<unsigned char>(first, first + sizeof(first)),
		"saved cache payload is exact");

	ctx.check(vulkanbackend::savePipelineCacheFile(path, identity, second, sizeof(second)) ==
		vulkanbackend::PipelineCacheFileStatus::Accepted, "replacement cache save succeeds");
	loaded = vulkanbackend::loadPipelineCacheFile(path, identity);
	ctx.check(loaded.payload == std::vector<unsigned char>(second, second + sizeof(second)),
		"replacement cache payload is exact");
	ctx.check(!std::filesystem::exists(temporary), "atomic save leaves no temporary file");
	std::remove(path.string().c_str());
}

HEADLESS_TEST(pipeline_cache_file, rejects_foreign_corrupt_and_oversized_data)
{
	const std::filesystem::path path = "a126cpp-pipeline-cache-rejection-test.bin";
	std::remove(path.string().c_str());
	const vulkanbackend::PipelineCacheIdentity identity = testPipelineCacheIdentity();
	const unsigned char payload[] = { 3, 1, 4, 1, 5, 9 };
	ctx.check(vulkanbackend::savePipelineCacheFile(path, identity, payload, sizeof(payload)) ==
		vulkanbackend::PipelineCacheFileStatus::Accepted, "rejection fixture save succeeds");

	vulkanbackend::PipelineCacheIdentity foreign = identity;
	foreign.driverVersion++;
	ctx.check(vulkanbackend::loadPipelineCacheFile(path, foreign).status ==
		vulkanbackend::PipelineCacheFileStatus::IdentityMismatch,
		"foreign driver cache is rejected");
	ctx.check(vulkanbackend::loadPipelineCacheFile(path, identity, 2).status ==
		vulkanbackend::PipelineCacheFileStatus::PayloadTooLarge,
		"oversized cache is rejected before allocation");

	std::ifstream input(path, std::ios::in | std::ios::binary);
	std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),
		std::istreambuf_iterator<char>());
	input.close();
	bytes.back() ^= 0xff;
	std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
	output.write(reinterpret_cast<const char *>(bytes.data()),
		static_cast<std::streamsize>(bytes.size()));
	output.close();
	ctx.check(vulkanbackend::loadPipelineCacheFile(path, identity).status ==
		vulkanbackend::PipelineCacheFileStatus::ChecksumMismatch,
		"checksum mismatch is rejected");

	std::ofstream truncated(path, std::ios::out | std::ios::binary | std::ios::trunc);
	truncated.write("A126", 4);
	truncated.close();
	ctx.check(vulkanbackend::loadPipelineCacheFile(path, identity).status ==
		vulkanbackend::PipelineCacheFileStatus::Truncated, "truncated cache is rejected");
	std::remove(path.string().c_str());
}
