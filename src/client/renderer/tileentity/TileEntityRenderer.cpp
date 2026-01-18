#include "client/renderer/tileentity/TileEntityRenderer.h"

#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "client/renderer/Textures.h"
#include "client/gui/Font.h"
#include "world/level/Level.h"

// Beta: TileEntityRenderer.bindTexture() (TileEntityRenderer.java:13-16)
void TileEntityRenderer::bindTexture(const jstring &resourceName)
{
	Textures *t = tileEntityRenderDispatcher->textures;
	if (t != nullptr)
	{
		t->bind(t->loadTexture(resourceName));
	}
}

// Beta: TileEntityRenderer.bindTexture() with backup (TileEntityRenderer.java:18-21)
void TileEntityRenderer::bindTexture(const jstring &urlTexture, const jstring &backupTexture)
{
	// TODO: Implement HTTP texture loading
	bindTexture(backupTexture);
}

// Beta: TileEntityRenderer.init() (TileEntityRenderer.java:27-29)
void TileEntityRenderer::init(TileEntityRenderDispatcher *tileEntityRenderDispatcher)
{
	this->tileEntityRenderDispatcher = tileEntityRenderDispatcher;
}

// Beta: TileEntityRenderer.getFont() (TileEntityRenderer.java:31-33)
Font *TileEntityRenderer::getFont()
{
	return tileEntityRenderDispatcher != nullptr ? tileEntityRenderDispatcher->getFont() : nullptr;
}

// Beta: TileEntityRenderer.getLevel() (TileEntityRenderer.java:23-25)
Level *TileEntityRenderer::getLevel()
{
	return tileEntityRenderDispatcher != nullptr ? tileEntityRenderDispatcher->level : nullptr;
}
