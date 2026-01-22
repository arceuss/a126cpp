#pragma once

#include "java/Type.h"
#include <functional>

class Entity;

// newb12: ChunkPos - chunk coordinates
// Reference: newb12/net/minecraft/world/level/ChunkPos.java
struct ChunkPos
{
	int_t x;
	int_t z;

	ChunkPos(int_t x, int_t z) : x(x), z(z) {}

	bool operator==(const ChunkPos &other) const
	{
		return x == other.x && z == other.z;
	}

	double distanceToSqr(const Entity &entity);
};

namespace std
{
	template <>
	struct hash<ChunkPos>
	{
		size_t operator()(const ChunkPos &k) const
		{
			return static_cast<size_t>(k.x << 8 | k.z);
		}
	};
}