#pragma once

#include <memory>

#include "java/String.h"

class TileEntity;
class TileEntityRenderDispatcher;
class Textures;
class Font;
class Level;

// Beta 1.2: TileEntityRenderer.java
class TileEntityRenderer
{
protected:
	TileEntityRenderDispatcher *tileEntityRenderDispatcher = nullptr;

public:
	virtual ~TileEntityRenderer() {}

	// Beta: TileEntityRenderer.bindTexture() - binds texture (TileEntityRenderer.java:13-16)
	void bindTexture(const jstring &resourceName);

	// Beta: TileEntityRenderer.bindTexture() - binds texture with backup (TileEntityRenderer.java:18-21)
	void bindTexture(const jstring &urlTexture, const jstring &backupTexture);

	// Bind a texture by cached ID (skip string hash lookup)
	void bindTextureId(int_t id);

	// Beta: TileEntityRenderer.init() - initializes renderer (TileEntityRenderer.java:27-29)
	void init(TileEntityRenderDispatcher *tileEntityRenderDispatcher);

	// Beta: TileEntityRenderer.getFont() - gets font (TileEntityRenderer.java:31-33)
	Font *getFont();

	// Beta: TileEntityRenderer.getLevel() - gets level (TileEntityRenderer.java:23-25)
	Level *getLevel();

	// Render a tile entity at the given position. Override in subclasses.
	virtual void renderEntity(TileEntity *entity, double x, double y, double z, float a) = 0;
};
