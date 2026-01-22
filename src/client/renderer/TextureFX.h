#pragma once

#include "java/Type.h"
#include <vector>

class Textures;

// Alpha: TextureFX.java
class TextureFX
{
public:
	std::vector<byte_t> field_1127_a;  // Alpha: byte[] field_1127_a = new byte[1024]
	int_t field_1126_b;                // Alpha: int field_1126_b (texture index)
	bool field_1131_c = false;          // Alpha: boolean field_1131_c (anaglyph flag)
	int_t field_1130_d = 0;             // Alpha: int field_1130_d (alternate texture ID)
	int_t field_1129_e = 1;             // Alpha: int field_1129_e (tile size multiplier)
	int_t field_1128_f = 0;             // Alpha: int field_1128_f (0=terrain, 1=items)

public:
	TextureFX(int_t textureIndex);
	virtual ~TextureFX() = default;

	// Alpha: void func_783_a() - update animation
	virtual void func_783_a();

	// Alpha: void func_782_a(RenderEngine var1) - bind texture
	virtual void func_782_a(Textures &textures);
};
