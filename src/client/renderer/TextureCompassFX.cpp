#include "client/renderer/TextureCompassFX.h"
#include "client/Minecraft.h"
#include "world/item/Items.h"
#include "java/BufferedImage.h"
#include "java/Resource.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

// Beta 1.2: CompassTexture.java - ported 1:1 to TextureFX (uses proper variable names)
// Beta: super(Item.compass.getIcon(null)) - compass icon index is 54 in Alpha 1.2.6 (Items.cpp:444)
// Using constant 54 since iconIndex is protected in Item class
TextureCompassFX::TextureCompassFX(Minecraft* mc) : TextureFX(54), mc(mc), rot(0.0), rota(0.0)
{
	// Beta: CompassTexture(Minecraft mc) calls super(Item.compass.getIcon(null))
	// Alpha 1.2.6: compass icon index is 54 (Items.cpp:444)
	// Beta: this.textureId = 1 (texture atlas: 0=terrain, 1=items)
	field_1128_f = 1;  // Beta: textureId = 1 (items.png)
	
	// Beta: lines 20-27 - Load compass icon from /gui/items.png
	raw.resize(256);  // Beta: int[] raw = new int[256]
	try
	{
		// Beta: BufferedImage bi = ImageIO.read(Minecraft.class.getResource("/gui/items.png"))
		std::unique_ptr<std::istream> is(Resource::getResource(u"/gui/items.png"));
		BufferedImage bi = BufferedImage::ImageIO_read(*is);
		
		// Beta: int xo = this.tex % 16 * 16; (line 22)
		// Beta: int yo = this.tex / 16 * 16; (line 23)
		int_t xo = field_1126_b % 16 * 16;  // field_1126_b is the texture index (54)
		int_t yo = field_1126_b / 16 * 16;
		
		// Beta: bi.getRGB(xo, yo, 16, 16, this.raw, 0, 16) (line 24)
		// Convert RGBA bytes to packed ARGB ints
		std::vector<unsigned char> rgbData(16 * 16 * 4);
		bi.getRGB(xo, yo, 16, 16, rgbData.data());
		
		// Convert RGBA bytes to ARGB int32_t
		for (int_t i = 0; i < 256; ++i)
		{
			int_t r = rgbData[i * 4 + 0] & 0xFF;
			int_t g = rgbData[i * 4 + 1] & 0xFF;
			int_t b = rgbData[i * 4 + 2] & 0xFF;
			int_t a = rgbData[i * 4 + 3] & 0xFF;
			// Pack as ARGB: (a << 24) | (r << 16) | (g << 8) | b
			raw[i] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}
	catch (const std::exception& e)
	{
		// Beta: catch (IOException var5) { var5.printStackTrace(); } (line 25-27)
		std::cerr << "Failed to load compass icon: " << e.what() << std::endl;
		// Initialize raw with zeros on error
		for (int_t i = 0; i < 256; ++i)
		{
			raw[i] = 0;
		}
	}
}

void TextureCompassFX::func_783_a()
{
	// Beta: CompassTexture.tick() - complete 1:1 port with proper variable names
	
	// Beta: lines 32-50 - Copy raw icon to pixels with anaglyph support
	for (int_t i = 0; i < 256; ++i)
	{
		// Beta: int a = this.raw[i] >> 24 & 0xFF; (line 33)
		// Beta: int r = this.raw[i] >> 16 & 0xFF; (line 34)
		// Beta: int g = this.raw[i] >> 8 & 0xFF; (line 35)
		// Beta: int b = this.raw[i] >> 0 & 0xFF; (line 36)
		int_t a = (raw[i] >> 24) & 0xFF;
		int_t r = (raw[i] >> 16) & 0xFF;
		int_t g = (raw[i] >> 8) & 0xFF;
		int_t b = (raw[i] >> 0) & 0xFF;
		
		// Beta: lines 37-44 - Anaglyph mode
		if (field_1131_c)  // Beta: this.anaglyph3d
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;  // Beta: line 38
			int_t gg = (r * 30 + g * 70) / 100;  // Beta: line 39
			int_t bb = (r * 30 + b * 70) / 100;  // Beta: line 40
			r = rr;  // Beta: line 41
			g = gg;  // Beta: line 42
			b = bb;  // Beta: line 43
		}

		// Beta: lines 46-49 - Write to pixel buffer
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);  // Beta: this.pixels[i * 4 + 0] = (byte)r
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);  // Beta: this.pixels[i * 4 + 1] = (byte)g
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);  // Beta: this.pixels[i * 4 + 2] = (byte)b
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);  // Beta: this.pixels[i * 4 + 3] = (byte)a
	}

	// Beta: lines 52-60 - Calculate target rotation (rott)
	double rott = 0.0;
	if (mc->level != nullptr && mc->player != nullptr)
	{
		// Beta: double xa = this.mc.level.xSpawn - this.mc.player.x; (line 54)
		// Beta: double za = this.mc.level.zSpawn - this.mc.player.z; (line 55)
		double xa = static_cast<double>(mc->level->xSpawn) - mc->player->x;  // Beta: level.xSpawn
		double za = static_cast<double>(mc->level->zSpawn) - mc->player->z;  // Beta: level.zSpawn
		
		// Beta: rott = (this.mc.player.yRot - 90.0F) * Math.PI / 180.0 - Math.atan2(za, xa); (line 56)
		rott = (static_cast<double>(mc->player->yRot) - 90.0) * M_PI / 180.0 - std::atan2(za, xa);  // Beta: player.yRot
		
		// Beta: if (this.mc.level.dimension.foggy) { rott = Math.random() * (float) Math.PI * 2.0; } (line 57-59)
		if (mc->level->dimension != nullptr && mc->level->dimension->foggy)  // Beta: level.dimension.foggy
		{
			rott = (static_cast<double>(rand()) / RAND_MAX) * M_PI * 2.0;
		}
	}

	// Beta: lines 62-70 - Normalize rotation difference (rotd)
	double rotd = rott - rot;

	while (rotd < -M_PI)  // Beta: line 64
	{
		rotd += M_PI * 2;  // Beta: line 65
	}

	while (rotd >= M_PI)  // Beta: line 68
	{
		rotd -= M_PI * 2;  // Beta: line 69
	}

	// Beta: lines 72-78 - Clamp rotation difference
	if (rotd < -1.0)  // Beta: line 72
	{
		rotd = -1.0;  // Beta: line 73
	}

	if (rotd > 1.0)  // Beta: line 76
	{
		rotd = 1.0;  // Beta: line 77
	}

	// Beta: lines 80-82 - Update rotation with smoothing
	this->rota += rotd * 0.1;  // Beta: line 80
	this->rota *= 0.8;  // Beta: line 81
	this->rot = this->rot + this->rota;  // Beta: line 82
	
	// Beta: lines 83-84 - Calculate sin/cos
	double sin = std::sin(this->rot);  // Beta: line 83
	double cos = std::cos(this->rot);  // Beta: line 84

	// Beta: lines 86-107 - Draw compass needle (horizontal line)
	for (int_t d = -4; d <= 4; ++d)  // Beta: line 86
	{
		int_t x = static_cast<int_t>(8.5 + cos * d * 0.3);  // Beta: line 87
		int_t y = static_cast<int_t>(7.5 - sin * d * 0.3 * 0.5);  // Beta: line 88
		int_t i = y * 16 + x;  // Beta: line 89
		int_t r = 100;  // Beta: line 90
		int_t g = 100;  // Beta: line 91
		int_t b = 100;  // Beta: line 92
		int_t a = 255;  // Beta: line 93
		
		// Beta: lines 94-101 - Anaglyph mode
		if (field_1131_c)  // Beta: this.anaglyph3d
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;  // Beta: line 95
			int_t gg = (r * 30 + g * 70) / 100;  // Beta: line 96
			int_t bb = (r * 30 + b * 70) / 100;  // Beta: line 97
			r = rr;  // Beta: line 98
			g = gg;  // Beta: line 99
			b = bb;  // Beta: line 100
		}

		// Beta: lines 103-106 - Write needle pixel
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);  // Beta: this.pixels[i * 4 + 0] = (byte)r
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);  // Beta: this.pixels[i * 4 + 1] = (byte)g
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);  // Beta: this.pixels[i * 4 + 2] = (byte)b
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);  // Beta: this.pixels[i * 4 + 3] = (byte)a
	}

	// Beta: lines 109-130 - Draw compass pointer (vertical line, red at top)
	for (int_t d = -8; d <= 16; ++d)  // Beta: line 109
	{
		int_t x = static_cast<int_t>(8.5 + sin * d * 0.3);  // Beta: line 110
		int_t y = static_cast<int_t>(7.5 + cos * d * 0.3 * 0.5);  // Beta: line 111
		int_t i = y * 16 + x;  // Beta: line 112
		int_t r = d >= 0 ? 255 : 100;  // Beta: line 113
		int_t g = d >= 0 ? 20 : 100;  // Beta: line 114
		int_t b = d >= 0 ? 20 : 100;  // Beta: line 115
		int_t a = 255;  // Beta: line 116
		
		// Beta: lines 117-124 - Anaglyph mode
		if (field_1131_c)  // Beta: this.anaglyph3d
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;  // Beta: line 118
			int_t gg = (r * 30 + g * 70) / 100;  // Beta: line 119
			int_t bb = (r * 30 + b * 70) / 100;  // Beta: line 120
			r = rr;  // Beta: line 121
			g = gg;  // Beta: line 122
			b = bb;  // Beta: line 123
		}

		// Beta: lines 126-129 - Write pointer pixel
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);  // Beta: this.pixels[i * 4 + 0] = (byte)r
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);  // Beta: this.pixels[i * 4 + 1] = (byte)g
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);  // Beta: this.pixels[i * 4 + 2] = (byte)b
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);  // Beta: this.pixels[i * 4 + 3] = (byte)a
	}
}
