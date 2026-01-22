#pragma once

#include "client/renderer/TextureFX.h"

// Beta 1.2: WaterSideTexture.java - ported 1:1
class TextureWaterSideFX : public TextureFX
{
private:
	std::vector<float> current;  // Beta: protected float[] current = new float[256]
	std::vector<float> next;     // Beta: protected float[] next = new float[256]
	std::vector<float> heat;      // Beta: protected float[] heat = new float[256]
	std::vector<float> heata;    // Beta: protected float[] heata = new float[256]
	int_t tickCount = 0;          // Beta: private int tickCount = 0

public:
	TextureWaterSideFX();

	void func_783_a() override;  // Beta: void tick() - update flowing water animation
};
