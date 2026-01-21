#include "client/renderer/TileRenderer.h"

#include "client/renderer/Tesselator.h"
#include "world/level/tile/FluidTile.h"
#include "world/level/tile/DoorTile.h"
#include "world/level/tile/RedStoneDustTile.h"
#include "world/level/tile/FireTile.h"
#include "world/level/tile/CobblestoneTile.h"
#include "world/level/material/Material.h"
#include "world/level/Level.h"
#include "world/phys/Vec3.h"
#include "util/Mth.h"
#include "Facing.h"

TileRenderer::TileRenderer(LevelSource *levelSource) : level(levelSource)
{

}

TileRenderer::TileRenderer()
{

}

void TileRenderer::tesselateInWorld(Tile &tile, int_t x, int_t y, int_t z, int_t fixedTexture)
{
	this->fixedTexture = fixedTexture;
	tesselateInWorld(tile, x, y, z);
	this->fixedTexture = -1;
}

void TileRenderer::tesselateInWorldNoCulling(Tile &tile, int_t x, int_t y, int_t z)
{
	noCulling = true;
	tesselateInWorld(tile, x, y, z);
	noCulling = false;
}

bool TileRenderer::tesselateInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	int_t shape = tt.getRenderShape();
	tt.updateShape(*level, x, y, z);

	if (shape == Tile::SHAPE_BLOCK)
		return tesselateBlockInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_WATER)
		return tesselateWaterInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_CROSS_TEXTURE)
		return tesselateCrossTextureInWorld(tt, x, y, z);
	
	// Alpha: renderBlockCactus() calls func_1230_b (custom model with shrunk sides) (RenderBlocks.java:1175-1181)
	// Beta: Cactus uses render type 13 with custom model matching collision box
	if (shape == Tile::SHAPE_CACTUS)
		return tesselateCactusInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_TORCH)
		return tesselateTorchInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_DOOR)
		return tesselateDoorInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_LADDER)
		return tesselateLadderInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_STAIRS)
		return tesselateStairsInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_FIRE)
		return tesselateFireInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_RED_DUST)
		return tesselateDustInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_ROWS)
		return tesselateRowInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_RAIL)
		return tesselateRailInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_FENCE)
		return tesselateFenceInWorld(tt, x, y, z);
	
	if (shape == Tile::SHAPE_LEVER)
		return tesselateLeverInWorld(tt, x, y, z);
	
	return false;
}

bool TileRenderer::tesselateBlockInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	int_t col = tt.getColor(*level, x, y, z);
	float r = ((col >> 16) & 0xFF) / 255.0f;
	float g = ((col >> 8) & 0xFF) / 255.0f;
	float b = (col & 0xFF) / 255.0f;
	return tesselateBlockInWorld(tt, x, y, z, r, g, b);
}

bool TileRenderer::tesselateBlockInWorld(Tile &tt, int_t x, int_t y, int_t z, float r, float g, float b)
{
	Tesselator &t = Tesselator::instance;

	bool changed = false;
	float c10 = 0.5f;
	float c11 = 1.0f;
	float c2 = 0.8f;
	float c3 = 0.6f;

	float r11 = c11 * r;
	float g11 = c11 * g;
	float b11 = c11 * b;

	if (&tt == reinterpret_cast<Tile*>(&Tile::grass))
	{
		b = 1.0f;
		g = 1.0f;
		r = 1.0f;
	}

	float r10 = c10 * r;
	float r2 = c2 * r;
	float r3 = c3 * r;

	float g10 = c10 * g;
	float g2 = c2 * g;
	float g3 = c3 * g;

	float b10 = c10 * b;
	float b2 = c2 * b;
	float b3 = c3 * b;

	float centerBrightness = tt.getBrightness(*level, x, y, z);

	if (noCulling || tt.shouldRenderFace(*level, x, y - 1, z, Facing::DOWN))
	{
		float br = tt.getBrightness(*level, x, y - 1, z);

		t.color(r10 * br, g10 * br, b10 * br);
		renderFaceUp(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::DOWN));
		changed = true;
	}

	if (noCulling || tt.shouldRenderFace(*level, x, y + 1, z, Facing::UP))
	{
		float br = tt.getBrightness(*level, x, y + 1, z);
		if (tt.yy1 != 1.0 && !tt.material.isLiquid()) br = centerBrightness;

		t.color(r11 * br, g11 * br, b11 * br);
		renderFaceDown(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::UP));
		changed = true;
	}

	if (noCulling || tt.shouldRenderFace(*level, x, y, z - 1, Facing::NORTH))
	{
		float br = tt.getBrightness(*level, x, y, z - 1);
		if (tt.zz0 > 0.0) br = centerBrightness;

		t.color(r2 * br, g2 * br, b2 * br);
		renderNorth(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::NORTH));
		changed = true;
	}

	if (noCulling || tt.shouldRenderFace(*level, x, y, z + 1, Facing::SOUTH))
	{
		float br = tt.getBrightness(*level, x, y, z + 1);
		if (tt.zz1 < 1.0) br = centerBrightness;

		t.color(r2 * br, g2 * br, b2 * br);
		renderSouth(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::SOUTH));
		changed = true;
	}

	if (noCulling || tt.shouldRenderFace(*level, x - 1, y, z, Facing::WEST))
	{
		float br = tt.getBrightness(*level, x - 1, y, z);
		if (tt.xx0 > 0.0) br = centerBrightness;

		t.color(r3 * br, g3 * br, b3 * br);
		renderWest(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::WEST));
		changed = true;
	}

	if (noCulling || tt.shouldRenderFace(*level, x + 1, y, z, Facing::EAST))
	{
		float br = tt.getBrightness(*level, x + 1, y, z);
		if (tt.xx1 < 1.0) br = centerBrightness;

		t.color(r3 * br, g3 * br, b3 * br);
		renderEast(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::EAST));
		changed = true;
	}

	return changed;
}

// newb12: TileRenderer.renderBlock() - renders block at -0.5 offset (for falling blocks)
// Reference: newb12/net/minecraft/client/renderer/TileRenderer.java:1045-1096
void TileRenderer::renderBlock(Tile &tt, Level &level, int_t x, int_t y, int_t z)
{
	float c10 = 0.5f;  // newb12: float c10 = 0.5F (TileRenderer.java:1046)
	float c11 = 1.0f;  // newb12: float c11 = 1.0F (TileRenderer.java:1047)
	float c2 = 0.8f;  // newb12: float c2 = 0.8F (TileRenderer.java:1048)
	float c3 = 0.6f;  // newb12: float c3 = 0.6F (TileRenderer.java:1049)
	Tesselator &t = Tesselator::instance;  // newb12: Tesselator t = Tesselator.instance (TileRenderer.java:1050)
	t.begin();  // newb12: t.begin() (TileRenderer.java:1051)
	
	float center = tt.getBrightness(level, x, y, z);  // newb12: float center = tt.getBrightness(level, x, y, z) (TileRenderer.java:1052)
	float br = tt.getBrightness(level, x, y - 1, z);  // newb12: float br = tt.getBrightness(level, x, y - 1, z) (TileRenderer.java:1053)
	if (br < center)  // newb12: if (br < center) (TileRenderer.java:1054)
		br = center;  // newb12: br = center (TileRenderer.java:1055)
	
	t.color(c10 * br, c10 * br, c10 * br);  // newb12: t.color(c10 * br, c10 * br, c10 * br) (TileRenderer.java:1058)
	renderFaceUp(tt, -0.5, -0.5, -0.5, tt.getTexture(Facing::DOWN));  // newb12: this.renderFaceUp(tt, -0.5, -0.5, -0.5, tt.getTexture(0)) (TileRenderer.java:1059)
	
	br = tt.getBrightness(level, x, y + 1, z);  // newb12: br = tt.getBrightness(level, x, y + 1, z) (TileRenderer.java:1060)
	if (br < center)  // newb12: if (br < center) (TileRenderer.java:1061)
		br = center;  // newb12: br = center (TileRenderer.java:1062)
	
	t.color(c11 * br, c11 * br, c11 * br);  // newb12: t.color(c11 * br, c11 * br, c11 * br) (TileRenderer.java:1065)
	renderFaceDown(tt, -0.5, -0.5, -0.5, tt.getTexture(Facing::UP));  // newb12: this.renderFaceDown(tt, -0.5, -0.5, -0.5, tt.getTexture(1)) (TileRenderer.java:1066)
	
	br = tt.getBrightness(level, x, y, z - 1);  // newb12: br = tt.getBrightness(level, x, y, z - 1) (TileRenderer.java:1067)
	if (br < center)  // newb12: if (br < center) (TileRenderer.java:1068)
		br = center;  // newb12: br = center (TileRenderer.java:1069)
	
	t.color(c2 * br, c2 * br, c2 * br);  // newb12: t.color(c2 * br, c2 * br, c2 * br) (TileRenderer.java:1072)
	renderNorth(tt, -0.5, -0.5, -0.5, tt.getTexture(Facing::NORTH));  // newb12: this.renderNorth(tt, -0.5, -0.5, -0.5, tt.getTexture(2)) (TileRenderer.java:1073)
	
	br = tt.getBrightness(level, x, y, z + 1);  // newb12: br = tt.getBrightness(level, x, y, z + 1) (TileRenderer.java:1074)
	if (br < center)  // newb12: if (br < center) (TileRenderer.java:1075)
		br = center;  // newb12: br = center (TileRenderer.java:1076)
	
	t.color(c2 * br, c2 * br, c2 * br);  // newb12: t.color(c2 * br, c2 * br, c2 * br) (TileRenderer.java:1079)
	renderSouth(tt, -0.5, -0.5, -0.5, tt.getTexture(Facing::SOUTH));  // newb12: this.renderSouth(tt, -0.5, -0.5, -0.5, tt.getTexture(3)) (TileRenderer.java:1080)
	
	br = tt.getBrightness(level, x - 1, y, z);  // newb12: br = tt.getBrightness(level, x - 1, y, z) (TileRenderer.java:1081)
	if (br < center)  // newb12: if (br < center) (TileRenderer.java:1082)
		br = center;  // newb12: br = center (TileRenderer.java:1083)
	
	t.color(c3 * br, c3 * br, c3 * br);  // newb12: t.color(c3 * br, c3 * br, c3 * br) (TileRenderer.java:1086)
	renderWest(tt, -0.5, -0.5, -0.5, tt.getTexture(Facing::WEST));  // newb12: this.renderWest(tt, -0.5, -0.5, -0.5, tt.getTexture(4)) (TileRenderer.java:1087)
	
	br = tt.getBrightness(level, x + 1, y, z);  // newb12: br = tt.getBrightness(level, x + 1, y, z) (TileRenderer.java:1088)
	if (br < center)  // newb12: if (br < center) (TileRenderer.java:1089)
		br = center;  // newb12: br = center (TileRenderer.java:1090)
	
	t.color(c3 * br, c3 * br, c3 * br);  // newb12: t.color(c3 * br, c3 * br, c3 * br) (TileRenderer.java:1093)
	renderEast(tt, -0.5, -0.5, -0.5, tt.getTexture(Facing::EAST));  // newb12: this.renderEast(tt, -0.5, -0.5, -0.5, tt.getTexture(5)) (TileRenderer.java:1094)
	t.end();  // newb12: t.end() (TileRenderer.java:1095)
}

void TileRenderer::renderFaceUp(Tile &tt, double x, double y, double z, int_t tex)
{
	Tesselator &t = Tesselator::instance;

	if (fixedTexture >= 0) tex = fixedTexture;

	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;

	double u0 = (xt + tt.xx0 * 16.0) / 256.0;
	double u1 = (xt + tt.xx1 * 16.0 - 0.01) / 256.0;
	double v0 = (yt + tt.zz0 * 16.0) / 256.0;
	double v1 = (yt + tt.zz1 * 16.0 - 0.01) / 256.0;

	if (tt.xx0 < 0.0 || tt.xx1 > 1.0)
	{
		u0 = ((xt + 0.0f) / 256.0f);
		u1 = ((xt + 15.99f) / 256.0f);
	}

	if (tt.zz0 < 0.0 || tt.zz1 > 1.0)
	{
		v0 = ((yt + 0.0f) / 256.0f);
		v1 = ((yt + 15.99f) / 256.0f);
	}

	double x0 = x + tt.xx0;
	double x1 = x + tt.xx1;
	double y0 = y + tt.yy0;
	double z0 = z + tt.zz0;
	double z1 = z + tt.zz1;

	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x0, y0, z0, u0, v0);
	t.vertexUV(x1, y0, z0, u1, v0);
	t.vertexUV(x1, y0, z1, u1, v1);
}

void TileRenderer::renderFaceDown(Tile &tt, double x, double y, double z, int_t tex)
{
	Tesselator &t = Tesselator::instance;

	if (fixedTexture >= 0) tex = fixedTexture;

	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;

	double u0 = (xt + tt.xx0 * 16.0) / 256.0;
	double u1 = (xt + tt.xx1 * 16.0 - 0.01) / 256.0;
	double v0 = (yt + tt.zz0 * 16.0) / 256.0;
	double v1 = (yt + tt.zz1 * 16.0 - 0.01) / 256.0;

	if (tt.xx0 < 0.0 || tt.xx1 > 1.0)
	{
		u0 = ((xt + 0.0f) / 256.0f);
		u1 = ((xt + 15.99f) / 256.0f);
	}

	if (tt.zz0 < 0.0 || tt.zz1 > 1.0)
	{
		v0 = ((yt + 0.0f) / 256.0f);
		v1 = ((yt + 15.99f) / 256.0f);
	}

	double x0 = x + tt.xx0;
	double x1 = x + tt.xx1;
	double y1 = y + tt.yy1;
	double z0 = z + tt.zz0;
	double z1 = z + tt.zz1;

	t.vertexUV(x1, y1, z1, u1, v1);
	t.vertexUV(x1, y1, z0, u1, v0);
	t.vertexUV(x0, y1, z0, u0, v0);
	t.vertexUV(x0, y1, z1, u0, v1);
}

void TileRenderer::renderNorth(Tile &tt, double x, double y, double z, int_t tex)
{
	Tesselator &t = Tesselator::instance;

	if (fixedTexture >= 0) tex = fixedTexture;
	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;

	double u0 = (xt + tt.xx0 * 16.0) / 256.0;
	double u1 = (xt + tt.xx1 * 16.0 - 0.01) / 256.0;
	double v0 = (yt + tt.yy0 * 16.0) / 256.0;
	double v1 = (yt + tt.yy1 * 16.0 - 0.01) / 256.0;
	if (xFlipTexture)
	{
		double tmp = u0;
		u0 = u1;
		u1 = tmp;
	}

	if (tt.xx0 < 0.0 || tt.xx1 > 1.0)
	{
		u0 = ((xt + 0.0f) / 256.0f);
		u1 = ((xt + 15.99f) / 256.0f);
	}
	if (tt.yy0 < 0.0 || tt.yy1 > 1.0)
	{
		v0 = ((yt + 0.0f) / 256.0f);
		v1 = ((yt + 15.99f) / 256.0f);
	}

	double x0 = x + tt.xx0;
	double x1 = x + tt.xx1;
	double y0 = y + tt.yy0;
	double y1 = y + tt.yy1;
	double z0 = z + tt.zz0;

	t.vertexUV(x0, y1, z0, u1, v0);
	t.vertexUV(x1, y1, z0, u0, v0);
	t.vertexUV(x1, y0, z0, u0, v1);
	t.vertexUV(x0, y0, z0, u1, v1);
}

void TileRenderer::renderSouth(Tile &tt, double x, double y, double z, int_t tex)
{
	Tesselator &t = Tesselator::instance;

	if (fixedTexture >= 0) tex = fixedTexture;
	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;

	double u0 = (xt + tt.xx0 * 16.0) / 256.0;
	double u1 = (xt + tt.xx1 * 16.0 - 0.01) / 256.0;
	double v0 = (yt + tt.yy0 * 16.0) / 256.0;
	double v1 = (yt + tt.yy1 * 16.0 - 0.01) / 256.0;
	if (xFlipTexture)
	{
		double tmp = u0;
		u0 = u1;
		u1 = tmp;
	}

	if (tt.xx0 < 0.0 || tt.xx1 > 1.0)
	{
		u0 = ((xt + 0.0f) / 256.0f);
		u1 = ((xt + 15.99f) / 256.0f);
	}
	if (tt.yy0 < 0.0 || tt.yy1 > 1.0)
	{
		v0 = ((yt + 0.0f) / 256.0f);
		v1 = ((yt + 15.99f) / 256.0f);
	}

	double x0 = x + tt.xx0;
	double x1 = x + tt.xx1;
	double y0 = y + tt.yy0;
	double y1 = y + tt.yy1;
	double z1 = z + tt.zz1;

	t.vertexUV(x0, y1, z1, u0, v0);
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z1, u1, v1);
	t.vertexUV(x1, y1, z1, u1, v0);
}

void TileRenderer::renderWest(Tile &tt, double x, double y, double z, int_t tex)
{
	Tesselator &t = Tesselator::instance;

	if (fixedTexture >= 0) tex = fixedTexture;
	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;

	double u0 = (xt + tt.zz0 * 16.0) / 256.0;
	double u1 = (xt + tt.zz1 * 16.0 - 0.01) / 256.0;
	double v0 = (yt + tt.yy0 * 16.0) / 256.0;
	double v1 = (yt + tt.yy1 * 16.0 - 0.01) / 256.0;
	if (xFlipTexture)
	{
		double tmp = u0;
		u0 = u1;
		u1 = tmp;
	}

	if (tt.zz0 < 0.0 || tt.zz1 > 1.0)
	{
		u0 = ((xt + 0.0f) / 256.0f);
		u1 = ((xt + 15.99f) / 256.0f);
	}
	if (tt.yy0 < 0.0 || tt.yy1 > 1.0)
	{
		v0 = ((yt + 0.0f) / 256.0f);
		v1 = ((yt + 15.99f) / 256.0f);
	}

	double x0 = x + tt.xx0;
	double y0 = y + tt.yy0;
	double y1 = y + tt.yy1;
	double z0 = z + tt.zz0;
	double z1 = z + tt.zz1;

	t.vertexUV(x0, y1, z1, u1, v0);
	t.vertexUV(x0, y1, z0, u0, v0);
	t.vertexUV(x0, y0, z0, u0, v1);
	t.vertexUV(x0, y0, z1, u1, v1);
}

void TileRenderer::renderEast(Tile &tt, double x, double y, double z, int_t tex)
{
	Tesselator &t = Tesselator::instance;

	if (fixedTexture >= 0) tex = fixedTexture;
	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;

	double u0 = (xt + tt.zz0 * 16.0) / 256.0;
	double u1 = (xt + tt.zz1 * 16.0 - 0.01) / 256.0;
	double v0 = (yt + tt.yy0 * 16.0) / 256.0;
	double v1 = (yt + tt.yy1 * 16.0 - 0.01) / 256.0;
	if (xFlipTexture)
	{
		double tmp = u0;
		u0 = u1;
		u1 = tmp;
	}

	if (tt.zz0 < 0.0 || tt.zz1 > 1.0)
	{
		u0 = ((xt + 0.0f) / 256.0f);
		u1 = ((xt + 15.99f) / 256.0f);
	}
	if (tt.yy0 < 0.0 || tt.yy1 > 1.0)
	{
		v0 = ((yt + 0.0f) / 256.0f);
		v1 = ((yt + 15.99f) / 256.0f);
	}

	double x1 = x + tt.xx1;
	double y0 = y + tt.yy0;
	double y1 = y + tt.yy1;
	double z0 = z + tt.zz0;
	double z1 = z + tt.zz1;

	t.vertexUV(x1, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z0, u1, v1);
	t.vertexUV(x1, y1, z0, u1, v0);
	t.vertexUV(x1, y1, z1, u0, v0);
}

void TileRenderer::renderCube(Tile &tile, float alpha)
{
	int shape = tile.getRenderShape();
	Tesselator &t = Tesselator::instance;
	
	if (shape == Tile::SHAPE_BLOCK)
	{
		tile.updateDefaultShape();

		glTranslatef(-0.5f, -0.5f, -0.5f);
		
		float sd = 0.5f;
		float su = 1.0f;
		float sns = 0.8f;
		float sew = 0.6f;
		
		t.begin();

		t.color(su, su, su, alpha);
		renderFaceUp(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::DOWN));
		t.color(sd, sd, sd, alpha);
		renderFaceDown(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::UP));
		t.color(sns, sns, sns, alpha);
		renderNorth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::NORTH));
		renderSouth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::SOUTH));
		t.color(sew, sew, sew, alpha);
		renderWest(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::WEST));
		renderEast(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::EAST));
		
		t.end();
		
		glTranslatef(0.5f, 0.5f, 0.5f);
	}
}

// Beta: TileRenderer.renderTile() - renders tile at current position with data (TileRenderer.java:1746)
void TileRenderer::renderTile(Tile &tile, int_t data)
{
	Tesselator &t = Tesselator::instance;
	int_t shape = tile.getRenderShape();
	tile.updateDefaultShape();
	
	if (shape == Tile::SHAPE_BLOCK)
	{
		// Beta: Render as full block (TileRenderer.java:1749-1776)
		glTranslatef(-0.5f, -0.5f, -0.5f);
		t.begin();
		t.normal(0.0f, -1.0f, 0.0f);
		renderFaceUp(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::DOWN, data));
		t.end();
		t.begin();
		t.normal(0.0f, 1.0f, 0.0f);
		renderFaceDown(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::UP, data));
		t.end();
		t.begin();
		t.normal(0.0f, 0.0f, -1.0f);
		renderNorth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::NORTH, data));
		t.end();
		t.begin();
		t.normal(0.0f, 0.0f, 1.0f);
		renderSouth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::SOUTH, data));
		t.end();
		t.begin();
		t.normal(-1.0f, 0.0f, 0.0f);
		renderWest(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::WEST, data));
		t.end();
		t.begin();
		t.normal(1.0f, 0.0f, 0.0f);
		renderEast(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::EAST, data));
		t.end();
		glTranslatef(0.5f, 0.5f, 0.5f);
	}
	else if (shape == Tile::SHAPE_CROSS_TEXTURE)
	{
		// Beta: Render as cross-texture (flowers, reeds) (TileRenderer.java:1777-1781)
		// Note: This shape doesn't pass canRender check, so shouldn't reach here when rendering items
		// But keep for completeness if renderTile is called directly
		// For item rendering, cross-texture blocks should render as icons, not 3D models
		// This code path is kept for completeness but typically won't execute for EntityItem rendering
		t.begin();
		t.normal(0.0f, -1.0f, 0.0f);
		t.color(1.0f, 1.0f, 1.0f);
		int_t tex = tile.getTexture(Facing::NORTH);
		int_t xt = (tex & 0xF) << 4;
		int_t yt = tex & 0xF0;
		double u0 = static_cast<double>(xt) / 256.0;
		double u1 = static_cast<double>(xt + 15.99F) / 256.0;
		double v0 = static_cast<double>(yt) / 256.0;
		double v1 = static_cast<double>(yt + 15.99F) / 256.0;
		double offset = 0.45;
		double x0 = -0.5 + 0.5 - offset;
		double x1 = -0.5 + 0.5 + offset;
		double y0 = -0.5 + 0.0;
		double y1 = -0.5 + 1.0;
		double z0 = -0.5 + 0.5 - offset;
		double z1 = -0.5 + 0.5 + offset;
		// Render cross quads
		t.vertexUV(x0, y1, z0, u0, v0);
		t.vertexUV(x0, y0, z0, u0, v1);
		t.vertexUV(x1, y0, z1, u1, v1);
		t.vertexUV(x1, y1, z1, u1, v0);
		t.vertexUV(x1, y1, z1, u0, v0);
		t.vertexUV(x1, y0, z1, u0, v1);
		t.vertexUV(x0, y0, z0, u1, v1);
		t.vertexUV(x0, y1, z0, u1, v0);
		t.vertexUV(x0, y1, z1, u0, v0);
		t.vertexUV(x0, y0, z1, u0, v1);
		t.vertexUV(x1, y0, z0, u1, v1);
		t.vertexUV(x1, y1, z0, u1, v0);
		t.vertexUV(x1, y1, z0, u0, v0);
		t.vertexUV(x1, y0, z0, u0, v1);
		t.vertexUV(x0, y0, z1, u1, v1);
		t.vertexUV(x0, y1, z1, u1, v0);
		t.end();
	}
	else if (shape == Tile::SHAPE_CACTUS)
	{
		// Beta: Render cactus with custom model (TileRenderer.java:1782-1818)
		glTranslatef(-0.5f, -0.5f, -0.5f);
		float s = 0.0625f;  // 1.0F / 16.0F
		t.begin();
		t.normal(0.0f, -1.0f, 0.0f);
		renderFaceUp(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::DOWN));
		t.end();
		t.begin();
		t.normal(0.0f, 1.0f, 0.0f);
		renderFaceDown(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::UP));
		t.end();
		t.begin();
		t.normal(0.0f, 0.0f, -1.0f);
		t.addOffset(0.0f, 0.0f, s);
		renderNorth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::NORTH));
		t.addOffset(0.0f, 0.0f, -s);
		t.end();
		t.begin();
		t.normal(0.0f, 0.0f, 1.0f);
		t.addOffset(0.0f, 0.0f, -s);
		renderSouth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::SOUTH));
		t.addOffset(0.0f, 0.0f, s);
		t.end();
		t.begin();
		t.normal(-1.0f, 0.0f, 0.0f);
		t.addOffset(s, 0.0f, 0.0f);
		renderWest(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::WEST));
		t.addOffset(-s, 0.0f, 0.0f);
		t.end();
		t.begin();
		t.normal(1.0f, 0.0f, 0.0f);
		t.addOffset(-s, 0.0f, 0.0f);
		renderEast(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::EAST));
		t.addOffset(s, 0.0f, 0.0f);
		t.end();
		glTranslatef(0.5f, 0.5f, 0.5f);
	}
	else if (shape == Tile::SHAPE_STAIRS)
	{
		// Beta: Render stairs in hand (TileRenderer.java:1829-1865)
		// Render as two parts for item display
		for (int_t i = 0; i < 2; i++)
		{
			if (i == 0)
			{
				tile.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.5F);
			}
			if (i == 1)
			{
				tile.setShape(0.0F, 0.0F, 0.5F, 1.0F, 0.5F, 1.0F);
			}
			glTranslatef(-0.5F, -0.5F, -0.5F);
			t.begin();
			t.normal(0.0F, -1.0F, 0.0F);
			renderFaceUp(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::DOWN, data));
			t.end();
			t.begin();
			t.normal(0.0F, 1.0F, 0.0F);
			renderFaceDown(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::UP, data));
			t.end();
			t.begin();
			t.normal(0.0F, 0.0F, -1.0F);
			renderNorth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::NORTH, data));
			t.end();
			t.begin();
			t.normal(0.0F, 0.0F, 1.0F);
			renderSouth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::SOUTH, data));
			t.end();
			t.begin();
			t.normal(-1.0F, 0.0F, 0.0F);
			renderWest(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::WEST, data));
			t.end();
			t.begin();
			t.normal(1.0F, 0.0F, 0.0F);
			renderEast(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::EAST, data));
			t.end();
			glTranslatef(0.5F, 0.5F, 0.5F);
		}
		tile.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
	}
	else if (shape == Tile::SHAPE_FENCE)
	{
		// Beta: Render fence in hand (TileRenderer.java:1866-1915)
		// Render as 4 parts for item display
		for (int_t i = 0; i < 4; i++)
		{
			float w = 0.125F;
			if (i == 0)
			{
				tile.setShape(0.5F - w, 0.0F, 0.0F, 0.5F + w, 1.0F, w * 2.0F);
			}
			if (i == 1)
			{
				tile.setShape(0.5F - w, 0.0F, 1.0F - w * 2.0F, 0.5F + w, 1.0F, 1.0F);
			}
			w = 0.0625F;
			if (i == 2)
			{
				tile.setShape(0.5F - w, 1.0F - w * 3.0F, -w * 2.0F, 0.5F + w, 1.0F - w, 1.0F + w * 2.0F);
			}
			if (i == 3)
			{
				tile.setShape(0.5F - w, 0.5F - w * 3.0F, -w * 2.0F, 0.5F + w, 0.5F - w, 1.0F + w * 2.0F);
			}
			glTranslatef(-0.5F, -0.5F, -0.5F);
			t.begin();
			t.normal(0.0F, -1.0F, 0.0F);
			renderFaceUp(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::DOWN, data));
			t.end();
			t.begin();
			t.normal(0.0F, 1.0F, 0.0F);
			renderFaceDown(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::UP, data));
			t.end();
			t.begin();
			t.normal(0.0F, 0.0F, -1.0F);
			renderNorth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::NORTH, data));
			t.end();
			t.begin();
			t.normal(0.0F, 0.0F, 1.0F);
			renderSouth(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::SOUTH, data));
			t.end();
			t.begin();
			t.normal(-1.0F, 0.0F, 0.0F);
			renderWest(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::WEST, data));
			t.end();
			t.begin();
			t.normal(1.0F, 0.0F, 0.0F);
			renderEast(tile, 0.0, 0.0, 0.0, tile.getTexture(Facing::EAST, data));
			t.end();
			glTranslatef(0.5F, 0.5F, 0.5F);
		}
		tile.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
	}
}

// Beta: TileRenderer.canRender() - static check if renderShape can be rendered as item (TileRenderer.java:1918-1926)
bool TileRenderer::canRender(int_t renderShape)
{
	// Beta: canRender returns true for: 0 (SHAPE_BLOCK), 13 (SHAPE_CACTUS), 10, 11
	// Note: SHAPE_CROSS_TEXTURE (1) returns false, so flowers/reeds render as item icons
	if (renderShape == Tile::SHAPE_BLOCK)
		return true;
	if (renderShape == Tile::SHAPE_CACTUS)
		return true;
	if (renderShape == 10)  // SHAPE_STAIRS
		return true;
	if (renderShape == 11)  // SHAPE_FENCE
		return true;
	return false;
}

// Beta: TileRenderer.getWaterHeight() - calculates corner height by averaging neighbors
float TileRenderer::calculateCornerHeight(int_t x, int_t y, int_t z, const Material &material)
{
	// Beta: TileRenderer.getWaterHeight() (TileRenderer.java:1015-1043)
	int_t count = 0;
	float heightSum = 0.0F;

	for (int_t i = 0; i < 4; ++i)
	{
		int_t nx = x - (i & 1);
		int_t nz = z - ((i >> 1) & 1);
		
		// Beta: Check if block above is same fluid (returns 1.0F)
		const Material &aboveMat = level->getMaterial(nx, y + 1, nz);
		if (&aboveMat == &material)
		{
			return 1.0F;
		}

		const Material &neighborMat = level->getMaterial(nx, y, nz);
		if (&neighborMat == &material)
		{
			// Beta: Neighbor is same fluid - get its metadata
			int_t metadata = level->getData(nx, y, nz);
			if (metadata >= 8 || metadata == 0)
			{
				// Beta: Source block (metadata >= 8 or 0) counts as 10x weight
				heightSum += FluidTile::getHeight(metadata) * 10.0F;
				count += 10;
			}
			
			heightSum += FluidTile::getHeight(metadata);
			count++;
		}
		else if (!neighborMat.isSolid())
		{
			// Beta: If neighbor is not solid, count as 1.0F
			heightSum += 1.0F;
			count++;
		}
	}

	// Beta: Return 1.0F - (average height)
	return 1.0F - heightSum / static_cast<float>(count);
}

// Alpha: RenderBlocks.renderBottomFace() - renders bottom face of block
void TileRenderer::renderBottomFace(Tile &tt, double x, double y, double z, int_t tex)
{
	Tesselator &t = Tesselator::instance;

	if (fixedTexture >= 0) tex = fixedTexture;

	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;

	double u0 = (xt + tt.xx0 * 16.0) / 256.0;
	double u1 = (xt + tt.xx1 * 16.0 - 0.01) / 256.0;
	double v0 = (yt + tt.zz0 * 16.0) / 256.0;
	double v1 = (yt + tt.zz1 * 16.0 - 0.01) / 256.0;

	if (tt.xx0 < 0.0 || tt.xx1 > 1.0)
	{
		u0 = ((xt + 0.0f) / 256.0f);
		u1 = ((xt + 15.99f) / 256.0f);
	}

	if (tt.zz0 < 0.0 || tt.zz1 > 1.0)
	{
		v0 = ((yt + 0.0f) / 256.0f);
		v1 = ((yt + 15.99f) / 256.0f);
	}

	double x0 = x + tt.xx0;
	double x1 = x + tt.xx1;
	double y0 = y + tt.yy0;
	double z0 = z + tt.zz0;
	double z1 = z + tt.zz1;

	t.vertexUV(x0, y0, z0, u0, v0);
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z1, u1, v1);
	t.vertexUV(x1, y0, z0, u1, v0);
}

// Beta: TileRenderer.tesselateWaterInWorld() - renders water/lava with varying heights
bool TileRenderer::tesselateWaterInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: TileRenderer.tesselateWaterInWorld() (TileRenderer.java:867-1013)
	Tesselator &t = Tesselator::instance;
	
	// Beta: Check which faces should be rendered
	bool renderTop = noCulling || tt.shouldRenderFace(*level, x, y + 1, z, Facing::UP);
	bool renderBottom = noCulling || tt.shouldRenderFace(*level, x, y - 1, z, Facing::DOWN);
	bool renderNorth = noCulling || tt.shouldRenderFace(*level, x, y, z - 1, Facing::NORTH);
	bool renderSouth = noCulling || tt.shouldRenderFace(*level, x, y, z + 1, Facing::SOUTH);
	bool renderWest = noCulling || tt.shouldRenderFace(*level, x - 1, y, z, Facing::WEST);
	bool renderEast = noCulling || tt.shouldRenderFace(*level, x + 1, y, z, Facing::EAST);
	
	if (!renderTop && !renderBottom && !renderNorth && !renderSouth && !renderWest && !renderEast)
	{
		return false;
	}

	// Beta: Get fluid material
	FluidTile *fluidTile = dynamic_cast<FluidTile*>(&tt);
	if (!fluidTile)
		return false;
	
	const Material &material = tt.material;
	int_t metadata = level->getData(x, y, z);

	// Beta: Brightness multipliers (TileRenderer.java:881-884)
	float c10 = 0.5F;
	float c11 = 1.0F;
	float c2 = 0.8F;
	float c3 = 0.6F;

	// Beta: Calculate corner heights using getWaterHeight (TileRenderer.java:889-892)
	float h0 = calculateCornerHeight(x, y, z, material);
	float h1 = calculateCornerHeight(x, y, z + 1, material);
	float h2 = calculateCornerHeight(x + 1, y, z + 1, material);
	float h3 = calculateCornerHeight(x + 1, y, z, material);

	bool rendered = false;

	// Beta: Render top face with flow direction texture (TileRenderer.java:893-920)
	if (renderTop)
	{
		rendered = true;
		// Beta: getTexture(1, data) for top face - use level-based version
		int_t tex = tt.getTexture(*level, x, y, z, Facing::UP);
		
		// Beta: Calculate flow angle for texture UVs (TileRenderer.java:896-910)
		float angle = static_cast<float>(FluidTile::getSlopeAngle(*level, x, y, z, material));
		if (angle > -999.0F)
		{
			// Beta: If angle is valid, use side texture (getTexture(2, data))
			tex = tt.getTexture(*level, x, y, z, Facing::NORTH);
		}

		int_t xt = (tex & 0xF) << 4;
		int_t yt = tex & 0xF0;
		double uc = (xt + 8.0) / 256.0;
		double vc = (yt + 8.0) / 256.0;
		
		if (angle < -999.0F)
		{
			angle = 0.0F;
		}
		else
		{
			uc = (xt + 16) / 256.0F;
			vc = (yt + 16) / 256.0F;
		}

		// Beta: Calculate texture offset from flow angle (TileRenderer.java:912-913)
		float s = Mth::sin(angle) * 8.0F / 256.0F;
		float c = Mth::cos(angle) * 8.0F / 256.0F;
		
		float brightness = tt.getBrightness(*level, x, y, z);
		t.color(c11 * brightness, c11 * brightness, c11 * brightness);
		
		// Beta: Top face with flow direction UVs (TileRenderer.java:916-919)
		t.vertexUV(x + 0, y + h0, z + 0, uc - c - s, vc - c + s);
		t.vertexUV(x + 0, y + h1, z + 1, uc - c + s, vc + c + s);
		t.vertexUV(x + 1, y + h2, z + 1, uc + c + s, vc + c - s);
		t.vertexUV(x + 1, y + h3, z + 0, uc + c - s, vc - c - s);
	}

	// Beta: Render bottom face (TileRenderer.java:922-927)
	if (renderBottom)
	{
		rendered = true;
		float brightness = tt.getBrightness(*level, x, y - 1, z);
		t.color(c10 * brightness, c10 * brightness, c10 * brightness);
		// Beta: getTexture(0) for bottom face - use level-based version
		renderFaceUp(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::DOWN));
	}

	// Beta: Render 4 side faces (TileRenderer.java:929-1007)
	// Face 0: North (z - 1)
	if (renderNorth)
	{
		rendered = true;
		// Beta: getTexture(face + 2, data) for sides - use level-based version
		int_t texx = tt.getTexture(*level, x, y, z, Facing::NORTH);
		int_t xTex = (texx & 0xF) << 4;
		int_t yTex = texx & 0xF0;
		
		// Beta: North face uses h0 and h3 (TileRenderer.java:958-964)
		float hh0 = h0;
		float hh1 = h3;
		double x0 = x;
		double x1 = x + 1;
		double z0 = z;
		double z1 = z;
		
		double u0 = (xTex + 0) / 256.0F;
		double u1 = (xTex + 16 - 0.01) / 256.0;
		double v01 = (yTex + (1.0F - hh0) * 16.0F) / 256.0F;
		double v02 = (yTex + (1.0F - hh1) * 16.0F) / 256.0F;
		double v1 = (yTex + 16 - 0.01) / 256.0;
		
		float brightness = tt.getBrightness(*level, x, y, z - 1);
		// Beta: Face < 2 uses c2, face >= 2 uses c3 (TileRenderer.java:995-999)
		brightness *= c2;
		
		t.color(c11 * brightness, c11 * brightness, c11 * brightness);
		t.vertexUV(x0, y + hh0, z0, u0, v01);
		t.vertexUV(x1, y + hh1, z1, u1, v02);
		t.vertexUV(x1, y + 0, z1, u1, v1);
		t.vertexUV(x0, y + 0, z0, u0, v1);
	}

	// Face 1: South (z + 1)
	if (renderSouth)
	{
		rendered = true;
		int_t texx = tt.getTexture(*level, x, y, z, Facing::SOUTH);
		int_t xTex = (texx & 0xF) << 4;
		int_t yTex = texx & 0xF0;
		
		// Beta: South face uses h2 and h1 (TileRenderer.java:965-971)
		float hh0 = h2;
		float hh1 = h1;
		double x0 = x + 1;
		double x1 = x;
		double z0 = z + 1;
		double z1 = z + 1;
		
		double u0 = (xTex + 0) / 256.0F;
		double u1 = (xTex + 16 - 0.01) / 256.0;
		double v01 = (yTex + (1.0F - hh0) * 16.0F) / 256.0F;
		double v02 = (yTex + (1.0F - hh1) * 16.0F) / 256.0F;
		double v1 = (yTex + 16 - 0.01) / 256.0;
		
		float brightness = tt.getBrightness(*level, x, y, z + 1) * c2;
		t.color(c11 * brightness, c11 * brightness, c11 * brightness);
		t.vertexUV(x0, y + hh0, z0, u0, v01);
		t.vertexUV(x1, y + hh1, z1, u1, v02);
		t.vertexUV(x1, y + 0, z1, u1, v1);
		t.vertexUV(x0, y + 0, z0, u0, v1);
	}

	// Face 2: West (x - 1)
	if (renderWest)
	{
		rendered = true;
		int_t texx = tt.getTexture(*level, x, y, z, Facing::WEST);
		int_t xTex = (texx & 0xF) << 4;
		int_t yTex = texx & 0xF0;
		
		// Beta: West face uses h1 and h0 (TileRenderer.java:972-978)
		float hh0 = h1;
		float hh1 = h0;
		double x0 = x;
		double x1 = x;
		double z0 = z + 1;
		double z1 = z;
		
		double u0 = (xTex + 0) / 256.0F;
		double u1 = (xTex + 16 - 0.01) / 256.0;
		double v01 = (yTex + (1.0F - hh0) * 16.0F) / 256.0F;
		double v02 = (yTex + (1.0F - hh1) * 16.0F) / 256.0F;
		double v1 = (yTex + 16 - 0.01) / 256.0;
		
		float brightness = tt.getBrightness(*level, x - 1, y, z) * c3;
		t.color(c11 * brightness, c11 * brightness, c11 * brightness);
		t.vertexUV(x0, y + hh0, z0, u0, v01);
		t.vertexUV(x1, y + hh1, z1, u1, v02);
		t.vertexUV(x1, y + 0, z1, u1, v1);
		t.vertexUV(x0, y + 0, z0, u0, v1);
	}

	// Face 3: East (x + 1)
	if (renderEast)
	{
		rendered = true;
		int_t texx = tt.getTexture(*level, x, y, z, Facing::EAST);
		int_t xTex = (texx & 0xF) << 4;
		int_t yTex = texx & 0xF0;
		
		// Beta: East face uses h3 and h2 (TileRenderer.java:979-985)
		float hh0 = h3;
		float hh1 = h2;
		double x0 = x + 1;
		double x1 = x + 1;
		double z0 = z;
		double z1 = z + 1;
		
		double u0 = (xTex + 0) / 256.0F;
		double u1 = (xTex + 16 - 0.01) / 256.0;
		double v01 = (yTex + (1.0F - hh0) * 16.0F) / 256.0F;
		double v02 = (yTex + (1.0F - hh1) * 16.0F) / 256.0F;
		double v1 = (yTex + 16 - 0.01) / 256.0;
		
		float brightness = tt.getBrightness(*level, x + 1, y, z) * c3;
		t.color(c11 * brightness, c11 * brightness, c11 * brightness);
		t.vertexUV(x0, y + hh0, z0, u0, v01);
		t.vertexUV(x1, y + hh1, z1, u1, v02);
		t.vertexUV(x1, y + 0, z1, u1, v1);
		t.vertexUV(x0, y + 0, z0, u0, v1);
	}

	return rendered;
}

// Beta: RenderBlocks.func_1239_a() - renders cross-texture blocks (flowers, plants) (RenderBlocks.java:753-785)
bool TileRenderer::tesselateCrossTextureInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: RenderBlocks.renderBlockReed() calls func_1239_a (RenderBlocks.java:690-695)
	Tesselator &t = Tesselator::instance;
	
	// Beta: Get brightness and set color (RenderBlocks.java:692-693)
	float brightness = tt.getBrightness(*level, x, y, z);
	t.color(brightness, brightness, brightness);
	
	// Beta: Get texture (RenderBlocks.java:755-758)
	int_t tex = tt.getTexture(*level, x, y, z, Facing::NORTH);
	if (fixedTexture >= 0)
		tex = fixedTexture;
	
	// Beta: Calculate texture UVs (RenderBlocks.java:760-765)
	int_t xt = (tex & 0xF) << 4;
	int_t yt = tex & 0xF0;
	double u0 = static_cast<double>(xt) / 256.0;
	double u1 = static_cast<double>(xt + 15.99F) / 256.0;
	double v0 = static_cast<double>(yt) / 256.0;
	double v1 = static_cast<double>(yt + 15.99F) / 256.0;
	
	// Beta: func_1239_a uses fixed 0.45F offset from center (RenderBlocks.java:766-769)
	// Beta: var21 = var3 + 0.5D - 0.45F, var23 = var3 + 0.5D + 0.45F
	// Beta: var25 = var7 + 0.5D - 0.45F, var27 = var7 + 0.5D + 0.45F
	// Beta: var5 + 1.0D for top, var5 + 0.0D for bottom (always full height)
	// Beta uses fixed 0.45F offset (0.9 units wide) and full height (1.0) for all cross-texture blocks
	double offset = 0.45;
	double x0 = x + 0.5 - offset;
	double x1 = x + 0.5 + offset;
	double y0 = y + 0.0;  // Beta: var5 + 0.0D
	double y1 = y + 1.0;  // Beta: var5 + 1.0D
	double z0 = z + 0.5 - offset;
	double z1 = z + 0.5 + offset;
	
	// Beta: Render first quad - diagonal from (x0,z0) to (x1,z1) (RenderBlocks.java:770-773)
	// Quad 1: Diagonal plane
	t.vertexUV(x0, y1, z0, u0, v0);
	t.vertexUV(x0, y0, z0, u0, v1);
	t.vertexUV(x1, y0, z1, u1, v1);
	t.vertexUV(x1, y1, z1, u1, v0);
	
	// Beta: Render second quad - same as first, back face with flipped UVs (RenderBlocks.java:774-777)
	t.vertexUV(x1, y1, z1, u0, v0);
	t.vertexUV(x1, y0, z1, u0, v1);
	t.vertexUV(x0, y0, z0, u1, v1);
	t.vertexUV(x0, y1, z0, u1, v0);
	
	// Beta: Render third quad - perpendicular diagonal from (x0,z1) to (x1,z0) (RenderBlocks.java:778-781)
	// Quad 2: Perpendicular diagonal plane (forms cross with quad 1)
	t.vertexUV(x0, y1, z1, u0, v0);
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z0, u1, v1);
	t.vertexUV(x1, y1, z0, u1, v0);
	
	// Beta: Render fourth quad - same as third, back face with flipped UVs (RenderBlocks.java:782-785)
	t.vertexUV(x1, y1, z0, u0, v0);
	t.vertexUV(x1, y0, z0, u0, v1);
	t.vertexUV(x0, y0, z1, u1, v1);
	t.vertexUV(x0, y1, z1, u1, v0);
	
	return true;
}

// Beta: TileRenderer.tesselateCactusInWorld() - renders cactus with custom model (shrunk sides) (TileRenderer.java:1205-1297)
bool TileRenderer::tesselateCactusInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: tesselateCactusInWorld renders cactus with translation offsets on side faces
	Tesselator &t = Tesselator::instance;
	
	// Beta: Get color multipliers (TileRenderer.java:1198-1201)
	int_t col = tt.getColor(*level, x, y, z);
	float r = ((col >> 16) & 0xFF) / 255.0f;
	float g = ((col >> 8) & 0xFF) / 255.0f;
	float b = (col & 0xFF) / 255.0f;
	
	// Beta: Brightness multipliers (TileRenderer.java:1208-1211)
	float c10 = 0.5F;  // Bottom
	float c11 = 1.0F;  // Top
	float c2 = 0.8F;   // North/South
	float c3 = 0.6F;   // West/East
	
	// Beta: Calculate color components (TileRenderer.java:1212-1223)
	float r10 = c10 * r, r11 = c11 * r, r2 = c2 * r, r3 = c3 * r;
	float g10 = c10 * g, g11 = c11 * g, g2 = c2 * g, g3 = c3 * g;
	float b10 = c10 * b, b11 = c11 * b, b2 = c2 * b, b3 = c3 * b;
	
	// Beta: Offset for side faces (TileRenderer.java:1224)
	float s = 0.0625F;  // 1.0F / 16.0F
	
	float centerBrightness = tt.getBrightness(*level, x, y, z);
	bool changed = false;
	
	// Beta: Render bottom face (TileRenderer.java:1226-1231)
	if (noCulling || tt.shouldRenderFace(*level, x, y - 1, z, Facing::DOWN))
	{
		float br = tt.getBrightness(*level, x, y - 1, z);
		t.color(r10 * br, g10 * br, b10 * br);
		renderFaceUp(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::DOWN));
		changed = true;
	}
	
	// Beta: Render top face (TileRenderer.java:1233-1242)
	if (noCulling || tt.shouldRenderFace(*level, x, y + 1, z, Facing::UP))
	{
		float br = tt.getBrightness(*level, x, y + 1, z);
		if (tt.yy1 != 1.0 && !tt.material.isLiquid())
			br = centerBrightness;
		t.color(r11 * br, g11 * br, b11 * br);
		renderFaceDown(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::UP));
		changed = true;
	}
	
	// Beta: Render North face (z-1) with translation offset (TileRenderer.java:1244-1255)
	if (noCulling || tt.shouldRenderFace(*level, x, y, z - 1, Facing::NORTH))
	{
		float br = tt.getBrightness(*level, x, y, z - 1);
		if (tt.zz0 > 0.0)
			br = centerBrightness;
		t.color(r2 * br, g2 * br, b2 * br);
		// Beta: Translate by (0, 0, s) before rendering (TileRenderer.java:1251-1253)
		t.addOffset(0.0F, 0.0F, s);
		renderNorth(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::NORTH));
		t.addOffset(0.0F, 0.0F, -s);
		changed = true;
	}
	
	// Beta: Render South face (z+1) with translation offset (TileRenderer.java:1257-1268)
	if (noCulling || tt.shouldRenderFace(*level, x, y, z + 1, Facing::SOUTH))
	{
		float br = tt.getBrightness(*level, x, y, z + 1);
		if (tt.zz1 < 1.0)
			br = centerBrightness;
		t.color(r2 * br, g2 * br, b2 * br);
		// Beta: Translate by (0, 0, -s) before rendering (TileRenderer.java:1264-1266)
		t.addOffset(0.0F, 0.0F, -s);
		renderSouth(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::SOUTH));
		t.addOffset(0.0F, 0.0F, s);
		changed = true;
	}
	
	// Beta: Render West face (x-1) with translation offset (TileRenderer.java:1270-1281)
	if (noCulling || tt.shouldRenderFace(*level, x - 1, y, z, Facing::WEST))
	{
		float br = tt.getBrightness(*level, x - 1, y, z);
		if (tt.xx0 > 0.0)
			br = centerBrightness;
		t.color(r3 * br, g3 * br, b3 * br);
		// Beta: Translate by (s, 0, 0) before rendering (TileRenderer.java:1277-1279)
		t.addOffset(s, 0.0F, 0.0F);
		renderWest(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::WEST));
		t.addOffset(-s, 0.0F, 0.0F);
		changed = true;
	}
	
	// Beta: Render East face (x+1) with translation offset (TileRenderer.java:1283-1294)
	if (noCulling || tt.shouldRenderFace(*level, x + 1, y, z, Facing::EAST))
	{
		float br = tt.getBrightness(*level, x + 1, y, z);
		if (tt.xx1 < 1.0)
			br = centerBrightness;
		t.color(r3 * br, g3 * br, b3 * br);
		// Beta: Translate by (-s, 0, 0) before rendering (TileRenderer.java:1290-1292)
		t.addOffset(-s, 0.0F, 0.0F);
		renderEast(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::EAST));
		t.addOffset(s, 0.0F, 0.0F);
		changed = true;
	}
	
	return changed;
}

// Beta: TileRenderer.tesselateTorchInWorld() - renders torch blocks (TileRenderer.java:73-98)
bool TileRenderer::tesselateTorchInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	int_t dir = level->getData(x, y, z);
	Tesselator &t = Tesselator::instance;
	float br = tt.getBrightness(*level, x, y, z);
	if (Tile::lightEmission[tt.id] > 0)
	{
		br = 1.0F;
	}
	
	t.color(br, br, br);
	double r = 0.4F;
	double r2 = 0.5 - r;
	double h = 0.2F;
	if (dir == 1)
	{
		tesselateTorch(tt, (double)x - r2, (double)y + h, (double)z, -r, 0.0);
	}
	else if (dir == 2)
	{
		tesselateTorch(tt, (double)x + r2, (double)y + h, (double)z, r, 0.0);
	}
	else if (dir == 3)
	{
		tesselateTorch(tt, (double)x, (double)y + h, (double)z - r2, 0.0, -r);
	}
	else if (dir == 4)
	{
		tesselateTorch(tt, (double)x, (double)y + h, (double)z + r2, 0.0, r);
	}
	else
	{
		tesselateTorch(tt, (double)x, (double)y, (double)z, 0.0, 0.0);
	}
	
	return true;
}

// Beta: TileRenderer.tesselateTorch() - helper for torch rendering (TileRenderer.java:730-775)
void TileRenderer::tesselateTorch(Tile &tt, double x, double y, double z, double xxa, double zza)
{
	Tesselator &t = Tesselator::instance;
	int_t tex = tt.getTexture(Facing::NORTH);
	if (fixedTexture >= 0)
	{
		tex = fixedTexture;
	}
	
	int_t xt = (tex & 15) << 4;
	int_t yt = tex & 240;
	float u0 = xt / 256.0F;
	float u1 = (xt + 15.99F) / 256.0F;
	float v0 = yt / 256.0F;
	float v1 = (yt + 15.99F) / 256.0F;
	double uc0 = u0 + 0.02734375;
	double vc0 = v0 + 0.0234375;
	double uc1 = u0 + 0.03515625;
	double vc1 = v0 + 0.03125;
	x += 0.5;
	z += 0.5;
	double x0 = x - 0.5;
	double x1 = x + 0.5;
	double z0 = z - 0.5;
	double z1 = z + 0.5;
	double r = 0.0625;
	double h = 0.625;
	t.vertexUV(x + xxa * (1.0 - h) - r, y + h, z + zza * (1.0 - h) - r, uc0, vc0);
	t.vertexUV(x + xxa * (1.0 - h) - r, y + h, z + zza * (1.0 - h) + r, uc0, vc1);
	t.vertexUV(x + xxa * (1.0 - h) + r, y + h, z + zza * (1.0 - h) + r, uc1, vc1);
	t.vertexUV(x + xxa * (1.0 - h) + r, y + h, z + zza * (1.0 - h) - r, uc1, vc0);
	t.vertexUV(x - r, y + 1.0, z0, u0, v0);
	t.vertexUV(x - r + xxa, y + 0.0, z0 + zza, u0, v1);
	t.vertexUV(x - r + xxa, y + 0.0, z1 + zza, u1, v1);
	t.vertexUV(x - r, y + 1.0, z1, u1, v0);
	t.vertexUV(x + r, y + 1.0, z1, u0, v0);
	t.vertexUV(x + xxa + r, y + 0.0, z1 + zza, u0, v1);
	t.vertexUV(x + xxa + r, y + 0.0, z0 + zza, u1, v1);
	t.vertexUV(x + r, y + 1.0, z0, u1, v0);
	t.vertexUV(x0, y + 1.0, z + r, u0, v0);
	t.vertexUV(x0 + xxa, y + 0.0, z + r + zza, u0, v1);
	t.vertexUV(x1 + xxa, y + 0.0, z + r + zza, u1, v1);
	t.vertexUV(x1, y + 1.0, z + r, u1, v0);
	t.vertexUV(x1, y + 1.0, z - r, u0, v0);
	t.vertexUV(x1 + xxa, y + 0.0, z - r + zza, u0, v1);
	t.vertexUV(x0 + xxa, y + 0.0, z - r + zza, u1, v1);
	t.vertexUV(x0, y + 1.0, z - r, u1, v0);
}

// Beta: TileRenderer.tesselateLadderInWorld() - renders ladder blocks (TileRenderer.java:665-712)
bool TileRenderer::tesselateLadderInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	Tesselator &t = Tesselator::instance;
	int_t tex = tt.getTexture(Facing::NORTH);
	if (fixedTexture >= 0)
	{
		tex = fixedTexture;
	}
	
	float br = tt.getBrightness(*level, x, y, z);
	t.color(br, br, br);
	int_t xt = (tex & 15) << 4;
	int_t yt = tex & 240;
	double u0 = xt / 256.0F;
	double u1 = (xt + 15.99F) / 256.0F;
	double v0 = yt / 256.0F;
	double v1 = (yt + 15.99F) / 256.0F;
	int_t face = level->getData(x, y, z);
	float o = 0.0F;
	float r = 0.05F;
	if (face == 5)
	{
		t.vertexUV(x + r, y + 1 + o, z + 1 + o, u0, v0);
		t.vertexUV(x + r, y + 0 - o, z + 1 + o, u0, v1);
		t.vertexUV(x + r, y + 0 - o, z + 0 - o, u1, v1);
		t.vertexUV(x + r, y + 1 + o, z + 0 - o, u1, v0);
	}
	
	if (face == 4)
	{
		t.vertexUV(x + 1 - r, y + 0 - o, z + 1 + o, u1, v1);
		t.vertexUV(x + 1 - r, y + 1 + o, z + 1 + o, u1, v0);
		t.vertexUV(x + 1 - r, y + 1 + o, z + 0 - o, u0, v0);
		t.vertexUV(x + 1 - r, y + 0 - o, z + 0 - o, u0, v1);
	}
	
	if (face == 3)
	{
		t.vertexUV(x + 1 + o, y + 0 - o, z + r, u1, v1);
		t.vertexUV(x + 1 + o, y + 1 + o, z + r, u1, v0);
		t.vertexUV(x + 0 - o, y + 1 + o, z + r, u0, v0);
		t.vertexUV(x + 0 - o, y + 0 - o, z + r, u0, v1);
	}
	
	if (face == 2)
	{
		t.vertexUV(x + 1 + o, y + 1 + o, z + 1 - r, u0, v0);
		t.vertexUV(x + 1 + o, y + 0 - o, z + 1 - r, u0, v1);
		t.vertexUV(x + 0 - o, y + 0 - o, z + 1 - r, u1, v1);
		t.vertexUV(x + 0 - o, y + 1 + o, z + 1 - r, u1, v0);
	}
	
	return true;
}

// Beta: TileRenderer.tesselateDoorInWorld() - renders door blocks (TileRenderer.java:1386-1496)
bool TileRenderer::tesselateDoorInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	Tesselator &t = Tesselator::instance;
	DoorTile *dt = dynamic_cast<DoorTile *>(&tt);  // Beta: DoorTile dt = (DoorTile)tt (TileRenderer.java:1388)
	if (dt == nullptr)
		return false;  // Not a door tile
	
	bool changed = false;  // Beta: boolean changed = false (TileRenderer.java:1389)
	float c10 = 0.5f;  // Beta: float c10 = 0.5F (TileRenderer.java:1390)
	float c11 = 1.0f;  // Beta: float c11 = 1.0F (TileRenderer.java:1391)
	float c2 = 0.8f;   // Beta: float c2 = 0.8F (TileRenderer.java:1392)
	float c3 = 0.6f;   // Beta: float c3 = 0.6F (TileRenderer.java:1393)
	
	float centerBrightness = tt.getBrightness(*level, x, y, z);  // Beta: float centerBrightness = tt.getBrightness(this.level, x, y, z) (TileRenderer.java:1394)
	
	// Beta: Render UP face (TileRenderer.java:1395-1406)
	float br = tt.getBrightness(*level, x, y - 1, z);  // Beta: float br = tt.getBrightness(this.level, x, y - 1, z) (TileRenderer.java:1395)
	if (dt->yy0 > 0.0)  // Beta: if (dt.yy0 > 0.0) (TileRenderer.java:1396)
	{
		br = centerBrightness;  // Beta: br = centerBrightness (TileRenderer.java:1397)
	}
	
	if (Tile::lightEmission[tt.id] > 0)  // Beta: if (Tile.lightEmission[tt.id] > 0) (TileRenderer.java:1400)
	{
		br = 1.0f;  // Beta: br = 1.0F (TileRenderer.java:1401)
	}
	
	t.color(c10 * br, c10 * br, c10 * br);  // Beta: t.color(c10 * br, c10 * br, c10 * br) (TileRenderer.java:1404)
	renderFaceUp(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::UP));  // Beta: this.renderFaceUp(tt, x, y, z, tt.getTexture(this.level, x, y, z, 0)) (TileRenderer.java:1405)
	changed = true;  // Beta: changed = true (TileRenderer.java:1406)
	
	// Beta: Render DOWN face (TileRenderer.java:1407-1418)
	br = tt.getBrightness(*level, x, y + 1, z);  // Beta: float br = tt.getBrightness(this.level, x, y + 1, z) (TileRenderer.java:1407)
	if (dt->yy1 < 1.0)  // Beta: if (dt.yy1 < 1.0) (TileRenderer.java:1408)
	{
		br = centerBrightness;  // Beta: br = centerBrightness (TileRenderer.java:1409)
	}
	
	if (Tile::lightEmission[tt.id] > 0)  // Beta: if (Tile.lightEmission[tt.id] > 0) (TileRenderer.java:1412)
	{
		br = 1.0f;  // Beta: br = 1.0F (TileRenderer.java:1413)
	}
	
	t.color(c11 * br, c11 * br, c11 * br);  // Beta: t.color(c11 * br, c11 * br, c11 * br) (TileRenderer.java:1416)
	renderFaceDown(tt, x, y, z, tt.getTexture(*level, x, y, z, Facing::DOWN));  // Beta: this.renderFaceDown(tt, x, y, z, tt.getTexture(this.level, x, y, z, 1)) (TileRenderer.java:1417)
	changed = true;  // Beta: changed = true (TileRenderer.java:1418)
	
	// Beta: Render NORTH face (TileRenderer.java:1419-1437)
	br = tt.getBrightness(*level, x, y, z - 1);  // Beta: float br = tt.getBrightness(this.level, x, y, z - 1) (TileRenderer.java:1419)
	if (dt->zz0 > 0.0)  // Beta: if (dt.zz0 > 0.0) (TileRenderer.java:1420)
	{
		br = centerBrightness;  // Beta: br = centerBrightness (TileRenderer.java:1421)
	}
	
	if (Tile::lightEmission[tt.id] > 0)  // Beta: if (Tile.lightEmission[tt.id] > 0) (TileRenderer.java:1424)
	{
		br = 1.0f;  // Beta: br = 1.0F (TileRenderer.java:1425)
	}
	
	t.color(c2 * br, c2 * br, c2 * br);  // Beta: t.color(c2 * br, c2 * br, c2 * br) (TileRenderer.java:1428)
	int_t tex = tt.getTexture(*level, x, y, z, Facing::NORTH);  // Beta: int tex = tt.getTexture(this.level, x, y, z, 2) (TileRenderer.java:1429)
	if (tex < 0)  // Beta: if (tex < 0) (TileRenderer.java:1430)
	{
		xFlipTexture = true;  // Beta: this.xFlipTexture = true (TileRenderer.java:1431)
		tex = -tex;  // Beta: tex = -tex (TileRenderer.java:1432)
	}
	
	renderNorth(tt, x, y, z, tex);  // Beta: this.renderNorth(tt, x, y, z, tex) (TileRenderer.java:1435)
	changed = true;  // Beta: changed = true (TileRenderer.java:1436)
	xFlipTexture = false;  // Beta: this.xFlipTexture = false (TileRenderer.java:1437)
	
	// Beta: Render SOUTH face (TileRenderer.java:1438-1456)
	br = tt.getBrightness(*level, x, y, z + 1);  // Beta: float br = tt.getBrightness(this.level, x, y, z + 1) (TileRenderer.java:1438)
	if (dt->zz1 < 1.0)  // Beta: if (dt.zz1 < 1.0) (TileRenderer.java:1439)
	{
		br = centerBrightness;  // Beta: br = centerBrightness (TileRenderer.java:1440)
	}
	
	if (Tile::lightEmission[tt.id] > 0)  // Beta: if (Tile.lightEmission[tt.id] > 0) (TileRenderer.java:1443)
	{
		br = 1.0f;  // Beta: br = 1.0F (TileRenderer.java:1444)
	}
	
	t.color(c2 * br, c2 * br, c2 * br);  // Beta: t.color(c2 * br, c2 * br, c2 * br) (TileRenderer.java:1447)
	tex = tt.getTexture(*level, x, y, z, Facing::SOUTH);  // Beta: int tex = tt.getTexture(this.level, x, y, z, 3) (TileRenderer.java:1448)
	if (tex < 0)  // Beta: if (tex < 0) (TileRenderer.java:1449)
	{
		xFlipTexture = true;  // Beta: this.xFlipTexture = true (TileRenderer.java:1450)
		tex = -tex;  // Beta: tex = -tex (TileRenderer.java:1451)
	}
	
	renderSouth(tt, x, y, z, tex);  // Beta: this.renderSouth(tt, x, y, z, tex) (TileRenderer.java:1454)
	changed = true;  // Beta: changed = true (TileRenderer.java:1455)
	xFlipTexture = false;  // Beta: this.xFlipTexture = false (TileRenderer.java:1456)
	
	// Beta: Render WEST face (TileRenderer.java:1457-1475)
	br = tt.getBrightness(*level, x - 1, y, z);  // Beta: float br = tt.getBrightness(this.level, x - 1, y, z) (TileRenderer.java:1457)
	if (dt->xx0 > 0.0)  // Beta: if (dt.xx0 > 0.0) (TileRenderer.java:1458)
	{
		br = centerBrightness;  // Beta: br = centerBrightness (TileRenderer.java:1459)
	}
	
	if (Tile::lightEmission[tt.id] > 0)  // Beta: if (Tile.lightEmission[tt.id] > 0) (TileRenderer.java:1462)
	{
		br = 1.0f;  // Beta: br = 1.0F (TileRenderer.java:1463)
	}
	
	t.color(c3 * br, c3 * br, c3 * br);  // Beta: t.color(c3 * br, c3 * br, c3 * br) (TileRenderer.java:1466)
	tex = tt.getTexture(*level, x, y, z, Facing::WEST);  // Beta: int tex = tt.getTexture(this.level, x, y, z, 4) (TileRenderer.java:1467)
	if (tex < 0)  // Beta: if (tex < 0) (TileRenderer.java:1468)
	{
		xFlipTexture = true;  // Beta: this.xFlipTexture = true (TileRenderer.java:1469)
		tex = -tex;  // Beta: tex = -tex (TileRenderer.java:1470)
	}
	
	renderWest(tt, x, y, z, tex);  // Beta: this.renderWest(tt, x, y, z, tex) (TileRenderer.java:1473)
	changed = true;  // Beta: changed = true (TileRenderer.java:1474)
	xFlipTexture = false;  // Beta: this.xFlipTexture = false (TileRenderer.java:1475)
	
	// Beta: Render EAST face (TileRenderer.java:1476-1495)
	br = tt.getBrightness(*level, x + 1, y, z);  // Beta: float br = tt.getBrightness(this.level, x + 1, y, z) (TileRenderer.java:1476)
	if (dt->xx1 < 1.0)  // Beta: if (dt.xx1 < 1.0) (TileRenderer.java:1477)
	{
		br = centerBrightness;  // Beta: br = centerBrightness (TileRenderer.java:1478)
	}
	
	if (Tile::lightEmission[tt.id] > 0)  // Beta: if (Tile.lightEmission[tt.id] > 0) (TileRenderer.java:1481)
	{
		br = 1.0f;  // Beta: br = 1.0F (TileRenderer.java:1482)
	}
	
	t.color(c3 * br, c3 * br, c3 * br);  // Beta: t.color(c3 * br, c3 * br, c3 * br) (TileRenderer.java:1485)
	tex = tt.getTexture(*level, x, y, z, Facing::EAST);  // Beta: int tex = tt.getTexture(this.level, x, y, z, 5) (TileRenderer.java:1486)
	if (tex < 0)  // Beta: if (tex < 0) (TileRenderer.java:1487)
	{
		xFlipTexture = true;  // Beta: this.xFlipTexture = true (TileRenderer.java:1488)
		tex = -tex;  // Beta: tex = -tex (TileRenderer.java:1489)
	}
	
	renderEast(tt, x, y, z, tex);  // Beta: this.renderEast(tt, x, y, z, tex) (TileRenderer.java:1492)
	changed = true;  // Beta: changed = true (TileRenderer.java:1493)
	xFlipTexture = false;  // Beta: this.xFlipTexture = false (TileRenderer.java:1494)
	
	return changed;  // Beta: return changed (TileRenderer.java:1495)
}

// Beta: TileRenderer.tesselateStairsInWorld() - renders stairs blocks (TileRenderer.java:1357-1384)
bool TileRenderer::tesselateStairsInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	bool changed = false;
	int_t dir = level->getData(x, y, z);
	if (dir == 0)
	{
		tt.setShape(0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 1.0F);
		tesselateBlockInWorld(tt, x, y, z);
		tt.setShape(0.5F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
		tesselateBlockInWorld(tt, x, y, z);
		changed = true;
	}
	else if (dir == 1)
	{
		tt.setShape(0.0F, 0.0F, 0.0F, 0.5F, 1.0F, 1.0F);
		tesselateBlockInWorld(tt, x, y, z);
		tt.setShape(0.5F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F);
		tesselateBlockInWorld(tt, x, y, z);
		changed = true;
	}
	else if (dir == 2)
	{
		tt.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 0.5F);
		tesselateBlockInWorld(tt, x, y, z);
		tt.setShape(0.0F, 0.0F, 0.5F, 1.0F, 1.0F, 1.0F);
		tesselateBlockInWorld(tt, x, y, z);
		changed = true;
	}
	else if (dir == 3)
	{
		tt.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.5F);
		tesselateBlockInWorld(tt, x, y, z);
		tt.setShape(0.0F, 0.0F, 0.5F, 1.0F, 0.5F, 1.0F);
		tesselateBlockInWorld(tt, x, y, z);
		changed = true;
	}
	
	tt.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
	return changed;
}

bool TileRenderer::tesselateFireInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: Fire rendering - RenderBlocks.renderBlockFire() (RenderBlocks.java:220-406) - 1:1 port
	Tesselator &t = Tesselator::instance;
	
	// Beta: Get texture (RenderBlocks.java:222-225)
	int_t tex = tt.getTexture(Facing::UP, 0);
	if (fixedTexture >= 0)
		tex = fixedTexture;
	
	// Beta: Calculate brightness and set color (RenderBlocks.java:227-228)
	float br = tt.getBrightness(*level, x, y, z);
	t.color(br, br, br);
	
	// Beta: Calculate texture coordinates (RenderBlocks.java:229-234)
	int_t xt = (tex & 15) << 4;
	int_t yt = tex & 240;
	double u0 = (double)xt / 256.0;
	double u1 = (double)(xt + 15.99) / 256.0;
	double v0 = (double)yt / 256.0;
	double v1 = (double)(yt + 15.99) / 256.0;
	
	float height = 1.4f;  // Beta: var18 = 1.4F (RenderBlocks.java:235)
	double var19, var21, var23, var25, var27, var29, var31;  // Beta: Declare variables (RenderBlocks.java:236-242)
	
	// Beta: Check if block below is solid and can catch fire (RenderBlocks.java:243)
	bool solidBelow = level->isSolidTile(x, y - 1, z);
	bool canCatchFireBelow = FireTile::canBlockCatchFire(*level, x, y - 1, z);
	
	if (!solidBelow && !canCatchFireBelow)
	{
		// Beta: Render side flames when not on solid block (RenderBlocks.java:244-345)
		float sideOffset = 0.2f;  // Beta: var37 = 0.2F (RenderBlocks.java:244)
		float offset = 1.0f / 16.0f;  // Beta: var34 = 1.0F / 16.0F (RenderBlocks.java:245)
		
		// Beta: Randomize texture based on position (RenderBlocks.java:246-257)
		if ((x + y + z & 1) == 1)
		{
			u0 = (double)xt / 256.0;
			u1 = (double)(xt + 15.99) / 256.0;
			v0 = (double)(yt + 16) / 256.0;
			v1 = (double)(yt + 15.99 + 16.0) / 256.0;
		}
		
		if ((x / 2 + y / 2 + z / 2 & 1) == 1)
		{
			var19 = u1;
			u1 = u0;
			u0 = var19;
		}
		
		// Beta: Render side flames if adjacent blocks can catch fire (RenderBlocks.java:259-301)
		if (FireTile::canBlockCatchFire(*level, x - 1, y, z))
		{
			// West face (RenderBlocks.java:260-267)
			t.vertexUV((double)((float)x + sideOffset), (double)((float)y + height + offset), (double)(z + 1), u1, v0);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 1), u1, v1);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 0), u0, v1);
			t.vertexUV((double)((float)x + sideOffset), (double)((float)y + height + offset), (double)(z + 0), u0, v0);
			// Back face
			t.vertexUV((double)((float)x + sideOffset), (double)((float)y + height + offset), (double)(z + 0), u0, v0);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 0), u0, v1);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 1), u1, v1);
			t.vertexUV((double)((float)x + sideOffset), (double)((float)y + height + offset), (double)(z + 1), u1, v0);
		}
		
		if (FireTile::canBlockCatchFire(*level, x + 1, y, z))
		{
			// East face (RenderBlocks.java:270-278)
			t.vertexUV((double)((float)(x + 1) - sideOffset), (double)((float)y + height + offset), (double)(z + 0), u0, v0);
			t.vertexUV((double)(x + 1 - 0), (double)((float)(y + 0) + offset), (double)(z + 0), u0, v1);
			t.vertexUV((double)(x + 1 - 0), (double)((float)(y + 0) + offset), (double)(z + 1), u1, v1);
			t.vertexUV((double)((float)(x + 1) - sideOffset), (double)((float)y + height + offset), (double)(z + 1), u1, v0);
			// Back face
			t.vertexUV((double)((float)(x + 1) - sideOffset), (double)((float)y + height + offset), (double)(z + 1), u1, v0);
			t.vertexUV((double)(x + 1 - 0), (double)((float)(y + 0) + offset), (double)(z + 1), u1, v1);
			t.vertexUV((double)(x + 1 - 0), (double)((float)(y + 0) + offset), (double)(z + 0), u0, v1);
			t.vertexUV((double)((float)(x + 1) - sideOffset), (double)((float)y + height + offset), (double)(z + 0), u0, v0);
		}
		
		if (FireTile::canBlockCatchFire(*level, x, y, z - 1))
		{
			// North face (RenderBlocks.java:281-289)
			t.vertexUV((double)(x + 0), (double)((float)y + height + offset), (double)((float)z + sideOffset), u1, v0);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 0), u1, v1);
			t.vertexUV((double)(x + 1), (double)((float)(y + 0) + offset), (double)(z + 0), u0, v1);
			t.vertexUV((double)(x + 1), (double)((float)y + height + offset), (double)((float)z + sideOffset), u0, v0);
			// Back face
			t.vertexUV((double)(x + 1), (double)((float)y + height + offset), (double)((float)z + sideOffset), u0, v0);
			t.vertexUV((double)(x + 1), (double)((float)(y + 0) + offset), (double)(z + 0), u0, v1);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 0), u1, v1);
			t.vertexUV((double)(x + 0), (double)((float)y + height + offset), (double)((float)z + sideOffset), u1, v0);
		}
		
		if (FireTile::canBlockCatchFire(*level, x, y, z + 1))
		{
			// South face (RenderBlocks.java:292-300)
			t.vertexUV((double)(x + 1), (double)((float)y + height + offset), (double)((float)(z + 1) - sideOffset), u0, v0);
			t.vertexUV((double)(x + 1), (double)((float)(y + 0) + offset), (double)(z + 1 - 0), u0, v1);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 1 - 0), u1, v1);
			t.vertexUV((double)(x + 0), (double)((float)y + height + offset), (double)((float)(z + 1) - sideOffset), u1, v0);
			// Back face
			t.vertexUV((double)(x + 0), (double)((float)y + height + offset), (double)((float)(z + 1) - sideOffset), u1, v0);
			t.vertexUV((double)(x + 0), (double)((float)(y + 0) + offset), (double)(z + 1 - 0), u1, v1);
			t.vertexUV((double)(x + 1), (double)((float)(y + 0) + offset), (double)(z + 1 - 0), u0, v1);
			t.vertexUV((double)(x + 1), (double)((float)y + height + offset), (double)((float)(z + 1) - sideOffset), u0, v0);
		}
		
		// Beta: Render upward flame if fire is above (RenderBlocks.java:303-345)
		if (FireTile::canBlockCatchFire(*level, x, y + 1, z))
		{
			var19 = (double)x + 0.5 + 0.5;
			var21 = (double)x + 0.5 - 0.5;
			var23 = (double)z + 0.5 + 0.5;
			var25 = (double)z + 0.5 - 0.5;
			var27 = (double)x + 0.5 - 0.5;
			var29 = (double)x + 0.5 + 0.5;
			var31 = (double)z + 0.5 - 0.5;
			double var35 = (double)z + 0.5 + 0.5;
			u0 = (double)xt / 256.0;
			u1 = (double)(xt + 15.99) / 256.0;
			v0 = (double)yt / 256.0;
			v1 = (double)(yt + 15.99) / 256.0;
			int_t yAbove = y + 1;
			height = -0.2f;  // Beta: var18 = -0.2F (RenderBlocks.java:317)
			if ((x + yAbove + z & 1) == 0)
			{
				// Beta: Render upward flame quads (RenderBlocks.java:319-330)
				t.vertexUV(var27, (double)((float)yAbove + height), (double)(z + 0), u1, v0);
				t.vertexUV(var19, (double)(yAbove + 0), (double)(z + 0), u1, v1);
				t.vertexUV(var19, (double)(yAbove + 0), (double)(z + 1), u0, v1);
				t.vertexUV(var27, (double)((float)yAbove + height), (double)(z + 1), u0, v0);
				u0 = (double)xt / 256.0;
				u1 = (double)(xt + 15.99) / 256.0;
				v0 = (double)(yt + 16) / 256.0;
				v1 = (double)(yt + 15.99 + 16.0) / 256.0;
				t.vertexUV(var29, (double)((float)yAbove + height), (double)(z + 1), u1, v0);
				t.vertexUV(var21, (double)(yAbove + 0), (double)(z + 1), u1, v1);
				t.vertexUV(var21, (double)(yAbove + 0), (double)(z + 0), u0, v1);
				t.vertexUV(var29, (double)((float)yAbove + height), (double)(z + 0), u0, v0);
			}
			else
			{
				// Beta: Render upward flame quads (alternate pattern) (RenderBlocks.java:332-343)
				t.vertexUV((double)(x + 0), (double)((float)yAbove + height), var35, u1, v0);
				t.vertexUV((double)(x + 0), (double)(yAbove + 0), var25, u1, v1);
				t.vertexUV((double)(x + 1), (double)(yAbove + 0), var25, u0, v1);
				t.vertexUV((double)(x + 1), (double)((float)yAbove + height), var35, u0, v0);
				u0 = (double)xt / 256.0;
				u1 = (double)(xt + 15.99) / 256.0;
				v0 = (double)(yt + 16) / 256.0;
				v1 = (double)(yt + 15.99 + 16.0) / 256.0;
				t.vertexUV((double)(x + 1), (double)((float)yAbove + height), var31, u1, v0);
				t.vertexUV((double)(x + 1), (double)(yAbove + 0), var23, u1, v1);
				t.vertexUV((double)(x + 0), (double)(yAbove + 0), var23, u0, v1);
				t.vertexUV((double)(x + 0), (double)((float)yAbove + height), var31, u0, v0);
			}
		}
	}
	else
	{
		// Beta: Render center flame when on solid block (RenderBlocks.java:346-403)
		double var33 = (double)x + 0.5 + 0.2;
		var19 = (double)x + 0.5 - 0.2;
		var21 = (double)z + 0.5 + 0.2;
		var23 = (double)z + 0.5 - 0.2;
		var25 = (double)x + 0.5 - 0.3;
		var27 = (double)x + 0.5 + 0.3;
		var29 = (double)z + 0.5 - 0.3;
		var31 = (double)z + 0.5 + 0.3;
		
		// Beta: First two quads (RenderBlocks.java:355-362)
		t.vertexUV(var25, (double)((float)y + height), (double)(z + 1), u1, v0);
		t.vertexUV(var33, (double)(y + 0), (double)(z + 1), u1, v1);
		t.vertexUV(var33, (double)(y + 0), (double)(z + 0), u0, v1);
		t.vertexUV(var25, (double)((float)y + height), (double)(z + 0), u0, v0);
		t.vertexUV(var27, (double)((float)y + height), (double)(z + 0), u1, v0);
		t.vertexUV(var19, (double)(y + 0), (double)(z + 0), u1, v1);
		t.vertexUV(var19, (double)(y + 0), (double)(z + 1), u0, v1);
		t.vertexUV(var27, (double)((float)y + height), (double)(z + 1), u0, v0);
		
		// Beta: Switch to alternate texture (RenderBlocks.java:363-366)
		u0 = (double)xt / 256.0;
		u1 = (double)(xt + 15.99) / 256.0;
		v0 = (double)(yt + 16) / 256.0;
		v1 = (double)(yt + 15.99 + 16.0) / 256.0;
		
		// Beta: Next two quads (RenderBlocks.java:367-374)
		t.vertexUV((double)(x + 1), (double)((float)y + height), var31, u1, v0);
		t.vertexUV((double)(x + 1), (double)(y + 0), var23, u1, v1);
		t.vertexUV((double)(x + 0), (double)(y + 0), var23, u0, v1);
		t.vertexUV((double)(x + 0), (double)((float)y + height), var31, u0, v0);
		t.vertexUV((double)(x + 0), (double)((float)y + height), var29, u1, v0);
		t.vertexUV((double)(x + 0), (double)(y + 0), var21, u1, v1);
		t.vertexUV((double)(x + 1), (double)(y + 0), var21, u0, v1);
		t.vertexUV((double)(x + 1), (double)((float)y + height), var29, u0, v0);
		
		// Beta: Calculate new coordinates (RenderBlocks.java:375-382)
		var33 = (double)x + 0.5 - 0.5;
		var19 = (double)x + 0.5 + 0.5;
		var21 = (double)z + 0.5 - 0.5;
		var23 = (double)z + 0.5 + 0.5;
		var25 = (double)x + 0.5 - 0.4;
		var27 = (double)x + 0.5 + 0.4;
		var29 = (double)z + 0.5 - 0.4;
		var31 = (double)z + 0.5 + 0.4;
		
		// Beta: Next two quads (RenderBlocks.java:383-390)
		t.vertexUV(var25, (double)((float)y + height), (double)(z + 0), u0, v0);
		t.vertexUV(var33, (double)(y + 0), (double)(z + 0), u0, v1);
		t.vertexUV(var33, (double)(y + 0), (double)(z + 1), u1, v1);
		t.vertexUV(var25, (double)((float)y + height), (double)(z + 1), u1, v0);
		t.vertexUV(var27, (double)((float)y + height), (double)(z + 1), u0, v0);
		t.vertexUV(var19, (double)(y + 0), (double)(z + 1), u0, v1);
		t.vertexUV(var19, (double)(y + 0), (double)(z + 0), u1, v1);
		t.vertexUV(var27, (double)((float)y + height), (double)(z + 0), u1, v0);
		
		// Beta: Reset texture coordinates (RenderBlocks.java:391-394)
		u0 = (double)xt / 256.0;
		u1 = (double)(xt + 15.99) / 256.0;
		v0 = (double)yt / 256.0;
		v1 = (double)(yt + 15.99) / 256.0;
		
		// Beta: Final two quads (RenderBlocks.java:395-402)
		t.vertexUV((double)(x + 0), (double)((float)y + height), var31, u0, v0);
		t.vertexUV((double)(x + 0), (double)(y + 0), var23, u0, v1);
		t.vertexUV((double)(x + 1), (double)(y + 0), var23, u1, v1);
		t.vertexUV((double)(x + 1), (double)((float)y + height), var31, u1, v0);
		t.vertexUV((double)(x + 1), (double)((float)y + height), var29, u0, v0);
		t.vertexUV((double)(x + 1), (double)(y + 0), var21, u0, v1);
		t.vertexUV((double)(x + 0), (double)(y + 0), var21, u1, v1);
		t.vertexUV((double)(x + 0), (double)((float)y + height), var29, u1, v0);
	}
	
	return true;
}

bool TileRenderer::tesselateDustInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: TileRenderer.tesselateDustInWorld() - renders redstone dust as flat layer (TileRenderer.java:444-563)
	Tesselator &t = Tesselator::instance;
	
	// Beta: Get texture based on data (TileRenderer.java:446)
	// Beta: getTexture(1, data) where 1 = Facing::UP
	int_t tex = tt.getTexture(Facing::UP, level->getData(x, y, z));
	if (fixedTexture >= 0)
		tex = fixedTexture;
	
	// Beta: Calculate brightness and set color (TileRenderer.java:451-452)
	float br = tt.getBrightness(*level, x, y, z);
	t.color(br, br, br);
	
	// Beta: Calculate texture coordinates (TileRenderer.java:453-458)
	int_t xt = (tex & 15) << 4;
	int_t yt = tex & 240;
	double u0 = (double)xt / 256.0;
	double u1 = (double)(xt + 15.99) / 256.0;
	double v0 = (double)yt / 256.0;
	double v1 = (double)(yt + 15.99) / 256.0;
	
	float o = 0.0f;
	float r = 0.03125f;  // Beta: 0.03125F = 1/32 (TileRenderer.java:459-460)
	
	// Beta: Check connections to adjacent redstone dust (TileRenderer.java:461-485)
	bool w = RedStoneDustTile::shouldConnectTo(*level, x - 1, y, z)
		|| (!level->isSolidTile(x - 1, y, z) && RedStoneDustTile::shouldConnectTo(*level, x - 1, y - 1, z));
	bool e = RedStoneDustTile::shouldConnectTo(*level, x + 1, y, z)
		|| (!level->isSolidTile(x + 1, y, z) && RedStoneDustTile::shouldConnectTo(*level, x + 1, y - 1, z));
	bool n = RedStoneDustTile::shouldConnectTo(*level, x, y, z - 1)
		|| (!level->isSolidTile(x, y, z - 1) && RedStoneDustTile::shouldConnectTo(*level, x, y - 1, z - 1));
	bool s = RedStoneDustTile::shouldConnectTo(*level, x, y, z + 1)
		|| (!level->isSolidTile(x, y, z + 1) && RedStoneDustTile::shouldConnectTo(*level, x, y - 1, z + 1));
	
	if (!level->isSolidTile(x, y + 1, z))
	{
		if (level->isSolidTile(x - 1, y, z) && RedStoneDustTile::shouldConnectTo(*level, x - 1, y + 1, z))
			w = true;
		if (level->isSolidTile(x + 1, y, z) && RedStoneDustTile::shouldConnectTo(*level, x + 1, y + 1, z))
			e = true;
		if (level->isSolidTile(x, y, z - 1) && RedStoneDustTile::shouldConnectTo(*level, x, y + 1, z - 1))
			n = true;
		if (level->isSolidTile(x, y, z + 1) && RedStoneDustTile::shouldConnectTo(*level, x, y + 1, z + 1))
			s = true;
	}
	
	// Beta: Calculate quad coordinates (TileRenderer.java:487-491)
	float d = 0.3125f;  // Beta: 0.3125F = 5/16 (TileRenderer.java:487)
	float x0 = (float)(x + 0);
	float x1 = (float)(x + 1);
	float z0 = (float)(z + 0);
	float z1 = (float)(z + 1);
	
	// Beta: Determine pattern (TileRenderer.java:492-499)
	int_t pic = 0;
	if ((w || e) && !n && !s)
		pic = 1;  // Horizontal line
	if ((n || s) && !e && !w)
		pic = 2;  // Vertical line
	
	// Beta: Adjust texture coordinates for line patterns (TileRenderer.java:501-506)
	if (pic != 0)
	{
		u0 = (double)(xt + 16) / 256.0;
		u1 = (double)(xt + 16 + 15.99) / 256.0;
		v0 = (double)yt / 256.0;
		v1 = (double)(yt + 15.99) / 256.0;
	}
	
	// Beta: Adjust coordinates and texture for connections (TileRenderer.java:508-541)
	if (pic == 0)
	{
		if (e || n || s || w)
		{
			if (!w)
			{
				x0 += d;
				u0 += d / 16.0f;
			}
			if (!e)
			{
				x1 -= d;
				u1 -= d / 16.0f;
			}
			if (!n)
			{
				z0 += d;
				v0 += d / 16.0f;
			}
			if (!s)
			{
				z1 -= d;
				v1 -= d / 16.0f;
			}
		}
		
		// Beta: Render main quad (TileRenderer.java:543-546)
		t.vertexUV(x1 + o, y + r, z1 + o, u1, v1);
		t.vertexUV(x1 + o, y + r, z0 - o, u1, v0);
		t.vertexUV(x0 - o, y + r, z0 - o, u0, v0);
		t.vertexUV(x0 - o, y + r, z1 + o, u0, v1);
	}
	
	// Beta: Render horizontal line (TileRenderer.java:549-553)
	if (pic == 1)
	{
		t.vertexUV(x1 + o, y + r, z1 + o, u1, v1);
		t.vertexUV(x1 + o, y + r, z0 - o, u1, v0);
		t.vertexUV(x0 - o, y + r, z0 - o, u0, v0);
		t.vertexUV(x0 - o, y + r, z1 + o, u0, v1);
	}
	
	// Beta: Render vertical line (TileRenderer.java:556-560)
	if (pic == 2)
	{
		t.vertexUV(x1 + o, y + r, z1 + o, u1, v1);
		t.vertexUV(x1 + o, y + r, z0 - o, u0, v1);
		t.vertexUV(x0 - o, y + r, z0 - o, u0, v0);
		t.vertexUV(x0 - o, y + r, z1 + o, u1, v0);
	}
	
	return true;
}

bool TileRenderer::tesselateRowInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: Row texture rendering - used for pressure plates, etc.
	return tesselateCrossTextureInWorld(tt, x, y, z);
}

bool TileRenderer::tesselateRailInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: TileRenderer.tesselateRailInWorld() - renders rails based on data value (TileRenderer.java:600-663)
	Tesselator &t = Tesselator::instance;
	int_t data = level->getData(x, y, z);  // Beta: int data = this.level.getData(x, y, z) (TileRenderer.java:602)
	int_t tex = tt.getTexture(Facing::DOWN, data);  // Beta: int tex = tt.getTexture(0, data) (TileRenderer.java:603)
	if (fixedTexture >= 0)  // Beta: if (this.fixedTexture >= 0) (TileRenderer.java:604)
	{
		tex = fixedTexture;  // Beta: tex = this.fixedTexture (TileRenderer.java:605)
	}

	float br = tt.getBrightness(*level, x, y, z);  // Beta: float br = tt.getBrightness(this.level, x, y, z) (TileRenderer.java:608)
	t.color(br, br, br);  // Beta: t.color(br, br, br) (TileRenderer.java:609)
	
	// Beta: Calculate texture UV coordinates (TileRenderer.java:610-615)
	int_t xt = (tex & 15) << 4;  // Beta: int xt = (tex & 15) << 4 (TileRenderer.java:610)
	int_t yt = tex & 240;  // Beta: int yt = tex & 240 (TileRenderer.java:611)
	double u0 = static_cast<double>(xt) / 256.0;  // Beta: double u0 = xt / 256.0F (TileRenderer.java:612)
	double u1 = static_cast<double>(xt + 15.99f) / 256.0;  // Beta: double u1 = (xt + 15.99F) / 256.0F (TileRenderer.java:613)
	double v0 = static_cast<double>(yt) / 256.0;  // Beta: double v0 = yt / 256.0F (TileRenderer.java:614)
	double v1 = static_cast<double>(yt + 15.99f) / 256.0;  // Beta: double v1 = (yt + 15.99F) / 256.0F (TileRenderer.java:615)
	
	float r = 0.0625f;  // Beta: float r = 0.0625F (TileRenderer.java:616)
	
	// Beta: Initialize vertex positions (TileRenderer.java:617-628)
	float x0 = static_cast<float>(x) + 1.0f;  // Beta: float x0 = x + 1 (TileRenderer.java:617)
	float x1 = static_cast<float>(x) + 1.0f;  // Beta: float x1 = x + 1 (TileRenderer.java:618)
	float x2 = static_cast<float>(x) + 0.0f;  // Beta: float x2 = x + 0 (TileRenderer.java:619)
	float x3 = static_cast<float>(x) + 0.0f;  // Beta: float x3 = x + 0 (TileRenderer.java:620)
	float z0 = static_cast<float>(z) + 0.0f;  // Beta: float z0 = z + 0 (TileRenderer.java:621)
	float z1 = static_cast<float>(z) + 1.0f;  // Beta: float z1 = z + 1 (TileRenderer.java:622)
	float z2 = static_cast<float>(z) + 1.0f;  // Beta: float z2 = z + 1 (TileRenderer.java:623)
	float z3 = static_cast<float>(z) + 0.0f;  // Beta: float z3 = z + 0 (TileRenderer.java:624)
	float y0 = static_cast<float>(y) + r;  // Beta: float y0 = y + r (TileRenderer.java:625)
	float y1 = static_cast<float>(y) + r;  // Beta: float y1 = y + r (TileRenderer.java:626)
	float y2 = static_cast<float>(y) + r;  // Beta: float y2 = y + r (TileRenderer.java:627)
	float y3 = static_cast<float>(y) + r;  // Beta: float y3 = y + r (TileRenderer.java:628)
	
	// Beta: Adjust vertex positions based on rail data value (TileRenderer.java:629-644)
	if (data == 1 || data == 2 || data == 3 || data == 7)  // Beta: if (data == 1 || data == 2 || data == 3 || data == 7) (TileRenderer.java:629)
	{
		x0 = x3 = static_cast<float>(x) + 1.0f;  // Beta: x0 = x3 = x + 1 (TileRenderer.java:630)
		x1 = x2 = static_cast<float>(x) + 0.0f;  // Beta: x1 = x2 = x + 0 (TileRenderer.java:631)
		z0 = z1 = static_cast<float>(z) + 1.0f;  // Beta: z0 = z1 = z + 1 (TileRenderer.java:632)
		z2 = z3 = static_cast<float>(z) + 0.0f;  // Beta: z2 = z3 = z + 0 (TileRenderer.java:633)
	}
	else if (data == 8)  // Beta: else if (data == 8) (TileRenderer.java:634)
	{
		x0 = x1 = static_cast<float>(x) + 0.0f;  // Beta: x0 = x1 = x + 0 (TileRenderer.java:635)
		x2 = x3 = static_cast<float>(x) + 1.0f;  // Beta: x2 = x3 = x + 1 (TileRenderer.java:636)
		z0 = z3 = static_cast<float>(z) + 1.0f;  // Beta: z0 = z3 = z + 1 (TileRenderer.java:637)
		z1 = z2 = static_cast<float>(z) + 0.0f;  // Beta: z1 = z2 = z + 0 (TileRenderer.java:638)
	}
	else if (data == 9)  // Beta: else if (data == 9) (TileRenderer.java:639)
	{
		x0 = x3 = static_cast<float>(x) + 0.0f;  // Beta: x0 = x3 = x + 0 (TileRenderer.java:640)
		x1 = x2 = static_cast<float>(x) + 1.0f;  // Beta: x1 = x2 = x + 1 (TileRenderer.java:641)
		z0 = z1 = static_cast<float>(z) + 0.0f;  // Beta: z0 = z1 = z + 0 (TileRenderer.java:642)
		z2 = z3 = static_cast<float>(z) + 1.0f;  // Beta: z2 = z3 = z + 1 (TileRenderer.java:643)
	}

	// Beta: Adjust Y positions for sloped rails (TileRenderer.java:646-652)
	if (data == 2 || data == 4)  // Beta: if (data == 2 || data == 4) (TileRenderer.java:646)
	{
		y0 += 1.0f;  // Beta: y0++ (TileRenderer.java:647)
		y3 += 1.0f;  // Beta: y3++ (TileRenderer.java:648)
	}
	else if (data == 3 || data == 5)  // Beta: else if (data == 3 || data == 5) (TileRenderer.java:649)
	{
		y1 += 1.0f;  // Beta: y1++ (TileRenderer.java:650)
		y2 += 1.0f;  // Beta: y2++ (TileRenderer.java:651)
	}

	// Beta: Render quad (two triangles) (TileRenderer.java:654-662)
	t.vertexUV(x0, y0, z0, u1, v0);  // Beta: t.vertexUV(x0, y0, z0, u1, v0) (TileRenderer.java:654)
	t.vertexUV(x1, y1, z1, u1, v1);  // Beta: t.vertexUV(x1, y1, z1, u1, v1) (TileRenderer.java:655)
	t.vertexUV(x2, y2, z2, u0, v1);  // Beta: t.vertexUV(x2, y2, z2, u0, v1) (TileRenderer.java:656)
	t.vertexUV(x3, y3, z3, u0, v0);  // Beta: t.vertexUV(x3, y3, z3, u0, v0) (TileRenderer.java:657)
	
	// Beta: Render back face (TileRenderer.java:658-661)
	t.vertexUV(x3, y3, z3, u0, v0);  // Beta: t.vertexUV(x3, y3, z3, u0, v0) (TileRenderer.java:658)
	t.vertexUV(x2, y2, z2, u0, v1);  // Beta: t.vertexUV(x2, y2, z2, u0, v1) (TileRenderer.java:659)
	t.vertexUV(x1, y1, z1, u1, v1);  // Beta: t.vertexUV(x1, y1, z1, u1, v1) (TileRenderer.java:660)
	t.vertexUV(x0, y0, z0, u1, v0);  // Beta: t.vertexUV(x0, y0, z0, u1, v0) (TileRenderer.java:661)
	
	return true;  // Beta: return true (TileRenderer.java:662)
}

bool TileRenderer::tesselateFenceInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta: TileRenderer.tesselateFenceInWorld() - renders fence with center post and connections (TileRenderer.java:1299-1355)
	bool changed = false;
	float a = 0.375F;  // Beta: float a = 0.375F (TileRenderer.java:1301)
	float b = 0.625F;  // Beta: float b = 0.625F (TileRenderer.java:1302)
	tt.setShape(a, 0.0F, a, b, 1.0F, b);  // Beta: tt.setShape(a, 0.0F, a, b, 1.0F, b) (TileRenderer.java:1303)
	tesselateBlockInWorld(tt, x, y, z);  // Beta: this.tesselateBlockInWorld(tt, x, y, z) (TileRenderer.java:1304)
	
	bool vertical = false;  // Beta: boolean vertical = false (TileRenderer.java:1305)
	bool horizontal = false;  // Beta: boolean horizontal = false (TileRenderer.java:1306)
	if (level->getTile(x - 1, y, z) == tt.id || level->getTile(x + 1, y, z) == tt.id)  // Beta: if (this.level.getTile(x - 1, y, z) == tt.id || this.level.getTile(x + 1, y, z) == tt.id) (TileRenderer.java:1307)
	{
		vertical = true;  // Beta: vertical = true (TileRenderer.java:1308)
	}
	
	if (level->getTile(x, y, z - 1) == tt.id || level->getTile(x, y, z + 1) == tt.id)  // Beta: if (this.level.getTile(x, y, z - 1) == tt.id || this.level.getTile(x, y, z + 1) == tt.id) (TileRenderer.java:1311)
	{
		horizontal = true;  // Beta: horizontal = true (TileRenderer.java:1312)
	}
	
	bool l = level->getTile(x - 1, y, z) == tt.id;  // Beta: boolean l = this.level.getTile(x - 1, y, z) == tt.id (TileRenderer.java:1315)
	bool r = level->getTile(x + 1, y, z) == tt.id;  // Beta: boolean r = this.level.getTile(x + 1, y, z) == tt.id (TileRenderer.java:1316)
	bool u = level->getTile(x, y, z - 1) == tt.id;  // Beta: boolean u = this.level.getTile(x, y, z - 1) == tt.id (TileRenderer.java:1317)
	bool d = level->getTile(x, y, z + 1) == tt.id;  // Beta: boolean d = this.level.getTile(x, y, z + 1) == tt.id (TileRenderer.java:1318)
	if (!vertical && !horizontal)  // Beta: if (!vertical && !horizontal) (TileRenderer.java:1319)
	{
		vertical = true;  // Beta: vertical = true (TileRenderer.java:1320)
	}
	
	a = 0.4375F;  // Beta: a = 0.4375F (TileRenderer.java:1323)
	b = 0.5625F;  // Beta: b = 0.5625F (TileRenderer.java:1324)
	float h0 = 0.75F;  // Beta: float h0 = 0.75F (TileRenderer.java:1325)
	float h1 = 0.9375F;  // Beta: float h1 = 0.9375F (TileRenderer.java:1326)
	float x0 = l ? 0.0F : a;  // Beta: float x0 = l ? 0.0F : a (TileRenderer.java:1327)
	float x1 = r ? 1.0F : b;  // Beta: float x1 = r ? 1.0F : b (TileRenderer.java:1328)
	float z0 = u ? 0.0F : a;  // Beta: float z0 = u ? 0.0F : a (TileRenderer.java:1329)
	float z1 = d ? 1.0F : b;  // Beta: float z1 = d ? 1.0F : b (TileRenderer.java:1330)
	if (vertical)  // Beta: if (vertical) (TileRenderer.java:1331)
	{
		tt.setShape(x0, h0, a, x1, h1, b);  // Beta: tt.setShape(x0, h0, a, x1, h1, b) (TileRenderer.java:1332)
		tesselateBlockInWorld(tt, x, y, z);  // Beta: this.tesselateBlockInWorld(tt, x, y, z) (TileRenderer.java:1333)
	}
	
	if (horizontal)  // Beta: if (horizontal) (TileRenderer.java:1336)
	{
		tt.setShape(a, h0, z0, b, h1, z1);  // Beta: tt.setShape(a, h0, z0, b, h1, z1) (TileRenderer.java:1337)
		tesselateBlockInWorld(tt, x, y, z);  // Beta: this.tesselateBlockInWorld(tt, x, y, z) (TileRenderer.java:1338)
	}
	
	h0 = 0.375F;  // Beta: h0 = 0.375F (TileRenderer.java:1341)
	h1 = 0.5625F;  // Beta: h1 = 0.5625F (TileRenderer.java:1342)
	if (vertical)  // Beta: if (vertical) (TileRenderer.java:1343)
	{
		tt.setShape(x0, h0, a, x1, h1, b);  // Beta: tt.setShape(x0, h0, a, x1, h1, b) (TileRenderer.java:1344)
		tesselateBlockInWorld(tt, x, y, z);  // Beta: this.tesselateBlockInWorld(tt, x, y, z) (TileRenderer.java:1345)
	}
	
	if (horizontal)  // Beta: if (horizontal) (TileRenderer.java:1348)
	{
		tt.setShape(a, h0, z0, b, h1, z1);  // Beta: tt.setShape(a, h0, z0, b, h1, z1) (TileRenderer.java:1349)
		tesselateBlockInWorld(tt, x, y, z);  // Beta: this.tesselateBlockInWorld(tt, x, y, z) (TileRenderer.java:1350)
	}
	
	tt.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);  // Beta: tt.setShape(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F) (TileRenderer.java:1353)
	return changed;  // Beta: return changed (TileRenderer.java:1354)
}

bool TileRenderer::tesselateLeverInWorld(Tile &tt, int_t x, int_t y, int_t z)
{
	// Beta 1.2: TileRenderer.tesselateLeverInWorld() - full lever rendering implementation
	// Reference: beta1.2/net/minecraft/client/renderer/TileRenderer.java:100-261
	int_t data = level->getData(x, y, z);
	int_t dir = data & 7;
	bool flipped = (data & 8) > 0;
	Tesselator &t = Tesselator::instance;
	bool hadFixed = fixedTexture >= 0;
	if (!hadFixed)
	{
		// Beta: this.fixedTexture = Tile.stoneBrick.tex (TileRenderer.java:107)
		fixedTexture = Tile::cobblestone.tex;  // Use cobblestone texture for lever base
	}

	// Beta: Set base shape based on direction (TileRenderer.java:110-125)
	float w1 = 0.25F;
	float w2 = 0.1875F;
	float h = 0.1875F;
	if (dir == 5)
	{
		tt.setShape(0.5F - w2, 0.0F, 0.5F - w1, 0.5F + w2, h, 0.5F + w1);
	}
	else if (dir == 6)
	{
		tt.setShape(0.5F - w1, 0.0F, 0.5F - w2, 0.5F + w1, h, 0.5F + w2);
	}
	else if (dir == 4)
	{
		tt.setShape(0.5F - w2, 0.5F - w1, 1.0F - h, 0.5F + w2, 0.5F + w1, 1.0F);
	}
	else if (dir == 3)
	{
		tt.setShape(0.5F - w2, 0.5F - w1, 0.0F, 0.5F + w2, 0.5F + w1, h);
	}
	else if (dir == 2)
	{
		tt.setShape(1.0F - h, 0.5F - w1, 0.5F - w2, 1.0F, 0.5F + w1, 0.5F + w2);
	}
	else if (dir == 1)
	{
		tt.setShape(0.0F, 0.5F - w1, 0.5F - w2, h, 0.5F + w1, 0.5F + w2);
	}

	// Beta: Render base block (TileRenderer.java:127)
	tesselateBlockInWorld(tt, x, y, z);
	if (!hadFixed)
	{
		fixedTexture = -1;
	}

	// Beta: Get brightness and set color (TileRenderer.java:132-137)
	float br = tt.getBrightness(*level, x, y, z);
	if (Tile::lightEmission[tt.id] > 0)
	{
		br = 1.0F;
	}
	t.color(br, br, br);

	// Beta: Get texture (TileRenderer.java:138-141)
	int_t tex = tt.getTexture(Facing::DOWN);
	if (fixedTexture >= 0)
	{
		tex = fixedTexture;
	}

	// Beta: Calculate texture coordinates (TileRenderer.java:143-148)
	int_t xt = (tex & 15) << 4;
	int_t yt = tex & 240;
	float u0 = static_cast<float>(xt) / 256.0F;
	float u1 = static_cast<float>(xt + 15.99F) / 256.0F;
	float v0 = static_cast<float>(yt) / 256.0F;
	float v1 = static_cast<float>(yt + 15.99F) / 256.0F;

	// Beta: Create 8 corner vertices (TileRenderer.java:149-160)
	Vec3 *corners[8];
	float xv = 0.0625F;
	float zv = 0.0625F;
	float yv = 0.625F;
	corners[0] = Vec3::newTemp(-xv, 0.0, -zv);
	corners[1] = Vec3::newTemp(xv, 0.0, -zv);
	corners[2] = Vec3::newTemp(xv, 0.0, zv);
	corners[3] = Vec3::newTemp(-xv, 0.0, zv);
	corners[4] = Vec3::newTemp(-xv, yv, -zv);
	corners[5] = Vec3::newTemp(xv, yv, -zv);
	corners[6] = Vec3::newTemp(xv, yv, zv);
	corners[7] = Vec3::newTemp(-xv, yv, zv);

	// Beta: Transform corners based on flipped state and direction (TileRenderer.java:162-201)
	for (int i = 0; i < 8; i++)
	{
		if (flipped)
		{
			corners[i]->z -= 0.0625;
			corners[i]->xRot(static_cast<float>(Mth::PI * 2.0F / 9.0F));
		}
		else
		{
			corners[i]->z += 0.0625;
			corners[i]->xRot(static_cast<float>(-Mth::PI * 2.0F / 9.0F));
		}

		if (dir == 6)
		{
			corners[i]->yRot(static_cast<float>(Mth::PI / 2));
		}

		if (dir < 5)
		{
			corners[i]->y -= 0.375;
			corners[i]->xRot(static_cast<float>(Mth::PI / 2));
			if (dir == 4)
			{
				corners[i]->yRot(0.0F);
			}
			if (dir == 3)
			{
				corners[i]->yRot(static_cast<float>(Mth::PI));
			}
			if (dir == 2)
			{
				corners[i]->yRot(static_cast<float>(Mth::PI / 2));
			}
			if (dir == 1)
			{
				corners[i]->yRot(static_cast<float>(-Mth::PI / 2));
			}

			corners[i]->x += x + 0.5;
			corners[i]->y += y + 0.5F;
			corners[i]->z += z + 0.5;
		}
		else
		{
			corners[i]->x += x + 0.5;
			corners[i]->y += y + 0.125F;
			corners[i]->z += z + 0.5;
		}
	}

	// Beta: Render 6 faces using corner vertices (TileRenderer.java:204-258)
	Vec3 *c0 = nullptr;
	Vec3 *c1 = nullptr;
	Vec3 *c2 = nullptr;
	Vec3 *c3 = nullptr;

	for (int i = 0; i < 6; i++)
	{
		// Beta: Adjust texture coordinates for specific faces (TileRenderer.java:210-220)
		if (i == 0)
		{
			u0 = static_cast<float>(xt + 7) / 256.0F;
			u1 = static_cast<float>(xt + 9 - 0.01F) / 256.0F;
			v0 = static_cast<float>(yt + 6) / 256.0F;
			v1 = static_cast<float>(yt + 8 - 0.01F) / 256.0F;
		}
		else if (i == 2)
		{
			u0 = static_cast<float>(xt + 7) / 256.0F;
			u1 = static_cast<float>(xt + 9 - 0.01F) / 256.0F;
			v0 = static_cast<float>(yt + 6) / 256.0F;
			v1 = static_cast<float>(yt + 16 - 0.01F) / 256.0F;
		}

		// Beta: Select corners for each face (TileRenderer.java:222-252)
		if (i == 0)
		{
			c0 = corners[0];
			c1 = corners[1];
			c2 = corners[2];
			c3 = corners[3];
		}
		else if (i == 1)
		{
			c0 = corners[7];
			c1 = corners[6];
			c2 = corners[5];
			c3 = corners[4];
		}
		else if (i == 2)
		{
			c0 = corners[1];
			c1 = corners[0];
			c2 = corners[4];
			c3 = corners[5];
		}
		else if (i == 3)
		{
			c0 = corners[2];
			c1 = corners[1];
			c2 = corners[5];
			c3 = corners[6];
		}
		else if (i == 4)
		{
			c0 = corners[3];
			c1 = corners[2];
			c2 = corners[6];
			c3 = corners[7];
		}
		else if (i == 5)
		{
			c0 = corners[0];
			c1 = corners[3];
			c2 = corners[7];
			c3 = corners[4];
		}

		// Beta: Render face (TileRenderer.java:254-257)
		t.vertexUV(c0->x, c0->y, c0->z, u0, v1);
		t.vertexUV(c1->x, c1->y, c1->z, u1, v1);
		t.vertexUV(c2->x, c2->y, c2->z, u1, v0);
		t.vertexUV(c3->x, c3->y, c3->z, u0, v0);
	}

	return true;
}
