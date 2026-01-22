#include "client/particle/TerrainParticle.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/tile/Tile.h"
#include "client/renderer/Tesselator.h"

TerrainParticle::TerrainParticle(Level &level, double x, double y, double z, double xa, double ya, double za, Tile *tile)
	: Particle(level, x, y, z, xa, ya, za), tile(tile)
{
	// Beta: Set texture and gravity from tile (TerrainParticle.java:13-14)
	tex = tile->tex;
	gravity = tile->gravity;
	
	// Beta: Set base color (TerrainParticle.java:15)
	rCol = gCol = bCol = 0.6f;
	
	// Beta: Scale down size (TerrainParticle.java:16)
	size /= 2.0f;
}

TerrainParticle &TerrainParticle::init(int_t x, int_t y, int_t z)
{
	// Beta: Special case for grass - no color modification (TerrainParticle.java:20-21)
	// Compare by ID since tile is Tile* and Tile::grass is GrassTile&
	if (tile->id == 2)  // Tile::grass.id == 2
	{
		return *this;
	}
	
	// Beta: Get color from tile and modify particle color (TerrainParticle.java:23-26)
	int_t col = tile->getColor(level, x, y, z);
	rCol *= ((col >> 16) & 0xFF) / 255.0f;
	gCol *= ((col >> 8) & 0xFF) / 255.0f;
	bCol *= (col & 0xFF) / 255.0f;
	
	return *this;
}

int_t TerrainParticle::getParticleTexture() const
{
	return 1;  // Beta: TERRAIN_TEXTURE (TerrainParticle.java:33)
}

void TerrainParticle::render(Tesselator &t, float a, float xa, float ya, float za, float xa2, float za2)
{
	// Beta: Calculate texture coordinates with offset (TerrainParticle.java:38-41)
	float u0 = (tex % 16 + uo / 4.0f) / 16.0f;
	float u1 = u0 + 0.015609375f;
	float v0 = (tex / 16 + vo / 4.0f) / 16.0f;
	float v1 = v0 + 0.015609375f;
	
	float r = 0.1f * size;
	float x = (float)(xo + (this->x - xo) * a - xOff);
	float y = (float)(yo + (this->y - yo) * a - yOff);
	float z = (float)(zo + (this->z - zo) * a - zOff);
	
	float br = getBrightness(a);
	t.color(br * rCol, br * gCol, br * bCol);
	
	// Beta: Render quad (TerrainParticle.java:48-51)
	t.vertexUV(x - xa * r - xa2 * r, y - ya * r, z - za * r - za2 * r, u0, v1);
	t.vertexUV(x - xa * r + xa2 * r, y + ya * r, z - za * r + za2 * r, u0, v0);
	t.vertexUV(x + xa * r + xa2 * r, y + ya * r, z + za * r + za2 * r, u1, v0);
	t.vertexUV(x + xa * r - xa2 * r, y - ya * r, z + za * r - za2 * r, u1, v1);
}