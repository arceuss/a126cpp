#include "client/renderer/OffsettedRenderList.h"

#include <iostream>

#include "OpenGL.h"
#include "client/renderer/Chunk.h"

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
	// VBO rendering removed - this method is now unused
	// Keep empty to maintain interface compatibility
	(void)chunk;
	(void)layer;
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
	
	// Render legacy display lists
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
