#pragma once

#include "client/renderer/TextureFX.h"

// Beta 1.2: FireTexture.java - ported 1:1 to TextureFX
// Note: Beta 1.2 uses DynamicTexture, but we use TextureFX (Alpha 1.2.6 style)
class TextureFireFX : public TextureFX
{
private:
	std::vector<float> current;  // Beta: protected float[] current = new float[320]
	std::vector<float> next;     // Beta: protected float[] next = new float[320]

public:
	TextureFireFX(int_t id);  // Beta: FireTexture(int id) - id 0 or 1

	void func_783_a() override;  // Beta: void tick() - update fire animation
};
