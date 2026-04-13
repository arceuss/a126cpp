#include "client/renderer/OffsettedRenderList.h"
#include "client/renderer/Chunk.h"

#include <iostream>

#include "OpenGL.h"

void OffsettedRenderList::init(int_t x, int_t y, int_t z, double xOff, double yOff, double zOff)
{
	inited = true;
	lists.clear();
	vboRefs.clear();
	this->x = x;
	this->y = y;
	this->z = z;

	this->xOff = xOff;
	this->yOff = yOff;
	this->zOff = zOff;
}

bool OffsettedRenderList::isAt(int_t x, int_t y, int_t z)
{
	if (!inited) return false;
	return this->x == x && this->y == y && this->z == z;
}

void OffsettedRenderList::add(int_t list)
{
	lists.push_back(list);
	if (lists.size() == 0x10000) render();
}

void OffsettedRenderList::addVBO(const ChunkVBOEntry *entry)
{
	if (entry == nullptr || entry->vboId == 0) return;
	VBORef ref;
	ref.vboId = entry->vboId;
	ref.vertexCount = entry->vertexCount;
	ref.hasTexture = entry->hasTexture;
	ref.hasColor = entry->hasColor;
	ref.hasNormal = entry->hasNormal;
	vboRefs.push_back(ref);
}

void OffsettedRenderList::render()
{
	if (!inited) return;
	if (!rendered)
		rendered = true;

	if (lists.empty() && vboRefs.empty()) return;

	glPushMatrix();
	glTranslatef(static_cast<float>(static_cast<double>(x) - xOff),
	             static_cast<float>(static_cast<double>(y) - yOff),
	             static_cast<float>(static_cast<double>(z) - zOff));

	// Render display-list based chunks (legacy/synchronous path)
	if (!lists.empty())
	{
		glCallLists(lists.size(), GL_UNSIGNED_INT, lists.data());
	}

	// Render VBO-based chunks (async meshed path)
	for (const auto &ref : vboRefs)
	{
		glBindBuffer(GL_ARRAY_BUFFER, ref.vboId);

		glVertexPointer(3, GL_FLOAT, 32, reinterpret_cast<void *>(0));
		glEnableClientState(GL_VERTEX_ARRAY);

		if (ref.hasTexture)
		{
			glTexCoordPointer(2, GL_FLOAT, 32, reinterpret_cast<void *>(12));
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		if (ref.hasColor)
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, 32, reinterpret_cast<void *>(20));
			glEnableClientState(GL_COLOR_ARRAY);
		}

		if (ref.hasNormal)
		{
			glNormalPointer(GL_BYTE, 32, reinterpret_cast<void *>(24));
			glEnableClientState(GL_NORMAL_ARRAY);
		}

		glDrawArrays(GL_TRIANGLES, 0, ref.vertexCount);

		glDisableClientState(GL_VERTEX_ARRAY);
		if (ref.hasTexture)
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if (ref.hasColor)
			glDisableClientState(GL_COLOR_ARRAY);
		if (ref.hasNormal)
			glDisableClientState(GL_NORMAL_ARRAY);
	}

	if (!vboRefs.empty())
		glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPopMatrix();
}

void OffsettedRenderList::clear()
{
	inited = false;
	rendered = false;
	lists.clear();
	vboRefs.clear();
}
