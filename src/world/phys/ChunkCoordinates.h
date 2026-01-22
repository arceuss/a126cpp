#pragma once

#include "java/Type.h"
#include <functional>

// Alpha 1.2.6 ChunkCoordinates - simple 3D integer coordinates
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/ChunkCoordinates.java
struct ChunkCoordinates
{
	int x;
	int y;
	int z;
	
	ChunkCoordinates() : x(0), y(0), z(0) {}
	ChunkCoordinates(int x, int y, int z) : x(x), y(y), z(z) {}
	ChunkCoordinates(const ChunkCoordinates& other) : x(other.x), y(other.y), z(other.z) {}
	
	bool operator==(const ChunkCoordinates& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
	
	bool operator!=(const ChunkCoordinates& other) const
	{
		return !(*this == other);
	}
	
	// Comparison operator for std::set (matches Java compareTo)
	bool operator<(const ChunkCoordinates& other) const
	{
		if (y != other.y) return y < other.y;
		if (z != other.z) return z < other.z;
		return x < other.x;
	}
};

// Hash function for ChunkCoordinates (for std::unordered_map)
namespace std {
	template<>
	struct hash<ChunkCoordinates> {
		size_t operator()(const ChunkCoordinates& cc) const {
			// Java: return this.x + this.z << 8 + this.y << 16;
			// Note: Java's << has lower precedence, so it's: x + ((z << 8) + (y << 16))
			return cc.x + (cc.z << 8) + (cc.y << 16);
		}
	};
}
