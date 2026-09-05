#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/level/chunk/storage/RegionFile.h"
#include "zlib.h"

static std::string regionPath(headless::TempDir &directory)
{
	return String::toUTF8(directory.path()) + "/r.0.0.mcr";
}

static std::vector<byte_t> compressedBytes(const std::vector<byte_t> &input, int windowBits = MAX_WBITS,
	bool dictionary = false)
{
	z_stream stream = {};
	if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		throw std::runtime_error("test deflate initialization failed");
	if (dictionary)
	{
		const Bytef data[] = {1, 2, 3, 4};
		deflateSetDictionary(&stream, data, sizeof(data));
	}
	std::vector<byte_t> output(deflateBound(&stream, static_cast<uLong>(input.size())));
	stream.next_in = reinterpret_cast<Bytef *>(const_cast<byte_t *>(input.data()));
	stream.avail_in = static_cast<uInt>(input.size());
	stream.next_out = reinterpret_cast<Bytef *>(output.data());
	stream.avail_out = static_cast<uInt>(output.size());
	const int status = deflate(&stream, Z_FINISH);
	output.resize(stream.total_out);
	deflateEnd(&stream);
	if (status != Z_STREAM_END)
		throw std::runtime_error("test deflate failed");
	return output;
}

static std::vector<char> fileBytes(const std::string &path)
{
	std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
	return std::vector<char>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

static void replaceCompressionType(const std::string &path, int type)
{
	std::fstream file(std::filesystem::u8path(path), std::ios::binary | std::ios::in | std::ios::out);
	file.exceptions(std::ios::badbit | std::ios::failbit);
	file.seekp(8196);
	file.put(static_cast<char>(type));
}

HEADLESS_TEST(region, gzip_and_zlib_round_trip)
{
	headless::TempDir directory(ctx, "region-codecs");
	std::vector<byte_t> input(70000);
	for (size_t i = 0; i < input.size(); ++i)
		input[i] = static_cast<byte_t>(i * 31);
	for (int type = 1; type <= 2; ++type)
	{
		const std::string path = regionPath(directory) + std::to_string(type);
		const std::vector<byte_t> packed = compressedBytes(input, type == 1 ? MAX_WBITS + 16 : MAX_WBITS);
		{
			RegionFile region(path);
			region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size()));
		}
		replaceCompressionType(path, type);
		RegionFile reopened(path);
		ctx.check(reopened.getChunkData(0, 0) == input, "decoded bytes match input across output buffers");
		ctx.check(reopened.getChunkData(1, 0).empty(), "absent chunk remains distinct from decode failure");
	}
}

HEADLESS_TEST(region, truncated_stream_throws_without_modifying_file)
{
	headless::TempDir directory(ctx, "region-truncated");
	const std::string path = regionPath(directory);
	std::vector<byte_t> packed = compressedBytes(std::vector<byte_t>(10000, 42));
	packed.resize(packed.size() - 2);
	{
		RegionFile region(path);
		region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size()));
	}
	const std::vector<char> before = fileBytes(path);
	bool failed = false;
	try { RegionFile region(path); region.getChunkData(0, 0); }
	catch (const std::exception &) { failed = true; }
	ctx.check(failed, "truncated checksum must throw instead of hanging or reporting missing");
	ctx.check(fileBytes(path) == before, "failed load preserves saved file");
}

HEADLESS_TEST(region, dictionary_and_invalid_compression_are_errors)
{
	headless::TempDir directory(ctx, "region-invalid-codec");
	for (int type = 0; type < 2; ++type)
	{
		const std::string path = regionPath(directory) + std::to_string(type);
		const std::vector<byte_t> packed = compressedBytes(std::vector<byte_t>(128, 7), MAX_WBITS, type == 1);
		{
			RegionFile region(path);
			region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size()));
		}
		if (type == 0)
			replaceCompressionType(path, 3);
		bool failed = false;
		try { RegionFile region(path); region.getChunkData(0, 0); }
		catch (const std::exception &) { failed = true; }
		ctx.check(failed, "unsupported compression and dictionary input must fail");
	}
}

HEADLESS_TEST(region, short_header_is_not_reinitialized)
{
	headless::TempDir directory(ctx, "region-short-header");
	const std::string path = regionPath(directory);
	{ std::ofstream file(std::filesystem::u8path(path), std::ios::binary); file << "partial"; }
	const std::vector<char> before = fileBytes(path);
	bool failed = false;
	try { RegionFile region(path); }
	catch (const std::exception &) { failed = true; }
	ctx.check(failed, "partial header is corruption, not a new region");
	ctx.check(fileBytes(path) == before, "partial header is preserved");
}

HEADLESS_TEST(region, closed_file_write_fails_and_preserves_saved_chunk)
{
	headless::TempDir directory(ctx, "region-save-failure");
	const std::string path = regionPath(directory);
	const std::vector<byte_t> original(10000, 12);
	const std::vector<byte_t> packed = compressedBytes(original);
	RegionFile region(path);
	region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size()));
	region.close();
	bool failed = false;
	try { region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size())); }
	catch (const std::exception &) { failed = true; }
	ctx.check(failed, "closed stream cannot report a successful save");
	RegionFile reopened(path);
	ctx.check(reopened.getChunkData(0, 0) == original, "previously saved chunk remains readable");
}

HEADLESS_TEST(region, oversized_inflate_fails_without_modifying_file)
{
	headless::TempDir directory(ctx, "region-inflate-limit");
	const std::string path = regionPath(directory);
	const std::vector<byte_t> packed = compressedBytes(std::vector<byte_t>(RegionFile::MAX_CHUNK_BYTES + 1, 0));
	{
		RegionFile region(path);
		region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size()));
	}
	const std::vector<char> before = fileBytes(path);
	bool failed = false;
	try { RegionFile region(path); region.getChunkData(0, 0); }
	catch (const std::exception &) { failed = true; }
	ctx.check(failed, "decoded chunk resource limit is enforced");
	ctx.check(fileBytes(path) == before, "oversized input remains available for recovery");
}

HEADLESS_TEST(region, exact_sector_record_does_not_allocate_extra_sector)
{
	headless::TempDir directory(ctx, "region-sector-ceiling");
	const std::string path = regionPath(directory);
	const std::vector<byte_t> packed(4091, 0);
	{
		RegionFile region(path);
		region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size()));
	}
	ctx.checkEqual(fileBytes(path).size(), 8192 + 4096, "4091 payload bytes plus framing occupy one sector");
}

HEADLESS_TEST(region, record_length_cannot_cross_allocated_sector)
{
	headless::TempDir directory(ctx, "region-record-length");
	const std::string path = regionPath(directory);
	const std::vector<byte_t> packed = compressedBytes(std::vector<byte_t>(128, 7));
	{
		RegionFile region(path);
		region.writeChunkData(0, 0, packed.data(), static_cast<int_t>(packed.size()));
	}
	{
		std::fstream file(std::filesystem::u8path(path), std::ios::binary | std::ios::in | std::ios::out);
		file.seekp(8192);
		const char invalidLength[] = {0, 0, 16, 0};
		file.write(invalidLength, sizeof(invalidLength));
	}
	bool failed = false;
	try { RegionFile region(path); region.getChunkData(0, 0); }
	catch (const std::exception &) { failed = true; }
	ctx.check(failed, "record length includes its framing and must fit its allocation");
}
