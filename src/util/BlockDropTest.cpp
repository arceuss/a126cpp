#include "BlockDropTest.h"

#ifdef ENABLE_BLOCK_DROP_TEST

#include <iostream>
#include <iomanip>
#include <map>

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/entity/player/Player.h"
#include "java/Random.h"

namespace BlockDropTest
{

void testBlockDrops(int_t blockId, int_t iterations)
{
	if (blockId < 0 || blockId > 255 || Tile::tiles[blockId] == nullptr)
	{
		std::cout << "Block ID " << blockId << " not implemented\n";
		return;
	}
	
	Tile *tile = Tile::tiles[blockId];
	Random random;
	
	std::map<int_t, int_t> dropCounts;
	int_t totalDrops = 0;
	int_t zeroDrops = 0;
	
	std::cout << "\n=== Testing Block ID " << blockId << " ===\n";
	std::cout << "Iterations: " << iterations << "\n";
	std::cout << "Hardness: " << tile->destroySpeed << "\n";
	
	for (int_t i = 0; i < iterations; ++i)
	{
		int_t dropCount = tile->getResourceCount(random);
		if (dropCount == 0)
		{
			zeroDrops++;
			continue;
		}
		
		int_t dropId = tile->getResource(0, random);
		dropCounts[dropId] += dropCount;
		totalDrops += dropCount;
	}
	
	std::cout << "Total drops spawned: " << totalDrops << "\n";
	std::cout << "Zero-drop iterations: " << zeroDrops << " (" << (zeroDrops * 100.0f / iterations) << "%)\n";
	std::cout << "Drop ID distribution:\n";
	for (const auto &pair : dropCounts)
	{
		float percentage = (pair.second * 100.0f) / iterations;
		std::cout << "  ID " << std::setw(3) << pair.first << ": " << std::setw(4) << pair.second 
		          << " times (" << std::fixed << std::setprecision(2) << percentage << "%)\n";
	}
	
	// Verify against Alpha expectations
	if (blockId == 1) // stone
		std::cout << "  EXPECTED: ID 4 (cobblestone) - " << (dropCounts[4] == iterations ? "✓" : "✗ MISSING") << "\n";
	else if (blockId == 2) // grass
		std::cout << "  EXPECTED: ID 3 (dirt) - " << (dropCounts[3] == iterations ? "✓" : "✗") << "\n";
	else if (blockId == 13) // gravel
	{
		int_t flintCount = dropCounts[318]; // flint item ID
		int_t selfCount = dropCounts[13];
		float flintPercent = (flintCount * 100.0f) / iterations;
		std::cout << "  EXPECTED: ~10% ID 318 (flint), ~90% ID 13 (self)\n";
		std::cout << "  ACTUAL: " << std::fixed << std::setprecision(1) << flintPercent << "% flint - " 
		          << (flintPercent >= 5.0f && flintPercent <= 15.0f ? "✓" : "✗") << "\n";
	}
	else if (blockId == 18) // leaves
	{
		int_t saplingCount = dropCounts[6]; // sapling block ID
		float saplingPercent = (saplingCount * 100.0f) / iterations;
		std::cout << "  EXPECTED: ~5% ID 6 (sapling), ~95% nothing\n";
		std::cout << "  ACTUAL: " << std::fixed << std::setprecision(1) << saplingPercent << "% sapling - " 
		          << (saplingPercent >= 3.0f && saplingPercent <= 7.0f ? "✓" : "✗") << "\n";
	}
}

void testAllBlocks(int_t iterationsPerBlock)
{
	std::cout << "\n========================================";
	std::cout << "\n  BLOCK DROP TEST HARNESS";
	std::cout << "\n  Alpha 1.2.6 Compliance Check";
	std::cout << "\n========================================\n";
	
	int_t implementedBlocks[] = {1, 2, 3, 12, 13, 17, 18};
	
	for (int_t blockId : implementedBlocks)
	{
		testBlockDrops(blockId, iterationsPerBlock);
	}
	
	std::cout << "\n========================================\n";
}

void testBreakSpeed(int_t blockId)
{
	if (blockId < 0 || blockId > 255 || Tile::tiles[blockId] == nullptr)
	{
		std::cout << "Block ID " << blockId << " not implemented\n";
		return;
	}
	
	Tile *tile = Tile::tiles[blockId];
	
	// TODO: Create a mock player with different tools
	// For now, just print block properties
	// Note: destroySpeed is protected, so we can't directly access it here
	// This test function will work once tool system is implemented and we can call getDestroyProgress()
	std::cout << "\n=== Break Speed Test: Block ID " << blockId << " ===\n";
	std::cout << "Material: " << (tile->material.isFlammable() ? "flammable" : "non-flammable") << "\n";
	std::cout << "\nNote: Hardness/resistance are protected members. Break speed testing\n";
	std::cout << "requires tool system implementation and Player::getDestroySpeed() access.\n";
	std::cout << "See ALPHA_AUDIT.md for expected break speed formulas.\n";
}

} // namespace BlockDropTest

#endif // ENABLE_BLOCK_DROP_TEST
