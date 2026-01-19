#include "client/renderer/Textures.h"
#include "client/renderer/TextureFX.h"
#include "client/renderer/HttpTexture.h"
#include "client/renderer/HttpTextureProcessor.h"
#include "client/renderer/MobSkinTextureProcessor.h"

#include <cassert>

#include "client/Options.h"
#include "client/skins/TexturePackRepository.h"

#include "OpenGL.h"

Textures::Textures(TexturePackRepository &skins, Options &options) : skins(skins), options(options)
{

}

int_t Textures::loadTexture(const jstring &resourceName)
{
	auto *skin = skins.selected;

	auto it = idMap.find(resourceName);
	if (it != idMap.end())
	{
		return it->second;
	}

	MemoryTracker::genTextures(ib);
	int_t i = ib[0];

	if (!resourceName.compare(0, 2, u"##"))
	{
		std::unique_ptr<std::istream> is(skin->getResource(resourceName.substr(2)));
		BufferedImage img = readImage(*is);
		img = makeStrip(img);
		loadTexture(img, i);
	}
	else if (!resourceName.compare(0, 7, u"%clamp%"))
	{
		std::unique_ptr<std::istream> is(skin->getResource(resourceName.substr(7)));
		BufferedImage img = readImage(*is);
		clamp = true;
		loadTexture(img, i);
		clamp = false;
	}
	else if (!resourceName.compare(0, 6, u"%blur%"))
	{
		std::unique_ptr<std::istream> is(skin->getResource(resourceName.substr(6)));
		BufferedImage img = readImage(*is);
		blur = true;
		loadTexture(img, i);
		blur = false;
	}
	else
	{
		std::unique_ptr<std::istream> is(skin->getResource(resourceName));
		BufferedImage img = readImage(*is);
		loadTexture(img, i);
	}

	idMap.emplace(resourceName, i);
	return i;
}

BufferedImage Textures::makeStrip(BufferedImage &source)
{
	int_t cols = source.getWidth() / 16;

	BufferedImage out(16, source.getHeight() * cols);

	for (int_t i = 0; i < cols; i++)
	{
		// g.drawImage(source, -i * 16, i * source.getHeight(), null);
		// TODO
	}

	return out;
}

int_t Textures::getTexture(BufferedImage &img)
{
	MemoryTracker::genTextures(Textures::ib);
	loadTexture(img, ib[0]);
	loadedImages.emplace(ib[0], std::move(img));
	return ib[0];
}

void Textures::loadTexture(BufferedImage &img, int_t id)
{
	glBindTexture(GL_TEXTURE_2D, id);

	if (MIPMAP)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	if (blur)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (clamp)
	{
		// Use GL_CLAMP_TO_EDGE for OpenGL 4.6 (more performant than deprecated GL_CLAMP)
		// GL_CLAMP_TO_EDGE clamps to edge texels, which is what we want for texture atlases
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	int_t w = img.getWidth();
	int_t h = img.getHeight();

	const unsigned char *rawPixels = img.getRawPixels();
	std::unique_ptr<unsigned char[]> newPixels(new unsigned char[w * h * 4]);

	const unsigned char *rawPixels_p = rawPixels;
	unsigned char *newPixels_p = newPixels.get();

	for (int_t i = 0; i < w * h; i++, rawPixels_p += 4, newPixels_p += 4)
	{
		int_t a = rawPixels_p[3];
		int_t r = rawPixels_p[0];
		int_t g = rawPixels_p[1];
		int_t b = rawPixels_p[2];

		if (options.anaglyph3d)
		{
			int_t rr = (r * 30 + g * 59 + b * 11) / 100;
			int_t gg = (r * 30 + g * 70) / 100;
			int_t bb = (r * 30 + b * 70) / 100;

			r = rr;
			g = gg;
			b = bb;
		}

		newPixels_p[0] = r;
		newPixels_p[1] = g;
		newPixels_p[2] = b;
		newPixels_p[3] = a;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, newPixels.get());

	if (MIPMAP)
	{
		std::unique_ptr<int_t[]> mipmapPixels(new int_t[w * h * 4]);
		int_t *inPixels = (int_t*)newPixels.get();

		for (int level = 1; ; level++)
		{
			if (level > 4)
				break;

			int_t ow = w >> (level - 1);

			int_t ww = w >> level;
			int_t hh = h >> level;

			for (int_t x = 0; x < ww; x++)
			{
				for (int_t y = 0; y < hh; y++)
				{
					int_t c0 = inPixels[x * 2 + 0 + (y * 2 + 0) * ow];
					int_t c1 = inPixels[x * 2 + 1 + (y * 2 + 0) * ow];
					int_t c2 = inPixels[x * 2 + 1 + (y * 2 + 1) * ow];
					int_t c3 = inPixels[x * 2 + 0 + (y * 2 + 1) * ow];
					int_t col = crispBlend(crispBlend(c0, c1), crispBlend(c3, c2));
					mipmapPixels[x + y * ww] = col;
				}
			}

			glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, ww, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmapPixels.get());

			// if (ww == 1 || hh == 1)
			// 	break;
		}
	}
}

void Textures::releaseTexture(int_t id)
{
	loadedImages.erase(id);

	ib[0] = id;
	glDeleteTextures(1, reinterpret_cast<GLuint*>(ib.data()));
}

// newb12: Textures.loadHttpTexture() - loads HTTP texture (Textures.java:177-194)
int_t Textures::loadHttpTexture(const jstring &url, const jstring *backup)
{
	auto it = httpTextures.find(url);
	if (it != httpTextures.end())
	{
		HttpTexture *texture = it->second.get();
		// Check if download completed (image has dimensions) but not yet uploaded to OpenGL
		if (texture->loadedImage.getWidth() > 0 && texture->loadedImage.getHeight() > 0 && !texture->isLoaded.load())
		{
			// Upload the processed image to OpenGL
			if (texture->id < 0)
			{
				texture->id = getTexture(texture->loadedImage);
			}
			else
			{
				loadTexture(texture->loadedImage, texture->id);
			}
			texture->isLoaded.store(true);
		}
		
		// Return texture ID if it's ready, otherwise fall back to backup
		if (texture->id >= 0)
		{
			return texture->id;
		}
		// If texture is still downloading (id < 0 and image not ready), fall back to backup
	}
	else
	{
		// Texture not in map yet - add it with MobSkinTextureProcessor for player skins
		// Check if it's a skin URL (contains "/skin/")
		if (url.find(u"/skin/") != jstring::npos)
		{
			static MobSkinTextureProcessor processor;
			addHttpTexture(url, &processor);
			// Try again after adding (but it won't be ready immediately, so will fall back to backup)
			it = httpTextures.find(url);
			if (it != httpTextures.end())
			{
				HttpTexture *texture = it->second.get();
				// Check if download completed (image has dimensions) but not yet uploaded to OpenGL
				if (texture->loadedImage.getWidth() > 0 && texture->loadedImage.getHeight() > 0 && !texture->isLoaded.load())
				{
					// Upload the processed image to OpenGL
					if (texture->id < 0)
					{
						texture->id = getTexture(texture->loadedImage);
					}
					else
					{
						loadTexture(texture->loadedImage, texture->id);
					}
					texture->isLoaded.store(true);
				}
				
				// Return texture ID if it's ready
				if (texture->id >= 0)
				{
					return texture->id;
				}
			}
		}
	}
	
	// Fall back to backup texture if URL texture isn't ready yet or failed
	if (backup != nullptr)
	{
		return loadTexture(*backup);
	}
	return -1;
}

int_t Textures::loadHttpTexture(const jstring &url)
{
	return loadHttpTexture(url, nullptr);
}

// newb12: Textures.addHttpTexture() - adds HTTP texture (Textures.java:196-205)
HttpTexture *Textures::addHttpTexture(const jstring &url, HttpTextureProcessor *processor)
{
	auto it = httpTextures.find(url);
	if (it != httpTextures.end())
	{
		it->second->count++;
		return it->second.get();
	}
	else
	{
		auto texture = Util::make_unique<HttpTexture>(url, processor);
		HttpTexture *ptr = texture.get();
		httpTextures.emplace(url, std::move(texture));
		return ptr;
	}
}

// newb12: Textures.removeHttpTexture() - removes HTTP texture (Textures.java:207-219)
void Textures::removeHttpTexture(const jstring &url)
{
	auto it = httpTextures.find(url);
	if (it != httpTextures.end())
	{
		HttpTexture *texture = it->second.get();
		texture->count--;
		if (texture->count == 0)
		{
			if (texture->id >= 0)
			{
				releaseTexture(texture->id);
			}
			httpTextures.erase(it);
		}
	}
}

void Textures::tick()
{
	// Alpha: Minecraft.run() calls renderEngine.func_1067_a() every frame
	func_1067_a();
}

void Textures::registerTextureFX(std::unique_ptr<TextureFX> textureFX)
{
	// Alpha: RenderEngine.registerTextureFX(TextureFX var1)
	// Alpha: this.field_1604_f.add(var1); var1.func_783_a();
	textureFX->func_783_a();  // Initialize animation
	textureFXList.push_back(std::move(textureFX));
}

void Textures::func_1067_a()
{
	// Alpha: RenderEngine.func_1067_a() - complete 1:1 port
	int_t var1, var2;
	TextureFX *var2_ptr;
	int_t var3, var4, var5, var6, var7, var8, var9, var10, var11, var12;

	for (var1 = 0; var1 < static_cast<int_t>(textureFXList.size()); ++var1)
	{
		var2_ptr = textureFXList[var1].get();
		var2_ptr->field_1131_c = options.anaglyph3d;  // Alpha: var2.field_1131_c = this.options.anaglyph
		var2_ptr->func_783_a();  // Alpha: var2.func_783_a() - update animation
		
		// Alpha: this.imageData.clear(); this.imageData.put(var2.field_1127_a);
		pixels.clear();
		pixels.insert(pixels.end(), var2_ptr->field_1127_a.begin(), var2_ptr->field_1127_a.end());
		
		var2_ptr->func_782_a(*this);  // Alpha: var2.func_782_a(this) - bind texture

		// Alpha: Upload texture sub-image for each tile (field_1129_e x field_1129_e tiles)
		// Alpha: For water, field_1129_e = 1, so this loops once
		// Alpha: field_1127_a contains 16x16 RGBA = 1024 bytes for a single tile
		for (var3 = 0; var3 < var2_ptr->field_1129_e; ++var3)
		{
			for (var4 = 0; var4 < var2_ptr->field_1129_e; ++var4)
			{
				// Alpha: GL11.glTexSubImage2D(GL11.GL_TEXTURE_2D, 0, var2.field_1126_b % 16 * 16 + var3 * 16, var2.field_1126_b / 16 * 16 + var4 * 16, 16, 16, GL11.GL_RGBA, GL11.GL_UNSIGNED_BYTE, (ByteBuffer)this.imageData);
				int_t texX = (var2_ptr->field_1126_b % 16) * 16 + var3 * 16;
				int_t texY = (var2_ptr->field_1126_b / 16) * 16 + var4 * 16;
				
				// Alpha: imageData contains var2.field_1127_a (1024 bytes for 16x16 RGBA)
				// For field_1129_e = 1, we upload the entire buffer as one tile
				// For field_1129_e > 1, we'd need to calculate offsets, but water uses field_1129_e = 1
				glTexSubImage2D(GL_TEXTURE_2D, 0, texX, texY, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
				
				// Alpha: Mipmap generation (if useMipmaps is true, but we have MIPMAP = false)
				// Skipping mipmap code since MIPMAP = false
			}
		}
	}

	// Alpha: Second loop for alternate textures (field_1130_d > 0)
	for (var1 = 0; var1 < static_cast<int_t>(textureFXList.size()); ++var1)
	{
		var2_ptr = textureFXList[var1].get();
		if (var2_ptr->field_1130_d > 0)
		{
			pixels.clear();
			pixels.insert(pixels.end(), var2_ptr->field_1127_a.begin(), var2_ptr->field_1127_a.end());
			glBindTexture(GL_TEXTURE_2D, var2_ptr->field_1130_d);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
			// Skipping mipmap code since MIPMAP = false
		}
	}
}

int_t Textures::smoothBlend(int_t c0, int_t c1)
{
	int_t a0 = (c0 & 0xFF000000) >> 24 & 0xFF;
	int_t a1 = (c1 & 0xFF000000) >> 24 & 0xFF;
	return (((a0 + a1) >> 1) << 24) + (((c0 & 0xFEFEFE) + (c1 & 0xFEFEFE)) >> 1);
}

int_t Textures::crispBlend(int_t c0, int_t c1)
{
	int_t a0 = ((c0 & 0xFF000000) >> 24) & 0xFF;
	int_t a1 = ((c1 & 0xFF000000) >> 24) & 0xFF;

	int_t a = 255;
	if (a0 + a1 == 0)
	{
		a0 = 1;
		a1 = 1;
		a = 0;
	}

	int_t r0 = ((c0 >> 16) & 0xFF) * a0;
	int_t g0 = ((c0 >> 8) & 0xFF) * a0;
	int_t b0 = (c0 & 0xFF) * a0;

	int_t r1 = ((c1 >> 16) & 0xFF) * a1;
	int_t g1 = ((c1 >> 8) & 0xFF) * a1;
	int_t b1 = (c1 & 0xFF) * a1;

	int_t r = (r0 + r1) / (a0 + a1);
	int_t g = (g0 + g1) / (a0 + a1);
	int_t b = (b0 + b1) / (a0 + a1);

	return (a << 24) | (r << 16) | (g << 8) | b;
}

void Textures::reloadAll()
{
	TexturePack *skin = skins.selected;

	// Reload buffered textures
	for (auto &entry : loadedImages)
	{
		loadTexture(entry.second, entry.first);
	}

	// Reload resource textures
	for (auto &entry : idMap)
	{
		const jstring &name = entry.first;

		BufferedImage image;

		if (!name.compare(0, 2, u"##"))
		{
			std::unique_ptr<std::istream> is(skin->getResource(name.substr(2)));
			image = readImage(*is);
			image = makeStrip(image);
		}
		else if (!name.compare(0, 7, u"%clamp%"))
		{
			std::unique_ptr<std::istream> is(skin->getResource(name.substr(7)));
			clamp = true;
			image = readImage(*is);
		}
		else if (!name.compare(0, 6, u"%blur%"))
		{
			std::unique_ptr<std::istream> is(skin->getResource(name.substr(6)));
			blur = true;
			image = readImage(*is);
		}
		else
		{
			std::unique_ptr<std::istream> is(skin->getResource(name));
			image = readImage(*is);
		}

		int_t id = entry.second;
		loadTexture(image, id);
		blur = false;
		clamp = false;
	}
}

BufferedImage Textures::readImage(std::istream &in)
{
	return BufferedImage::ImageIO_read(in);
}

void Textures::bind(int_t id)
{
	if (id < 0) return;
	glBindTexture(GL_TEXTURE_2D, id);
}