#pragma once

#include "client/renderer/TextureFX.h"
#include <vector>
#include <memory>

class Minecraft;

// Beta 1.2: CompassTexture.java - ported 1:1 to TextureFX (uses proper variable names)
class TextureCompassFX : public TextureFX
{
private:
	Minecraft* mc;  // Beta: private Minecraft mc
	std::vector<int_t> raw;  // Beta: private int[] raw = new int[256]
	double rot;  // Beta: private double rot
	double rota;  // Beta: private double rota

public:
	TextureCompassFX(Minecraft* mc);

	void func_783_a() override;  // Beta: void tick() - update compass animation
};