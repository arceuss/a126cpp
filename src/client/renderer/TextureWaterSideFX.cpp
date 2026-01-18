#include "client/renderer/TextureWaterSideFX.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include <cmath>
#include <cstdlib>

// Beta 1.2: WaterSideTexture.java - ported 1:1
TextureWaterSideFX::TextureWaterSideFX() : TextureFX(Tile::water.tex + 1)
{
	// Beta: WaterSideTexture() calls super(Tile.water.tex + 1)
	// Beta 1.2: Tile.water.tex + 1 = side texture for water
	// Beta: Initialize arrays
	current.resize(256, 0.0f);
	next.resize(256, 0.0f);
	heat.resize(256, 0.0f);
	heata.resize(256, 0.0f);
	tickCount = 0;
	
	// Beta: this.replicate = 2 (line 14)
	// Note: replicate is field_1129_e in Alpha (tile size multiplier)
	field_1129_e = 2;
}

void TextureWaterSideFX::func_783_a()
{
	// Beta: WaterSideTexture.tick() - complete 1:1 port
	++tickCount;
	
	// Beta: lines 21-32 - Calculate next from current
	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			float pow = 0.0F;
			
			// Beta: lines 25-29 - Average neighbors (different pattern than top)
			for (int_t xx = y - 2; xx <= y; ++xx)
			{
				int_t xi = x & 15;
				int_t yi = xx & 15;
				pow += current[xi + yi * 16];
			}
			
			// Beta: line 31
			next[x + y * 16] = pow / 3.2F + heat[x + y * 16] * 0.8F;
		}
	}
	
	// Beta: lines 35-47 - Update heat and heata
	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			// Beta: line 37
			heat[x + y * 16] = heat[x + y * 16] + heata[x + y * 16] * 0.05F;
			if (heat[x + y * 16] < 0.0F)
			{
				heat[x + y * 16] = 0.0F;
			}
			
			// Beta: lines 42-45
			heata[x + y * 16] = heata[x + y * 16] - 0.3F;
			if (static_cast<double>(rand()) / RAND_MAX < 0.2)
			{
				heata[x + y * 16] = 0.5F;
			}
		}
	}
	
	// Beta: lines 49-51 - Swap next and current
	std::vector<float> tmp = next;
	next = current;
	current = tmp;
	
	// Beta: lines 53-81 - Convert current to RGBA bytes with scrolling
	for (int_t i = 0; i < 256; ++i)
	{
		// Beta: line 54 - Scroll based on tickCount
		float pow = current[(i - tickCount * 16) & 0xFF];
		if (pow > 1.0F)
		{
			pow = 1.0F;
		}
		
		if (pow < 0.0F)
		{
			pow = 0.0F;
		}
		
		// Beta: line 63
		float pp = pow * pow;
		// Beta: lines 64-67
		int_t r = static_cast<int_t>(32.0F + pp * 32.0F);
		int_t g = static_cast<int_t>(50.0F + pp * 64.0F);
		int_t b = 255;
		int_t a = static_cast<int_t>(146.0F + pp * 50.0F);
		
		// Beta: lines 68-75 - Anaglyph mode
		if (field_1131_c)
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;
			int_t gg = (r * 30 + g * 70) / 100;
			int_t bb = (r * 30 + b * 70) / 100;
			r = rr;
			g = gg;
			b = bb;
		}
		
		// Beta: lines 77-80
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);
	}
}
