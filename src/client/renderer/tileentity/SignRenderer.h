#pragma once

#include "client/renderer/tileentity/TileEntityRenderer.h"
#include "client/model/SignModel.h"

class SignTileEntity;
class Font;

// Beta 1.2: SignRenderer.java
class SignRenderer : public TileEntityRenderer
{
private:
	SignModel signModel;
	
	// Performance: Rebuild budget to avoid stalls when many signs load at once
	// Rebuild at most MAX_REBUILDS_PER_FRAME sign textures per frame
	static constexpr int_t MAX_REBUILDS_PER_FRAME = 8;
	static int_t rebuildsThisFrame;
	static int_t lastFrameNumber;  // Track frame number to reset counter
	
	// Performance: Pre-built VBO for sign text quad (OpenGL 4.6 optimization)
	// Replaces immediate mode glBegin/glEnd for faster rendering
	static GLuint quadVBO;
	static bool quadVBOInitialized;
	static void initQuadVBO();
	
	// Bake sign text into a texture (CPU-side rendering)
	// Returns true if texture was successfully created/updated
	bool bakeSignTexture(SignTileEntity &sign, Font *font);

public:
	SignRenderer();
	
	// Beta: SignRenderer.render() (SignRenderer.java:12-68)
	void render(SignTileEntity &sign, double x, double y, double z, float a);
	
	// Helper to render from TileEntity*
	void renderEntity(TileEntity *entity, double x, double y, double z, float a);
};
