#pragma once

#include "java/Type.h"

class Level;
class Random;

// Alpha 1.2.6 WorldGenerator base class
// Reference: apclient/minecraft/src/net/minecraft/src/WorldGenerator.java
class WorldGenerator
{
public:
	virtual ~WorldGenerator() {}
	
	// Generate feature at position (x, y, z) in level using random
	// Returns true if feature was successfully placed
	virtual bool generate(Level &level, Random &random, int_t x, int_t y, int_t z) = 0;
	
	// Alpha: func_517_a (no-op in base class, used by trees for scale)
	virtual void func_517_a(double x, double y, double z) {}
};
