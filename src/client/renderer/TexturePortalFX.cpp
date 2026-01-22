#include "client/renderer/TexturePortalFX.h"
#include "util/Mth.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI  3.14159265358979323846f
#endif

// Beta 1.2: PortalTexture.java - ported 1:1 to TextureFX
// Note: Portal tile not yet ported, using texture index 14 (Beta 1.2: Tile.portalTile.tex = 14)
TexturePortalFX::TexturePortalFX() : TextureFX(14)
{
	// Beta: PortalTexture() calls super(Tile.portalTile.tex)
	// Beta 1.2: Tile.portalTile.tex = 14
	// Beta: Initialize frames array
	frames.resize(32);  // Beta: byte[][] frames = new byte[32][1024]
	for (int_t i = 0; i < 32; ++i)
	{
		frames[i].resize(1024, 0);  // 16x16 RGBA = 1024 bytes
	}

	// Beta: lines 13-60 - Generate 32 animation frames
	// Beta: Random random = new Random(100L)
	srand(100L);  // Seed random for consistent generation

	for (int_t time = 0; time < 32; ++time)
	{
		for (int_t x = 0; x < 16; ++x)
		{
			for (int_t y = 0; y < 16; ++y)
			{
				float pow = 0.0F;  // Beta: line 18

				// Beta: lines 20-46 - Two noise layers
				for (int_t i = 0; i < 2; ++i)
				{
					float xo = i * 8;  // Beta: line 21
					float yo = i * 8;  // Beta: line 22
					float xd = (x - xo) / 16.0F * 2.0F;  // Beta: line 23
					float yd = (y - yo) / 16.0F * 2.0F;  // Beta: line 24
					
					// Beta: lines 25-39 - Wrap coordinates
					if (xd < -1.0F)
					{
						xd += 2.0F;  // Beta: line 26
					}

					if (xd >= 1.0F)
					{
						xd -= 2.0F;  // Beta: line 30
					}

					if (yd < -1.0F)
					{
						yd += 2.0F;  // Beta: line 34
					}

					if (yd >= 1.0F)
					{
						yd -= 2.0F;  // Beta: line 38
					}

					float dd = xd * xd + yd * yd;  // Beta: line 41
					float pp = std::atan2(yd, xd) + (time / 32.0F * Mth::PI * 2.0F - dd * 10.0F + i * 2) * (i * 2 - 1);  // Beta: line 42
					pp = (Mth::sin(pp) + 1.0F) / 2.0F;  // Beta: line 43
					pp /= dd + 1.0F;  // Beta: line 44
					pow += pp * 0.5F;  // Beta: line 45
				}

				pow += static_cast<float>(rand()) / RAND_MAX * 0.1F;  // Beta: line 48
				
				// Beta: lines 49-52 - Calculate RGBA
				int_t b = static_cast<int_t>(pow * 100.0F + 155.0F);
				int_t r = static_cast<int_t>(pow * pow * 200.0F + 55.0F);
				int_t g = static_cast<int_t>(pow * pow * pow * pow * 255.0F);
				int_t a = static_cast<int_t>(pow * 100.0F + 155.0F);
				int_t idx = y * 16 + x;  // Beta: line 53
				
				// Beta: lines 54-57 - Store in frame
				frames[time][idx * 4 + 0] = static_cast<byte_t>(r);
				frames[time][idx * 4 + 1] = static_cast<byte_t>(g);
				frames[time][idx * 4 + 2] = static_cast<byte_t>(b);
				frames[time][idx * 4 + 3] = static_cast<byte_t>(a);
			}
		}
	}
}

void TexturePortalFX::func_783_a()
{
	// Beta: PortalTexture.tick() - complete 1:1 port
	time++;  // Beta: line 65
	std::vector<byte_t> &source = frames[time & 31];  // Beta: byte[] source = this.frames[this.time & 31]

	// Beta: lines 68-86 - Copy frame to pixel buffer with anaglyph support
	for (int_t i = 0; i < 256; ++i)
	{
		int_t r = source[i * 4 + 0] & 255;  // Beta: line 69
		int_t g = source[i * 4 + 1] & 255;  // Beta: line 70
		int_t b = source[i * 4 + 2] & 255;  // Beta: line 71
		int_t a = source[i * 4 + 3] & 255;  // Beta: line 72
		
		// Beta: lines 73-80 - Anaglyph mode
		if (field_1131_c)  // Beta: this.anaglyph3d
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;  // Beta: line 74
			int_t gg = (r * 30 + g * 70) / 100;  // Beta: line 75
			int_t bb = (r * 30 + b * 70) / 100;  // Beta: line 76
			r = rr;  // Beta: line 77
			g = gg;  // Beta: line 78
			b = bb;  // Beta: line 79
		}

		// Beta: lines 82-85 - Write to pixel buffer
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);  // Beta: this.pixels[i * 4 + 0] = (byte)r
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);  // Beta: this.pixels[i * 4 + 1] = (byte)g
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);  // Beta: this.pixels[i * 4 + 2] = (byte)b
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);  // Beta: this.pixels[i * 4 + 3] = (byte)a
	}
}
