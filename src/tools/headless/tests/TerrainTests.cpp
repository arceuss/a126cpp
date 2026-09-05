#include <string>
#include "tools/headless/TestFramework.h"
#include "tools/headless/oracle/TerrainOracleData.h"
#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/levelgen/RandomLevelSource.h"

HEADLESS_TEST(worldgen, raw_overworld_chunks_match_alpha_bytecode)
{
	const long_t seeds[] = {0LL, 4242424242LL, -123456789LL};
	const int coords[][2] = {{0, 0}, {-3, 5}};
	const headless::TerrainRun *fixtures[] = {headless::terrain0, headless::terrain1,
		headless::terrain2, headless::terrain3, headless::terrain4, headless::terrain5};
	int fixture = 0;
	for (long_t seed : seeds)
		for (const auto &coord : coords)
		{
			Level level(u"terrain-oracle", Dimension::Id_Normal, seed);
			RandomLevelSource source(level, seed);
			const std::shared_ptr<LevelChunk> chunk = source.getChunk(coord[0], coord[1]);
			const headless::TerrainRun *runs = fixtures[fixture++];
			std::size_t index = 0;
			bool matched = true;
			while (index < chunk->blocks.size() && matched)
			{
				for (int i = 0; i < runs->count; ++i, ++index)
				{
					if (chunk->blocks[index] != runs->tile)
					{
						ctx.checkEqual(chunk->blocks[index], runs->tile, "seed " + std::to_string(seed) +
							" chunk " + std::to_string(coord[0]) + "," + std::to_string(coord[1]) +
							" first differing voxel " + std::to_string(index / 2048) + "," +
							std::to_string(index % 128) + "," + std::to_string((index / 128) % 16));
						matched = false;
						break;
					}
				}
				++runs;
			}
		}
}
