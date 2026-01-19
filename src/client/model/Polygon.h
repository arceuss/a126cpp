#pragma once

#include <array>

#include "client/model/Vertex.h"
#include "client/renderer/Tesselator.h"

// Renamed from Polygon to Poly to avoid Windows wingdi.h Polygon collision
class Poly
{
public:
	std::array<Vertex, 4> vertices;
	int_t vertexCount = 0;
	
private:
	bool flipNormalFlag = false;

public:
	Poly();
	Poly(std::array<Vertex, 4> &&vertices);
	Poly(std::array<Vertex, 4> &&vertices, int_t u0, int_t v0, int_t u1, int_t v1);
	Poly(std::array<Vertex, 4> &&vertices, float u0, float v0, float u1, float v1);

	void mirror();
	void render(Tesselator &t, float scale);
	
	Poly &flipNormal();
	bool shouldFlipNormal() const { return flipNormalFlag; }
};
