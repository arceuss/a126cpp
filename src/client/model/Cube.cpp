#include "client/model/Cube.h"

#include "client/MemoryTracker.h"
#include "client/renderer/Tesselator.h"
#include "OpenGL.h"
#include <vector>

Cube::Cube(int_t xTexOffs, int_t yTexOffs)
{
	this->xTexOffs = xTexOffs;
	this->yTexOffs = yTexOffs;
}

void Cube::setTexOffs(int_t xTexOffs, int_t yTexOffs)
{
	this->xTexOffs = xTexOffs;
	this->yTexOffs = yTexOffs;
}

void Cube::addBox(float x0, float y0, float z0, int_t w, int_t h, int_t d)
{
	addBox(x0, y0, z0, w, h, d, 0.0f);
}

void Cube::addBox(float x0, float y0, float z0, int_t w, int_t h, int_t d, float g)
{
	float x1 = x0 + w;
	float y1 = y0 + h;
	float z1 = z0 + d;

	x0 -= g;
	y0 -= g;
	z0 -= g;
	x1 += g;
	y1 += g;
	z1 += g;

	if (mirror)
	{
		float tmp = x1;
		x1 = x0;
		x0 = tmp;
	}

	Vertex u0 = Vertex(x0, y0, z0, 0.0f, 0.0f);
	Vertex u1 = Vertex(x1, y0, z0, 0.0f, 8.0f);
	Vertex u2 = Vertex(x1, y1, z0, 8.0f, 8.0f);
	Vertex u3 = Vertex(x0, y1, z0, 8.0f, 0.0f);

	Vertex l0 = Vertex(x0, y0, z1, 0.0f, 0.0f);
	Vertex l1 = Vertex(x1, y0, z1, 0.0f, 8.0f);
	Vertex l2 = Vertex(x1, y1, z1, 8.0f, 8.0f);
	Vertex l3 = Vertex(x0, y1, z1, 8.0f, 0.0f);

	polygons[0] = Poly(std::array<Vertex, 4>{ l1, u1, u2, l2 }, xTexOffs + d + w, yTexOffs + d, xTexOffs + d + w + d, yTexOffs + d + h);
	polygons[1] = Poly(std::array<Vertex, 4>{ u0, l0, l3, u3 }, xTexOffs + 0, yTexOffs + d, xTexOffs + d, yTexOffs + d + h);
	
	polygons[2] = Poly(std::array<Vertex, 4>{ l1, l0, u0, u1 }, xTexOffs + d, yTexOffs + 0, xTexOffs + d + w, yTexOffs + d);
	polygons[3] = Poly(std::array<Vertex, 4>{ u2, u3, l3, l2 }, xTexOffs + d + w, yTexOffs + 0, xTexOffs + d + w + w, yTexOffs + d);
	
	polygons[4] = Poly(std::array<Vertex, 4>{ u1, u0, u3, u2 }, xTexOffs + d, yTexOffs + d, xTexOffs + d + w, yTexOffs + d + h);
	polygons[5] = Poly(std::array<Vertex, 4>{ l0, l1, l2, l3 }, xTexOffs + d + w + d, yTexOffs + d, xTexOffs + d + w + d + w, yTexOffs + d + h);

	if (mirror)
	{
		for (auto &p : polygons)
			p.mirror();
	}
}

void Cube::addTexBox(float x0, float y0, float z0, int_t w, int_t h, int_t d, int_t tex)
{
	float x1 = x0 + w;
	float y1 = y0 + h;
	float z1 = z0 + d;

	Vertex u0 = Vertex(x0, y0, z0, 0.0f, 0.0f);
	Vertex u1 = Vertex(x1, y0, z0, 0.0f, 8.0f);
	Vertex u2 = Vertex(x1, y1, z0, 8.0f, 8.0f);
	Vertex u3 = Vertex(x0, y1, z0, 8.0f, 0.0f);

	Vertex l0 = Vertex(x0, y0, z1, 0.0f, 0.0f);
	Vertex l1 = Vertex(x1, y0, z1, 0.0f, 8.0f);
	Vertex l2 = Vertex(x1, y1, z1, 8.0f, 8.0f);
	Vertex l3 = Vertex(x0, y1, z1, 8.0f, 0.0f);

	float us = 0.25f;
	float vs = 0.25f;
	float _u0 = ((tex % 16) + 1.0f - us) / 16.0f;
	float v0 = ((tex / 16) + 1.0f - vs) / 16.0f;
	float _u1 = ((tex % 16) + us) / 16.0f;
	float v1 = ((tex / 16) + vs) / 16.0f;

	polygons[0] = Poly(std::array<Vertex, 4>{ l1, u1, u2, l2 }, _u0, v0, _u1, v1);
	polygons[1] = Poly(std::array<Vertex, 4>{ u0, l0, l3, u3 }, _u0, v0, _u1, v1);
	
	polygons[2] = Poly(std::array<Vertex, 4>{ l1, l0, u0, u1 }, _u0, v0, _u1, v1);
	polygons[3] = Poly(std::array<Vertex, 4>{ u2, u3, l3, l2 }, _u0, v0, _u1, v1);
	
	polygons[4] = Poly(std::array<Vertex, 4>{ u1, u0, u3, u2 }, _u0, v0, _u1, v1);
	polygons[5] = Poly(std::array<Vertex, 4>{ l0, l1, l2, l3 }, _u0, v0, _u1, v1);
}

void Cube::setPos(float x, float y, float z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

void Cube::render(float scale)
{
	if (neverRender) return;
	if (!visible) return;
	if (!compiled) compile(scale);
	
	if (cubeVAO == 0 || vertexCount == 0)
		return;

	// Bind VAO and VBO before rendering
	// In compatibility profile, VAOs store the vertex pointers but we need to ensure VBO is bound
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBindVertexArray(cubeVAO);
	
	// Re-enable client states (VAO stores pointers but not enabled state in compatibility profile)
	glEnableClientState(GL_VERTEX_ARRAY);
	if (hasTex)
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (hasCol)
		glEnableClientState(GL_COLOR_ARRAY);
	if (hasNorm)
		glEnableClientState(GL_NORMAL_ARRAY);
	
	// Note: VBO was built with TRIANGLE_MODE=true, so quads are converted to triangles
	// The vertexCount reflects the triangle count (6 vertices per original quad)
	if (xRot != 0.0f || yRot != 0.0f || zRot != 0.0f)
	{
		glPushMatrix();
		glTranslatef(x * scale, y * scale, z * scale);
		if (zRot != 0.0f) glRotatef(zRot * c, 0.0f, 0.0f, 1.0f);
		if (yRot != 0.0f) glRotatef(yRot * c, 0.0f, 1.0f, 0.0f);
		if (xRot != 0.0f) glRotatef(xRot * c, 1.0f, 0.0f, 0.0f);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
		glPopMatrix();
	}
	else if (x != 0.0f || y != 0.0f || z != 0.0f)
	{
		glPushMatrix();
		glTranslatef(x * scale, y * scale, z * scale);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
		glPopMatrix();
	}
	else
	{
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}
	
	// Disable client states and unbind VAO and VBO
	glDisableClientState(GL_VERTEX_ARRAY);
	if (hasTex)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (hasCol)
		glDisableClientState(GL_COLOR_ARRAY);
	if (hasNorm)
		glDisableClientState(GL_NORMAL_ARRAY);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Cube::translateTo(float scale)
{
	if (neverRender) return;
	if (!visible) return;
	if (!compiled) compile(scale);

	if (xRot != 0.0f || yRot != 0.0f || zRot != 0.0f)
	{
		glTranslatef(x * scale, y * scale, z * scale);
		if (zRot != 0.0f) glRotatef(zRot * c, 0.0f, 0.0f, 1.0f);
		if (yRot != 0.0f) glRotatef(yRot * c, 0.0f, 1.0f, 0.0f);
		if (xRot != 0.0f) glRotatef(xRot * c, 1.0f, 0.0f, 0.0f);
	}
	else if (x != 0.0f || y != 0.0f || z != 0.0f)
	{
		glTranslatef(x * scale, y * scale, z * scale);
	}
}

void Cube::compile(float scale)
{
	// Build VBO/IBO/VAO instead of display list using Tesselator (preserves normals and exact behavior)
	Tesselator &t = Tesselator::instance;
	
	// Reset tesselator and build geometry using existing Poly::render() logic
	// Poly::render() calls t.begin() and t.end() for each polygon, so we don't need to call it here
	// Always reset tesselator to ensure it's in a clean state
	t.reset();
	
	// Collect all polygon data into tesselator
	// Each Poly::render() calls t.begin() and t.end(), which will render immediately
	// Instead, we need to build the VBO without calling end() for each polygon
	// So we'll manually build the geometry
	t.begin();
	
	// Manually build geometry from polygons (similar to Poly::render() but without calling end())
	for (auto &p : polygons)
	{
		if (p.vertexCount == 0)
			continue;
		
		// Calculate normal (same as Poly::render())
		Vec3 *v0 = p.vertices[1].pos.vectorTo(p.vertices[0].pos);
		Vec3 *v1 = p.vertices[1].pos.vectorTo(p.vertices[2].pos);
		Vec3 *n = v1->cross(*v0)->normalize();
		
		// Set normal
		if (p.shouldFlipNormal())
			t.normal(-n->x, -n->y, -n->z);
		else
			t.normal(n->x, n->y, n->z);
		
		// Add vertices
		for (int_t i = 0; i < p.vertexCount; i++)
		{
			Vertex &v = p.vertices[i];
			t.vertexUV(v.pos.x * scale, v.pos.y * scale, v.pos.z * scale, v.u, v.v);
		}
	}
	
	// Build VBO from tesselator data (matches chunk rendering approach)
	cubeVBO = t.buildVBO(vertexCount, hasTex, hasCol, hasNorm);
	
	if (cubeVBO == 0 || vertexCount == 0)
	{
		compiled = true;
		return;
	}
	
	// Generate VAO for cube rendering (fixed-function pipeline)
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);
	
	// Bind VBO
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	
	// Setup fixed-function vertex attribute pointers (VAO stores these)
	char *vbo_base = reinterpret_cast<char *>(0);
	
	// Position: always enabled
	glVertexPointer(3, GL_FLOAT, 32, reinterpret_cast<GLvoid *>(vbo_base + 0));
	glEnableClientState(GL_VERTEX_ARRAY);
	
	if (hasTex)
	{
		glTexCoordPointer(2, GL_FLOAT, 32, reinterpret_cast<GLvoid *>(vbo_base + 12));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	
	if (hasCol)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, 32, reinterpret_cast<GLvoid *>(vbo_base + 20));
		glEnableClientState(GL_COLOR_ARRAY);
	}
	
	if (hasNorm)
	{
		glNormalPointer(GL_BYTE, 32, reinterpret_cast<GLvoid *>(vbo_base + 24));
		glEnableClientState(GL_NORMAL_ARRAY);
	}
	
	// Unbind VAO (vertex pointer configuration remains stored)
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	// Index count equals vertex count (GL_QUADS mode, 4 vertices per quad)
	indexCount = vertexCount;
	
	compiled = true;
}
