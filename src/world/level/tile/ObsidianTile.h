#pragma once

#include "world/level/tile/StoneTile.h"

#include "java/Type.h"

// Alpha 1.2.6 Block.obsidian (Block.java:71)
// obsidian = (new BlockObsidian(49, 37)).setHardness(10.0F).setResistance(2000.0F).setStepSound(soundStoneFootstep)
// BlockObsidian extends BlockStone and drops itself (BlockObsidian.java:10-16)
class ObsidianTile : public StoneTile
{
public:
	ObsidianTile(int_t id, int_t texture);
	int_t getResource(int_t data, Random &random) override;
};
