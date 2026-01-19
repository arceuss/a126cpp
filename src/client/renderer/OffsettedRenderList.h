#pragma once

#include "client/MemoryTracker.h"

#include "java/Type.h"

class Chunk;  // Forward declaration

class OffsettedRenderList
{
private:
	int_t x = 0, y = 0, z = 0;
	std::vector<int_t> lists; // Legacy display lists
	std::vector<std::pair<std::shared_ptr<Chunk>, int_t>> chunkVBOs;  // Chunks with layer for VBO rendering
	double xOff = 0.0, yOff = 0.0, zOff = 0.0;  // Store as double to preserve precision at long distances
	bool inited = false;
	bool rendered = false;

public:
	OffsettedRenderList() { lists.reserve(0x10000); chunkVBOs.reserve(0x10000); }

	void init(int_t x, int_t y, int_t z, double xOff, double yOff, double zOff);

	bool isAt(int_t x, int_t y, int_t z);

	void add(int_t list);  // Legacy display list
	void addChunk(std::shared_ptr<Chunk> chunk, int_t layer);  // VBO rendering

	void render();
	void clear();
};
