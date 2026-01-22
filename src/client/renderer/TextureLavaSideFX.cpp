#include "client/renderer/TextureLavaSideFX.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "util/Mth.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI  3.14159265358979323846f
#endif

// Beta 1.2: LavaSideTexture.java - ported 1:1
TextureLavaSideFX::TextureLavaSideFX() : TextureFX(Tile::lava.tex + 1)
{
	// Beta: LavaSideTexture() calls super(Tile.lava.tex + 1)
	// Beta 1.2: Tile.lava.tex + 1 = side texture for lava
	// Beta: Initialize arrays
	current.resize(256, 0.0f);
	next.resize(256, 0.0f);
	heat.resize(256, 0.0f);
	heata.resize(256, 0.0f);
	tickCount = 0;
	
	// Beta: this.replicate = 2 (line 15)
	// Note: replicate is field_1129_e in Alpha (tile size multiplier)
	field_1129_e = 2;
}

void TextureLavaSideFX::func_783_a()
{
	// Beta: LavaSideTexture.tick() - complete 1:1 port
	++tickCount;
	
	// Beta: lines 22-54 - Same calculation as LavaTexture
	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			float pow = 0.0F;
			int_t xxo = static_cast<int_t>(Mth::sin(y * M_PI * 2.0F / 16.0F) * 1.2F);
			int_t yyo = static_cast<int_t>(Mth::sin(x * M_PI * 2.0F / 16.0F) * 1.2F);
			
			for (int_t xx = x - 1; xx <= x + 1; ++xx)
			{
				for (int_t yy = y - 1; yy <= y + 1; ++yy)
				{
					int_t xi = (xx + xxo) & 15;
					int_t yi = (yy + yyo) & 15;
					pow += current[xi + yi * 16];
				}
			}
			
			next[x + y * 16] = pow / 10.0F
				+ (
					heat[((x + 0) & 15) + ((y + 0) & 15) * 16]
					+ heat[((x + 1) & 15) + ((y + 0) & 15) * 16]
					+ heat[((x + 1) & 15) + ((y + 1) & 15) * 16]
					+ heat[((x + 0) & 15) + ((y + 1) & 15) * 16]
				) / 4.0F * 0.8F;
			
			heat[x + y * 16] = heat[x + y * 16] + heata[x + y * 16] * 0.01F;
			if (heat[x + y * 16] < 0.0F)
			{
				heat[x + y * 16] = 0.0F;
			}
			
			heata[x + y * 16] = heata[x + y * 16] - 0.06F;
			if (static_cast<double>(rand()) / RAND_MAX < 0.005)
			{
				heata[x + y * 16] = 1.5F;
			}
		}
	}
	
	// Beta: lines 53-55 - Swap next and current
	std::vector<float> tmp = next;
	next = current;
	current = tmp;
	
	// Beta: lines 61-87 - Convert current to RGBA bytes with scrolling
	for (int_t i = 0; i < 256; ++i)
	{
		// Beta: line 62 - Scroll based on tickCount
		float pow = current[(i - tickCount / 3 * 16) & 0xFF] * 2.0F;
		if (pow > 1.0F)
		{
			pow = 1.0F;
		}
		
		if (pow < 0.0F)
		{
			pow = 0.0F;
		}
		
		int_t r = static_cast<int_t>(pow * 100.0F + 155.0F);
		int_t g = static_cast<int_t>(pow * pow * 255.0F);
		int_t b = static_cast<int_t>(pow * pow * pow * pow * 128.0F);
		
		if (field_1131_c)
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;
			int_t gg = (r * 30 + g * 70) / 100;
			int_t bb = (r * 30 + b * 70) / 100;
			r = rr;
			g = gg;
			b = bb;
		}
		
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(-1);
	}
}
