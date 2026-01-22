#pragma once

#include "world/level/dimension/Dimension.h"

// Beta 1.2: HellDimension.java - Nether dimension
// Java: net.minecraft.world.level.dimension.HellDimension
class HellDimension : public Dimension
{
public:
	HellDimension(Level &level);
	
	virtual ~HellDimension() {}
	
protected:
	virtual void init() override;
	virtual void updateLightRamp() override;
	
public:
	virtual ChunkSource *createRandomLevelSource() override;
	virtual ChunkStorage *createStorage(std::shared_ptr<File> dir) override;
	virtual bool isValidSpawn(int_t x, int_t z) override;
	virtual float getTimeOfDay(int_t time, float add) override;
	virtual Vec3 *getFogColor(float time, float unknown) override;
	virtual bool mayRespawn() override;
};
