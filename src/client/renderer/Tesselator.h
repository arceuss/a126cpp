#pragma once

#include <memory>

#include "java/Type.h"

#include "util/Memory.h"

struct ChunkMeshData
{
	std::unique_ptr<char[]> vertexData;
	int_t vertexCount = 0;
	int_t dataSize = 0;
	bool hasTexture = false;
	bool hasColor = false;
	bool hasNormal = false;
	bool empty = true;

	void appendFrom(const char *src, int_t srcSize, int_t srcVertices, bool tex, bool col, bool norm);
};

class Tesselator
{
private:
	static constexpr bool TRIANGLE_MODE = true;
	static constexpr int_t MAX_MEMORY_USE = 0x1000000;
	static constexpr int_t MAX_FLOATS = 0x200000;

	// Buffer
	std::unique_ptr<char[]> buffer;
	char *buffer_p;
	char *buffer_e;

	// Tesselation state
	int_t vertices = 0;

	double u = 0.0, v = 0.0;
	int_t col = 0;

	bool hasColor = false;
	bool hasTexture = false;
	bool hasNormal = false;

	int_t count = 0;
	bool noColorFlag = false;

	unsigned int draw_mode = 0;

	double xo = 0.0;
	double yo = 0.0;
	double zo = 0.0;

	int_t normalValue = 0;

public:
	static Tesselator instance;

	// Thread-local instance pointer (LCE pattern: Tesselator::getInstance() via TLS)
	// Main thread sets this to &instance. Worker threads set it to their own offline Tesselator.
	static Tesselator &getInstance();
	static void setThreadInstance(Tesselator *tess);

private:
	// State
	bool tesselating = false;
	bool vboMode = false;

	// VBO state
	std::unique_ptr<unsigned int[]> vboIds;
	int_t vboId = 0;
	int_t vboCounts = 10;

	// Buffer state
	int_t size = 0;

	// Offline mode (for off-thread chunk meshing)
	bool offlineMode = false;
	ChunkMeshData *outputTarget = nullptr;

public:
	Tesselator(int_t size);
	Tesselator(int_t size, bool offline);

	Tesselator getUniqueInstance(int_t size);

	void setOutputTarget(ChunkMeshData *target);

	// Tessellator functions
	void end();
	void clear();
	void begin();
	void begin(unsigned int mode);
	void tex(double u, double v);
	void color(float r, float g, float b);
	void color(float r, float g, float b, float a);
	void color(int_t r, int_t g, int_t b);
	void color(int_t r, int_t g, int_t b, int_t a);
	void vertexUV(double x, double y, double z, double u, double v);
	void vertex(double x, double y, double z);
	void color(int_t rgb);
	void color(int_t rgb, int_t a);
	void noColor();
	void normal(float x, float y, float z);
	void offset(double x, double y, double z);
	void addOffset(float x, float y, float z);

	bool isOffline() const { return offlineMode; }
};
