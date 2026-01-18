#include "client/renderer/TextureClockFX.h"
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

// Beta 1.2: ClockTexture.java - ported 1:1 to TextureFX (uses proper variable names)
// Beta: super(Item.clock.getIcon(null)) - pocketSundial icon index is 70 in Alpha 1.2.6 (Items.cpp:449)
// Using constant 70 since iconIndex is protected in Item class
TextureClockFX::TextureClockFX(Minecraft* mc) : TextureFX(70), mc(mc), rot(0.0), rota(0.0)
{
	// Beta: ClockTexture(Minecraft mc) calls super(Item.clock.getIcon(null))
	// Alpha 1.2.6: pocketSundial icon index is 70 (Items.cpp:449)
	// Beta: this.textureId = 1 (texture atlas: 0=terrain, 1=items)
	field_1128_f = 1;  // Beta: textureId = 1 (items.png)
	
	// Beta: lines 21-30 - Load clock icon from /gui/items.png and dial from /misc/dial.png
	raw.resize(256);  // Beta: int[] raw = new int[256]
	dialRaw.resize(256);  // Beta: int[] dialRaw = new int[256]
	try
	{
		// Beta: BufferedImage bi = ImageIO.read(Minecraft.class.getResource("/gui/items.png")) (line 22)
		std::unique_ptr<std::istream> is(Resource::getResource(u"/gui/items.png"));
		BufferedImage bi = BufferedImage::ImageIO_read(*is);
		
		// Beta: int xo = this.tex % 16 * 16; (line 23)
		// Beta: int yo = this.tex / 16 * 16; (line 24)
		int_t xo = field_1126_b % 16 * 16;  // field_1126_b is the texture index (70)
		int_t yo = field_1126_b / 16 * 16;
		
		// Beta: bi.getRGB(xo, yo, 16, 16, this.raw, 0, 16) (line 25)
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
		
		// Beta: bi = ImageIO.read(Minecraft.class.getResource("/misc/dial.png")) (line 26)
		is = std::unique_ptr<std::istream>(Resource::getResource(u"/misc/dial.png"));
		bi = BufferedImage::ImageIO_read(*is);
		
		// Beta: bi.getRGB(0, 0, 16, 16, this.dialRaw, 0, 16) (line 27)
		bi.getRGB(0, 0, 16, 16, rgbData.data());
		
		// Convert RGBA bytes to ARGB int32_t
		for (int_t i = 0; i < 256; ++i)
		{
			int_t r = rgbData[i * 4 + 0] & 0xFF;
			int_t g = rgbData[i * 4 + 1] & 0xFF;
			int_t b = rgbData[i * 4 + 2] & 0xFF;
			int_t a = rgbData[i * 4 + 3] & 0xFF;
			// Pack as ARGB: (a << 24) | (r << 16) | (g << 8) | b
			dialRaw[i] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}
	catch (const std::exception& e)
	{
		// Beta: catch (IOException var5) { var5.printStackTrace(); } (line 28-30)
		std::cerr << "Failed to load clock icon or dial: " << e.what() << std::endl;
		// Initialize raw and dialRaw with zeros on error
		for (int_t i = 0; i < 256; ++i)
		{
			raw[i] = 0;
			dialRaw[i] = 0;
		}
	}
}

void TextureClockFX::func_783_a()
{
	// Beta: ClockTexture.tick() - complete 1:1 port with proper variable names
	
	// Beta: lines 35-42 - Calculate target rotation (rott) based on time of day
	double rott = 0.0;
	if (mc->level != nullptr && mc->player != nullptr)
	{
		// Beta: float time = this.mc.level.getTimeOfDay(1.0F); (line 37)
		// Beta: rott = -time * (float) Math.PI * 2.0F; (line 38)
		float time = mc->level->getTimeOfDay(1.0F);  // Beta: level.getTimeOfDay(1.0F)
		rott = -static_cast<double>(time) * M_PI * 2.0;
		
		// Beta: if (this.mc.level.dimension.foggy) { rott = Math.random() * (float) Math.PI * 2.0; } (line 39-41)
		if (mc->level->dimension != nullptr && mc->level->dimension->foggy)  // Beta: level.dimension.foggy
		{
			rott = (static_cast<double>(rand()) / RAND_MAX) * M_PI * 2.0;
		}
	}

	// Beta: lines 44-60 - Normalize rotation difference (rotd)
	double rotd = rott - rot;

	while (rotd < -M_PI)  // Beta: line 46
	{
		rotd += M_PI * 2;  // Beta: line 47
	}

	while (rotd >= M_PI)  // Beta: line 50
	{
		rotd -= M_PI * 2;  // Beta: line 51
	}

	// Beta: lines 54-60 - Clamp rotation difference
	if (rotd < -1.0)  // Beta: line 54
	{
		rotd = -1.0;  // Beta: line 55
	}

	if (rotd > 1.0)  // Beta: line 58
	{
		rotd = 1.0;  // Beta: line 59
	}

	// Beta: lines 62-64 - Update rotation with smoothing
	this->rota += rotd * 0.1;  // Beta: line 62
	this->rota *= 0.8;  // Beta: line 63
	this->rot = this->rot + this->rota;  // Beta: line 64
	
	// Beta: lines 65-66 - Calculate sin/cos
	double sin = std::sin(this->rot);  // Beta: line 65
	double cos = std::cos(this->rot);  // Beta: line 66

	// Beta: lines 68-99 - Rotate dial texture onto red pixels in clock texture
	for (int_t i = 0; i < 256; ++i)  // Beta: line 68
	{
		// Beta: lines 69-72 - Extract ARGB from raw
		int_t a = (raw[i] >> 24) & 0xFF;  // Beta: int a = this.raw[i] >> 24 & 0xFF; (line 69)
		int_t r = (raw[i] >> 16) & 0xFF;  // Beta: int r = this.raw[i] >> 16 & 0xFF; (line 70)
		int_t g = (raw[i] >> 8) & 0xFF;  // Beta: int g = this.raw[i] >> 8 & 0xFF; (line 71)
		int_t b = (raw[i] >> 0) & 0xFF;  // Beta: int b = this.raw[i] >> 0 & 0xFF; (line 72)
		
		// Beta: lines 73-84 - If pixel is red (r == b && g == 0 && b > 0), rotate dial texture onto it
		if (r == b && g == 0 && b > 0)  // Beta: line 73
		{
			// Beta: double xo = -(i % 16 / 15.0 - 0.5); (line 74)
			// Beta: double yo = i / 16 / 15.0 - 0.5; (line 75)
			double xo = -(static_cast<double>(i % 16) / 15.0 - 0.5);
			double yo = static_cast<double>(i / 16) / 15.0 - 0.5;
			
			int_t br = r;  // Beta: int br = r; (line 76)
			
			// Beta: lines 77-78 - Rotate coordinates
			int_t x = static_cast<int_t>((xo * cos + yo * sin + 0.5) * 16.0);  // Beta: line 77
			int_t y = static_cast<int_t>((yo * cos - xo * sin + 0.5) * 16.0);  // Beta: line 78
			
			// Beta: int j = (x & 15) + (y & 15) * 16; (line 79)
			int_t j = (x & 15) + (y & 15) * 16;
			
			// Beta: lines 80-83 - Get pixel from dial texture and blend with red
			a = (dialRaw[j] >> 24) & 0xFF;  // Beta: line 80
			r = ((dialRaw[j] >> 16) & 0xFF) * r / 255;  // Beta: line 81
			g = ((dialRaw[j] >> 8) & 0xFF) * br / 255;  // Beta: line 82
			b = ((dialRaw[j] >> 0) & 0xFF) * br / 255;  // Beta: line 83
		}

		// Beta: lines 86-93 - Anaglyph mode
		if (field_1131_c)  // Beta: this.anaglyph3d
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;  // Beta: line 87
			int_t gg = (r * 30 + g * 70) / 100;  // Beta: line 88
			int_t bb = (r * 30 + b * 70) / 100;  // Beta: line 89
			r = rr;  // Beta: line 90
			g = gg;  // Beta: line 91
			b = bb;  // Beta: line 92
		}

		// Beta: lines 95-98 - Write to pixel buffer
		field_1127_a[i * 4 + 0] = static_cast<byte_t>(r);  // Beta: this.pixels[i * 4 + 0] = (byte)r
		field_1127_a[i * 4 + 1] = static_cast<byte_t>(g);  // Beta: this.pixels[i * 4 + 1] = (byte)g
		field_1127_a[i * 4 + 2] = static_cast<byte_t>(b);  // Beta: this.pixels[i * 4 + 2] = (byte)b
		field_1127_a[i * 4 + 3] = static_cast<byte_t>(a);  // Beta: this.pixels[i * 4 + 3] = (byte)a
	}
}
