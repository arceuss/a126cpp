#include "client/renderer/TextureLavaFX.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "util/Mth.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI  3.14159265358979323846f
#endif

// Beta 1.2: LavaTexture.java - ported 1:1
TextureLavaFX::TextureLavaFX() : TextureFX(Tile::lava.tex)
{
	// Beta: LavaTexture() calls super(Tile.lava.tex)
	// Beta 1.2: Tile.lava = ID 10 (LiquidTileDynamic)
	// Beta: Initialize arrays
	current.resize(256, 0.0f);
	next.resize(256, 0.0f);
	heat.resize(256, 0.0f);
	heata.resize(256, 0.0f);
}

void TextureLavaFX::func_783_a()
{
	// Beta: LavaTexture.tick() - complete 1:1 port
	// Beta: lines 18-51 - Calculate next from current with heat
	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 16; ++y)
		{
			float pow = 0.0F;
			// Beta: lines 21-22 - Sin offsets
			int_t xxo = static_cast<int_t>(Mth::sin(y * M_PI * 2.0F / 16.0F) * 1.2F);
			int_t yyo = static_cast<int_t>(Mth::sin(x * M_PI * 2.0F / 16.0F) * 1.2F);
			
			// Beta: lines 24-30 - Average neighbors with wrapping
			for (int_t xx = x - 1; xx <= x + 1; ++xx)
			{
				for (int_t yy = y - 1; yy <= y + 1; ++yy)
				{
					int_t xi = (xx + xxo) & 15;  // Wrap around (mod 16)
					int_t yi = (yy + yyo) & 15;
					pow += current[xi + yi * 16];
				}
			}
			
			// Beta: lines 32-40
			next[x + y * 16] = pow / 10.0F
				+ (
					heat[((x + 0) & 15) + ((y + 0) & 15) * 16]
					+ heat[((x + 1) & 15) + ((y + 0) & 15) * 16]
					+ heat[((x + 1) & 15) + ((y + 1) & 15) * 16]
					+ heat[((x + 0) & 15) + ((y + 1) & 15) * 16]
				) / 4.0F * 0.8F;
			
			// Beta: lines 41-44
			heat[x + y * 16] = heat[x + y * 16] + heata[x + y * 16] * 0.01F;
			if (heat[x + y * 16] < 0.0F)
			{
				heat[x + y * 16] = 0.0F;
			}
			
			// Beta: lines 46-49
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
	
	// Beta: lines 57-83 - Convert current to RGBA bytes
	for (int_t i = 0; i < 256; ++i)
	{
		float pow = current[i] * 2.0F;
		if (pow > 1.0F)
		{
			pow = 1.0F;
		}
		
		if (pow < 0.0F)
		{
			pow = 0.0F;
		}
		
		// Beta: lines 67-69
		int_t r = static_cast<int_t>(pow * 100.0F + 155.0F);
		int_t g = static_cast<int_t>(pow * pow * 255.0F);
		int_t b = static_cast<int_t>(pow * pow * pow * pow * 128.0F);
		
		// Beta: lines 70-77 - Anaglyph mode
		if (field_1131_c)
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;
			int_t gg = (r * 30 + g * 70) / 100;
			int_t bb = (r * 30 + b * 70) / 100;
			r = rr;
			g = gg;
			b = bb;
		}
		
		// Beta: lines 79-82
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(-1);  // -1 = 255
	}
}
