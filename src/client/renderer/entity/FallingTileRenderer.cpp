#include "client/renderer/entity/FallingTileRenderer.h"

#include "world/entity/item/FallingTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "util/Mth.h"
#include "pc/OpenGL.h"

FallingTileRenderer::FallingTileRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
	shadowRadius = 0.5f;
}

void FallingTileRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	// newb12: FallingTileRenderer.render() (FallingTileRenderer.java:17-27)
	FallingTile &tile = static_cast<FallingTile &>(entity);
	
	glPushMatrix();  // newb12: GL11.glPushMatrix() (FallingTileRenderer.java:18)
	glTranslatef((float)x, (float)y, (float)z);  // newb12: GL11.glTranslatef((float)x, (float)y, (float)z) (FallingTileRenderer.java:19)
	bindTexture(u"/terrain.png");  // newb12: this.bindTexture("/terrain.png") (FallingTileRenderer.java:20)
	Tile *tt = Tile::tiles[tile.tile];  // newb12: Tile tt = Tile.tiles[tile.tile] (FallingTileRenderer.java:21)
	Level &level = tile.getLevel();  // newb12: Level level = tile.getLevel() (FallingTileRenderer.java:22)
	glDisable(GL_LIGHTING);  // newb12: GL11.glDisable(2896) (GL_LIGHTING) (FallingTileRenderer.java:23)
	tileRenderer.renderBlock(*tt, level, Mth::floor(tile.x), Mth::floor(tile.y), Mth::floor(tile.z));  // newb12: this.tileRenderer.renderBlock(tt, level, Mth.floor(tile.x), Mth.floor(tile.y), Mth.floor(tile.z)) (FallingTileRenderer.java:24)
	glEnable(GL_LIGHTING);  // newb12: GL11.glEnable(2896) (GL_LIGHTING) (FallingTileRenderer.java:25)
	glPopMatrix();  // newb12: GL11.glPopMatrix() (FallingTileRenderer.java:26)
}
