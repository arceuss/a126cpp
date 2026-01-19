#include "client/renderer/OffsettedRenderList.h"

#include <iostream>

#include "OpenGL.h"
#include "client/renderer/Chunk.h"
#include "client/renderer/Tesselator.h"

void OffsettedRenderList::init(int_t x, int_t y, int_t z, double xOff, double yOff, double zOff)
{
	inited = true;
	// Clear vectors but keep capacity for faster reuse
	lists.clear();
	chunkVBOs.clear();
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

void OffsettedRenderList::addChunk(std::shared_ptr<Chunk> chunk, int_t layer)
{
	chunkVBOs.push_back(std::make_pair(chunk, layer));
	// Render when list gets large (same as legacy lists)
	if (chunkVBOs.size() == 0x10000) render();
}

void OffsettedRenderList::render()
{
	if (!inited) return;
	if (!rendered)
		rendered = true;

	glPushMatrix();
	// AlphaPlace fix: Subtract offset in double precision before casting to float
	// This prevents floating point precision loss at long distances
	glTranslatef(static_cast<float>(static_cast<double>(x) - xOff), 
	             static_cast<float>(static_cast<double>(y) - yOff), 
	             static_cast<float>(static_cast<double>(z) - zOff));
	
	// Render chunk VBOs (same order as they were added)
	for (const auto &chunkPair : chunkVBOs)
	{
		auto &chunk = chunkPair.first;
		int_t layer = chunkPair.second;
		
		GLuint vboId = 0;
		int_t vertexCount = 0;
		bool hasTex = false;
		bool hasCol = false;
		bool hasNorm = false;
		
		if (chunk->getVBO(layer, vboId, vertexCount, hasTex, hasCol, hasNorm))
		{
			// Apply chunk's render offset and transforms
			glPushMatrix();
			// Translate to chunk's render offset
			glTranslatef(static_cast<float>(chunk->xRenderOffs), 
			             static_cast<float>(chunk->yRenderOffs), 
			             static_cast<float>(chunk->zRenderOffs));
			
			// Apply scaling transform (same as original display list)
			float ss = 1.0000001f;
			int_t chunkSize = chunk->zs;  // Assume square chunks
			int_t chunkYSize = chunk->ys;
			glTranslatef(-chunkSize / 2.0f, -chunkYSize / 2.0f, -chunkSize / 2.0f);
			glScalef(ss, ss, ss);
			glTranslatef(chunkSize / 2.0f, chunkYSize / 2.0f, chunkSize / 2.0f);
			
			// Render the VBO
			Tesselator::renderVBO(vboId, vertexCount, hasTex, hasCol, hasNorm);
			
			glPopMatrix();
		}
	}
	
	// Render legacy display lists (for non-chunk rendering like sky/star/dark)
	if (!lists.empty())
	{
		glCallLists(lists.size(), GL_UNSIGNED_INT, lists.data());
	}
	
	glPopMatrix();
}

void OffsettedRenderList::clear()
{
	inited = false;
	rendered = false;
	// Optimized: Clear without deallocating (faster for frequent clears)
	lists.clear();
	chunkVBOs.clear();
	// Note: Vectors maintain capacity, so clear() is efficient
}
