#pragma once

#include "java/Type.h"

// newb12: TilePos - tile coordinates
// Reference: newb12/net/minecraft/world/level/TilePos.java
struct TilePos
{
	int_t x;
	int_t y;
	int_t z;

	TilePos(int_t x, int_t y, int_t z) : x(x), y(y), z(z) {}

	bool operator==(const TilePos &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

namespace std
{
	template <>
	struct hash<TilePos>
	{
		size_t operator()(const TilePos &k) const
		{
			return static_cast<size_t>(k.x * 8976890 + k.y * 981131 + k.z);
		}
	};
}
