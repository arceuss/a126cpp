#include "client/renderer/tileentity/SignRenderer.h"

#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "client/renderer/Textures.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "client/gui/Font.h"
#include "OpenGL.h"

SignRenderer::SignRenderer()
{
}

void SignRenderer::render(SignTileEntity &sign, double x, double y, double z, float a)
{
	if (sign.level == nullptr)
		return;

	// Cache the sign texture ID on first use (avoids per-sign string hash + map lookup)
	if (signTextureId < 0)
	{
		Textures *t = tileEntityRenderDispatcher->textures;
		if (t != nullptr)
			signTextureId = t->loadTexture(u"/item/sign.png");
	}

	// Get tile type from the sign's cached data instead of querying the world each frame.
	// sign.getData() is already stored on the tile entity -- we just need to know if it's
	// a standing sign or wall sign, which we determine from the tile ID.
	// Still need to query the tile ID, but skip hasChunkAt -- tile entities are only in
	// the render list while their chunk is loaded.
	int_t tileId = sign.level->getTile(sign.x, sign.y, sign.z);

	if (tileId < 0 || tileId >= 256)
		return;

	Tile *tilePtr = Tile::tiles[tileId];
	if (tilePtr == nullptr)
		return;

	Tile &tile = *tilePtr;
	glPushMatrix();
	float size = 0.6666667f;

	if (tile.id == Tile::sign.id)
	{
		glTranslatef((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);
		float rot = sign.getData() * 360 / 16.0f;
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);
		signModel.cube2.visible = true;
	}
	else
	{
		int_t face = sign.getData();
		float rot = 0.0f;

		if (face == 2) rot = 180.0f;
		if (face == 4) rot = 90.0f;
		if (face == 5) rot = -90.0f;

		glTranslatef((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, -0.3125f, -0.4375f);
		signModel.cube2.visible = false;
	}

	bindTextureId(signTextureId);
	glPushMatrix();
	glScalef(size, -size, -size);
	signModel.render();
	glPopMatrix();

	Font *font = getFont();
	float s = 0.016666668f * size;
	glTranslatef(0.0f, 0.5f * size, 0.07f * size);
	glScalef(s, -s, s);
	glNormal3f(0.0f, 0.0f, -1.0f * s);
	glDepthMask(false);
	int_t col = 0;
	sign.renderCachedText(*font, col);

	glDepthMask(true);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPopMatrix();
}

void SignRenderer::renderEntity(TileEntity *entity, double x, double y, double z, float a)
{
	// Safe: dispatch already confirmed the type via typeid registry
	render(static_cast<SignTileEntity &>(*entity), x, y, z, a);
}
