#pragma once

#include "world/level/levelgen/feature/Feature.h"
#include "java/Random.h"
#include "java/Type.h"
#include <vector>
#include <array>

class Level;

// Beta 1.2 BasicTree (big tree feature)
// Reference: deobfb12/minecraft/src/net/minecraft/world/level/levelgen/feature/BasicTree.java
class WorldGenBigTree : public Feature
{
private:
	static constexpr byte_t axisConversionArray[6] = {2, 0, 0, 1, 2, 1};
	Random rnd;
	Level *thisLevel;
	int_t origin[3] = {0, 0, 0};
	int_t height = 0;
	int_t trunkHeight;
	double trunkHeightScale = 0.618;
	double branchDensity = 1.0;
	double branchSlope = 0.381;
	double widthScale = 1.0;
	double foliageDensity = 1.0;
	int_t trunkWidth = 1;
	int_t heightVariance = 12;
	int_t foliageHeight = 4;
	std::vector<std::array<int_t, 4>> foliageCoords;

	void prepare();
	void crossection(int_t x, int_t y, int_t z, float radius, byte_t axis, int_t blockId);
	float treeShape(int_t offset);
	float foliageShape(int_t offset);
	void foliageCluster(int_t x, int_t y, int_t z);
	void limb(int_t *start, int_t *end, int_t blockId);
	void makeFoliage();
	bool trimBranches(int_t offset);
	void makeTrunk();
	void makeBranches();
	int_t checkLine(int_t *start, int_t *end);
	bool checkLocation();

public:
	WorldGenBigTree();
	
	virtual void init(double scale, double branchDensity, double foliageDensity) override;
	virtual bool place(Level &level, Random &random, int_t x, int_t y, int_t z) override;
};
