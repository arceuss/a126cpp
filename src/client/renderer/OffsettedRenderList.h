#pragma once

#include <vector>

#include "client/MemoryTracker.h"

#include "java/Type.h"

struct ChunkVBOEntry;

class OffsettedRenderList
{
private:
	int_t x = 0, y = 0, z = 0;
	std::vector<int_t> lists;
	double xOff = 0.0, yOff = 0.0, zOff = 0.0;
	bool inited = false;
	bool rendered = false;

	// VBO entries for async-meshed chunks
	struct VBORef
	{
		unsigned int vboId;
		int_t vertexCount;
		bool hasTexture, hasColor, hasNormal;
	};
	std::vector<VBORef> vboRefs;

public:
	OffsettedRenderList() { lists.reserve(0x10000); vboRefs.reserve(256); }

	void init(int_t x, int_t y, int_t z, double xOff, double yOff, double zOff);

	bool isAt(int_t x, int_t y, int_t z);

	void add(int_t list);
	void addVBO(const ChunkVBOEntry *entry);

	void render();
	void clear();
};
