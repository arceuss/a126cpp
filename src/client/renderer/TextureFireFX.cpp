#include "client/renderer/TextureFireFX.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI  3.14159265358979323846f
#endif

// Beta 1.2: FireTexture.java - ported 1:1 to TextureFX
// Beta 1.2: Tile.fire.tex = 31 (Tile.java:172)
TextureFireFX::TextureFireFX(int_t id) : TextureFX(31 + id * 16)
{
	// Beta: FireTexture(int id) calls super(Tile.fire.tex + id * 16)
	// Beta 1.2: Tile.fire.tex = 31
	// Beta: Initialize arrays
	current.resize(320, 0.0f);  // Beta: protected float[] current = new float[320]
	next.resize(320, 0.0f);     // Beta: protected float[] next = new float[320]
}

void TextureFireFX::func_783_a()
{
	// Beta: FireTexture.tick() - complete 1:1 port
	// Beta: lines 15-35 - Calculate next from current with diffusion
	for (int_t x = 0; x < 16; ++x)
	{
		for (int_t y = 0; y < 20; ++y)
		{
			int_t count = 18;
			float pow = current[x + (y + 1) % 20 * 16] * count;  // Beta: line 18

			// Beta: lines 20-28 - Average neighbors
			for (int_t xx = x - 1; xx <= x + 1; ++xx)
			{
				for (int_t yy = y; yy <= y + 1; ++yy)
				{
					if (xx >= 0 && yy >= 0 && xx < 16 && yy < 20)  // Beta: line 22
					{
						pow += current[xx + yy * 16];  // Beta: line 23
					}

					++count;  // Beta: line 26
				}
			}

			// Beta: line 30
			next[x + y * 16] = pow / (count * 1.06F);
			
			// Beta: lines 31-33 - Random noise at bottom (y >= 19)
			if (y >= 19)
			{
				// Beta: Math.random() * Math.random() * Math.random() * 4.0 + Math.random() * 0.1F + 0.2F
				float r1 = static_cast<float>(rand()) / RAND_MAX;
				float r2 = static_cast<float>(rand()) / RAND_MAX;
				float r3 = static_cast<float>(rand()) / RAND_MAX;
				float r4 = static_cast<float>(rand()) / RAND_MAX;
				next[x + y * 16] = r1 * r2 * r3 * 4.0f + r4 * 0.1f + 0.2f;
			}
		}
	}

	// Beta: lines 37-39 - Swap next and current
	std::vector<float> tmp = next;
	next = current;
	current = tmp;

	// Beta: lines 41-73 - Convert current to RGBA bytes
	for (int_t i = 0; i < 256; ++i)
	{
		float pow = current[i] * 1.8F;  // Beta: line 42
		if (pow > 1.0F)  // Beta: line 43
		{
			pow = 1.0F;  // Beta: line 44
		}

		if (pow < 0.0F)  // Beta: line 47
		{
			pow = 0.0F;  // Beta: line 48
		}

		// Beta: lines 51-54 - Calculate RGB
		int_t r = static_cast<int_t>(pow * 155.0F + 100.0F);
		int_t g = static_cast<int_t>(pow * pow * 255.0F);
		int_t b = static_cast<int_t>(pow * pow * pow * pow * pow * pow * pow * pow * pow * pow * 255.0F);
		int_t a = 255;  // Beta: line 54

		// Beta: lines 55-57 - Alpha based on intensity
		if (pow < 0.5F)
		{
			a = 0;  // Beta: line 56
		}

		// Beta: line 59 - var15 is calculated but not used in Beta 1.2
		// float var15 = (pow - 0.5F) * 2.0F;

		// Beta: lines 60-67 - Anaglyph mode
		if (field_1131_c)  // Beta: this.anaglyph3d
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;  // Beta: line 61
			int_t gg = (r * 30 + g * 70) / 100;  // Beta: line 62
			int_t bb = (r * 30 + b * 70) / 100;  // Beta: line 63
			r = rr;  // Beta: line 64
			g = gg;  // Beta: line 65
			b = bb;  // Beta: line 66
		}

		// Beta: lines 69-72 - Write to pixel buffer
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);  // Beta: this.pixels[i * 4 + 0] = (byte)r
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);  // Beta: this.pixels[i * 4 + 1] = (byte)g
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);  // Beta: this.pixels[i * 4 + 2] = (byte)b
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);  // Beta: this.pixels[i * 4 + 3] = (byte)a
	}
}
