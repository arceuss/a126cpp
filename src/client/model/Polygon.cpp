#include "client/model/Polygon.h"
#include "client/renderer/Tesselator.h"
#include "OpenGL.h"
#include <cmath>

Poly::Poly()
{

}

Poly::Poly(std::array<Vertex, 4> &&vertices) : vertices(std::move(vertices))
{
	vertexCount = this->vertices.size();
}

Poly::Poly(std::array<Vertex, 4> &&vertices, int_t u0, int_t v0, int_t u1, int_t v1) : Poly(std::move(vertices))
{
	float us = 1.0F / 640.0F;
	float vs = 1.0F / 480.0F;
	this->vertices[0] = this->vertices[0].remap(u1 / 64.0f - us, v0 / 32.0f + vs);
	this->vertices[1] = this->vertices[1].remap(u0 / 64.0f + us, v0 / 32.0f + vs);
	this->vertices[2] = this->vertices[2].remap(u0 / 64.0f + us, v1 / 32.0f - vs);
	this->vertices[3] = this->vertices[3].remap(u1 / 64.0f - us, v1 / 32.0f - vs);
}

Poly::Poly(std::array<Vertex, 4> &&vertices, float u0, float v0, float u1, float v1) : Poly(std::move(vertices))
{
	this->vertices[0] = this->vertices[0].remap(u1, v0);
	this->vertices[1] = this->vertices[1].remap(u0, v0);
	this->vertices[2] = this->vertices[2].remap(u0, v1);
	this->vertices[3] = this->vertices[3].remap(u1, v1);
}

void Poly::mirror()
{
	for (int_t i = 0; i < vertices.size() / 2; i++)
		std::swap(vertices[i], vertices[vertices.size() - i - 1]);
}

void Poly::render(Tesselator &t, float scale)
{
	Vec3 *v0 = vertices[1].pos.vectorTo(vertices[0].pos);
	Vec3 *v1 = vertices[1].pos.vectorTo(vertices[2].pos);
	Vec3 *n = v1->cross(*v0)->normalize();

	t.begin();

	if (flipNormalFlag)
		t.normal(-n->x, -n->y, -n->z);
	else
		t.normal(n->x, n->y, n->z);

	for (int_t i = 0; i < vertices.size(); i++)
	{
		Vertex &v = vertices[i];
		t.vertexUV(v.pos.x * scale, v.pos.y * scale, v.pos.z * scale, v.u, v.v);
	}

	t.end();
}

// The same normal, vertex order and texture coordinates as render(), with the
// positions and normal taken through a caller's matrix so many polygons can
// share one Tesselator draw instead of one per model.
void Poly::emitTransformed(Tesselator &t, float scale, const ModelMatrix &transform)
{
	Vec3 *v0 = vertices[1].pos.vectorTo(vertices[0].pos);
	Vec3 *v1 = vertices[1].pos.vectorTo(vertices[2].pos);
	Vec3 *n = v1->cross(*v0)->normalize();

	float nx = static_cast<float>(flipNormalFlag ? -n->x : n->x);
	float ny = static_cast<float>(flipNormalFlag ? -n->y : n->y);
	float nz = static_cast<float>(flipNormalFlag ? -n->z : n->z);
	float tx = 0.0f, ty = 0.0f, tz = 0.0f;
	transform.transformNormal(nx, ny, nz, tx, ty, tz);
	const float length = std::sqrt(tx * tx + ty * ty + tz * tz);
	if (length > 0.0f)
	{
		tx /= length;
		ty /= length;
		tz /= length;
	}
	t.normal(tx, ty, tz);

	for (int_t i = 0; i < vertices.size(); i++)
	{
		Vertex &v = vertices[i];
		float x = 0.0f, y = 0.0f, z = 0.0f;
		transform.transformPoint(static_cast<float>(v.pos.x * scale),
			static_cast<float>(v.pos.y * scale), static_cast<float>(v.pos.z * scale), x, y, z);
		t.vertexUV(x, y, z, v.u, v.v);
	}
}

Poly &Poly::flipNormal()
{
	flipNormalFlag = true;
	return *this;
}
