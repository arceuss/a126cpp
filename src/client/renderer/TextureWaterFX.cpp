#include "client/renderer/TextureWaterFX.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include <cmath>
#include <cstdlib>

// Beta 1.2: WaterTexture.java - ported 1:1
TextureWaterFX::TextureWaterFX() : TextureFX(Tile::water.tex)
{
	// Beta: WaterTexture() calls super(Tile.water.tex)
	// Beta 1.2: Tile.water = ID 8 (LiquidTileDynamic)
	// Beta: Initialize arrays
	current.resize(256, 0.0f);
	next.resize(256, 0.0f);
	heat.resize(256, 0.0f);
	heata.resize(256, 0.0f);
	tickCount = 0;
}

void TextureWaterFX::func_783_a()
{
	// Beta: WaterTexture.tick() - complete 1:1 port
	++tickCount;
	
	// Beta: lines 20-32 - Calculate next from current
	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			float pow = 0.0F;
			
			// Beta: lines 24-28 - Average neighbors (wrapping around)
			for (int_t xx = x - 1; xx <= x + 1; ++xx)
			{
				int_t xi = xx & 15;  // Wrap around (mod 16)
				int_t yi = y & 15;
				pow += current[xi + yi * 16];
			}
			
			// Beta: line 30
			next[x + y * 16] = pow / 3.3F + heat[x + y * 16] * 0.8F;
		}
	}
	
	// Beta: lines 34-46 - Update heat and heata
	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			// Beta: line 36
			heat[x + y * 16] = heat[x + y * 16] + heata[x + y * 16] * 0.05F;
			if (heat[x + y * 16] < 0.0F)
			{
				heat[x + y * 16] = 0.0F;
			}
			
			// Beta: line 41
			heata[x + y * 16] = heata[x + y * 16] - 0.1F;
			// Beta: line 42 - Math.random() < 0.05
			if (static_cast<double>(rand()) / RAND_MAX < 0.05)
			{
				heata[x + y * 16] = 0.5F;
			}
		}
	}
	
	// Beta: lines 48-50 - Swap next and current
	std::vector<float> tmp = next;
	next = current;
	current = tmp;
	
	// Beta: lines 52-80 - Convert current to RGBA bytes
	for (int_t i = 0; i < 256; ++i)
	{
		float pow = current[i];
		if (pow > 1.0F)
		{
			pow = 1.0F;
		}
		
		if (pow < 0.0F)
		{
			pow = 0.0F;
		}
		
		// Beta: line 62
		float pp = pow * pow;
		// Beta: lines 63-66
		int_t r = static_cast<int_t>(32.0F + pp * 32.0F);
		int_t g = static_cast<int_t>(50.0F + pp * 64.0F);
		int_t b = 255;
		int_t a = static_cast<int_t>(146.0F + pp * 50.0F);
		
		// Beta: lines 67-74 - Anaglyph mode
		if (field_1131_c)
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;
			int_t gg = (r * 30 + g * 70) / 100;
			int_t bb = (r * 30 + b * 70) / 100;
			r = rr;
			g = gg;
			b = bb;
		}
		
		// Beta: lines 76-79
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);
	}
}
