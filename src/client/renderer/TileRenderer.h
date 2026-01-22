#pragma once

#include "world/level/LevelSource.h"
#include "world/level/tile/Tile.h"

#include "java/Type.h"

class Level;

class TileRenderer
{
private:
	LevelSource *level = nullptr;
	int_t fixedTexture = -1;

	bool xFlipTexture = false;
	bool noCulling = false;

public:
	TileRenderer(LevelSource *levelSource);
	TileRenderer();

	void tesselateInWorld(Tile &tile, int_t x, int_t y, int_t z, int_t fixedTexture);
	void tesselateInWorldNoCulling(Tile &tile, int_t x, int_t y, int_t z);
	bool tesselateInWorld(Tile &tt, int_t x, int_t y, int_t z);

	bool tesselateBlockInWorld(Tile &tt, int_t x, int_t y, int_t z);
	bool tesselateBlockInWorld(Tile &tt, int_t x, int_t y, int_t z, float r, float g, float b);
	
	// newb12: TileRenderer.renderBlock() - renders block at -0.5 offset (for falling blocks)
	// Reference: newb12/net/minecraft/client/renderer/TileRenderer.java:1045-1096
	void renderBlock(Tile &tt, Level &level, int_t x, int_t y, int_t z);

	// Alpha: RenderBlocks.renderBlockFluids() - renders water/lava with varying heights
	bool tesselateWaterInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: RenderBlocks.renderCrossTexture() - renders cross-texture blocks (flowers, plants)
	bool tesselateCrossTextureInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Alpha: RenderBlocks.func_1230_b() - renders cactus with custom model (shrunk sides)
	bool tesselateCactusInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateTorchInWorld() - renders torch blocks
	bool tesselateTorchInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateDoorInWorld() - renders door blocks
	bool tesselateDoorInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateLadderInWorld() - renders ladder blocks
	bool tesselateLadderInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateStairsInWorld() - renders stairs blocks
	bool tesselateStairsInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateFireInWorld() - renders fire blocks
	bool tesselateFireInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateDustInWorld() - renders redstone dust
	bool tesselateDustInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateRowInWorld() - renders row texture blocks
	bool tesselateRowInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateRailInWorld() - renders rail blocks
	bool tesselateRailInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateFenceInWorld() - renders fence blocks
	bool tesselateFenceInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateLeverInWorld() - renders lever blocks
	bool tesselateLeverInWorld(Tile &tt, int_t x, int_t y, int_t z);
	
	// Beta: TileRenderer.tesselateTorch() - helper for torch rendering
	void tesselateTorch(Tile &tt, double x, double y, double z, double xxa, double zza);

private:
	// Alpha: RenderBlocks.func_1224_a() - calculates corner height by averaging neighbors
	float calculateCornerHeight(int_t x, int_t y, int_t z, const Material &material);

	// Alpha: RenderBlocks.renderBottomFace() - renders bottom face of block
	void renderBottomFace(Tile &tt, double x, double y, double z, int_t tex);

public:
	void renderFaceUp(Tile &tt, double x, double y, double z, int_t tex);
	void renderFaceDown(Tile &tt, double x, double y, double z, int_t tex);
	void renderNorth(Tile &tt, double x, double y, double z, int_t tex);
	void renderSouth(Tile &tt, double x, double y, double z, int_t tex);
	void renderWest(Tile &tt, double x, double y, double z, int_t tex);
	void renderEast(Tile &tt, double x, double y, double z, int_t tex);

	void renderCube(Tile &tile, float alpha);
	
	// Beta: TileRenderer.renderTile() - renders tile at current position with data
	void renderTile(Tile &tile, int_t data);
	
	// Beta: TileRenderer.canRender() - static check if renderShape can be rendered
	static bool canRender(int_t renderShape);
};
