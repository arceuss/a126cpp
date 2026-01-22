#pragma once

#include "client/renderer/tileentity/TileEntityRenderer.h"
#include "client/model/SignModel.h"

class SignTileEntity;

// Beta 1.2: SignRenderer.java
class SignRenderer : public TileEntityRenderer
{
private:
	SignModel signModel;

public:
	SignRenderer();
	
	// Beta: SignRenderer.render() (SignRenderer.java:12-68)
	void render(SignTileEntity &sign, double x, double y, double z, float a);
	
	// Helper to render from TileEntity*
	void renderEntity(TileEntity *entity, double x, double y, double z, float a);
};
