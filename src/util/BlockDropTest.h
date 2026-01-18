#pragma once

// Debug test harness for verifying Alpha 1.2.6 block drop behavior
// Enable with #define ENABLE_BLOCK_DROP_TEST before including this header

#ifdef ENABLE_BLOCK_DROP_TEST

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/entity/player/Player.h"
#include "java/Random.h"

namespace BlockDropTest
{
	// Test drop behavior for a single block type
	// Prints: block ID, drop ID, drop count, drop metadata (if applicable)
	void testBlockDrops(int_t blockId, int_t iterations = 100);
	
	// Test all implemented blocks (1, 2, 3, 12, 13, 17, 18)
	void testAllBlocks(int_t iterationsPerBlock = 100);
	
	// Test break speed for a block with different "tools" (hand = 1.0F, tools = TODO when implemented)
	void testBreakSpeed(int_t blockId);
}

#endif // ENABLE_BLOCK_DROP_TEST
