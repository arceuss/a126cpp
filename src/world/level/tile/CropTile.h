#pragma once

#include "world/level/tile/FlowerTile.h"

// Beta 1.2: CropTile.java - ID 59, texture 88, extends Bush
// Alpha 1.2.6: BlockCrops - ID 59
class CropTile : public FlowerTile
{
public:
	CropTile(int_t id, int_t tex);
	
	// Beta: CropTile overrides
	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random) override;
	virtual int_t getTexture(Facing face, int_t data) override;
	virtual Shape getRenderShape() override;
	virtual void destroy(Level &level, int_t x, int_t y, int_t z, int_t data) override;
	virtual int_t getResource(int_t data, Random &random) override;
	virtual int_t getResourceCount(Random &random) override;
	
	// Beta: CropTile.growCropsToMax() - sets crop to max growth stage
	void growCropsToMax(Level &level, int_t x, int_t y, int_t z);
	
protected:
	// Beta: CropTile.mayPlaceOn() - crops can only be placed on farmland
	// Override canThisPlantGrowOnThisBlockID instead (FlowerTile doesn't have mayPlaceOn)
	virtual bool canThisPlantGrowOnThisBlockID(int_t blockId) override;
	
private:
	// Beta: CropTile.getGrowthSpeed() - calculates growth speed based on farmland hydration
	float getGrowthSpeed(Level &level, int_t x, int_t y, int_t z);
};
