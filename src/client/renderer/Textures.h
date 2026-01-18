#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>
#include <vector>

#include "java/Type.h"
#include "java/String.h"

#include "client/MemoryTracker.h"
#include "java/BufferedImage.h"

#include "util/Memory.h"

class TexturePackRepository;
class Options;
class TextureFX;
class HttpTextureProcessor;

// Include HttpTexture.h - needed for std::unique_ptr<HttpTexture> member variable
#include "client/renderer/HttpTexture.h"

class Textures
{
private:
	std::unordered_map<jstring, int_t> idMap;

public:
	static constexpr bool MIPMAP = false;

private:
	std::unordered_map<int_t, BufferedImage> loadedImages;

	std::vector<int_t> ib = MemoryTracker::createIntBuffer(1);
	std::vector<byte_t> pixels = MemoryTracker::createByteBuffer(0x100000);

	TexturePackRepository &skins;
	Options &options;

	bool clamp = false;
	bool blur = false;

	// Alpha: RenderEngine.field_1604_f - List of TextureFX instances
	std::vector<std::unique_ptr<TextureFX>> textureFXList;
	
	// newb12: Map<String, HttpTexture> httpTextures - HTTP texture cache (Textures.java:29)
	std::unordered_map<jstring, std::unique_ptr<HttpTexture>> httpTextures;

public:
	Textures(TexturePackRepository &skins, Options &options);

	int_t loadTexture(const jstring &resourceName);

private:
	BufferedImage makeStrip(BufferedImage &source);
	
public:
	int_t getTexture(BufferedImage &img);
	void loadTexture(BufferedImage &img, int_t id);
	void releaseTexture(int_t id);
	int_t loadHttpTexture(const jstring &url, const jstring *backup);
	int_t loadHttpTexture(const jstring &url);
	HttpTexture *addHttpTexture(const jstring &url, HttpTextureProcessor *processor);
	void removeHttpTexture(const jstring &url);

	void tick();

	// Alpha: RenderEngine.registerTextureFX(TextureFX var1)
	void registerTextureFX(std::unique_ptr<TextureFX> textureFX);

private:
	int_t smoothBlend(int_t c0, int_t c1);
	int_t crispBlend(int_t c0, int_t c1);

	// Alpha: RenderEngine.func_1067_a() - update and upload animated textures
	void func_1067_a();

public:
	void reloadAll();

private:
	BufferedImage readImage(std::istream &in);

public:
	void bind(int_t id);
};
