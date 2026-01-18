#pragma once

#include "client/renderer/TextureFX.h"
#include <vector>

// Beta 1.2: PortalTexture.java - ported 1:1 to TextureFX
// Note: Portals don't exist in Alpha 1.2.6, but porting for Beta 1.2 compatibility
class TexturePortalFX : public TextureFX
{
private:
	int_t time = 0;  // Beta: private int time = 0
	std::vector<std::vector<byte_t>> frames;  // Beta: private byte[][] frames = new byte[32][1024]

public:
	TexturePortalFX();

	void func_783_a() override;  // Beta: void tick() - update portal animation
};
