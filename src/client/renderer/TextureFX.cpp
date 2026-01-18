#include "client/renderer/TextureFX.h"
#include "client/renderer/Textures.h"
#include "OpenGL.h"

// Alpha: TextureFX.java
TextureFX::TextureFX(int_t textureIndex) : field_1126_b(textureIndex)
{
	// Alpha: field_1127_a = new byte[1024] (16x16 RGBA = 1024 bytes)
	field_1127_a.resize(1024);
}

void TextureFX::func_783_a()
{
	// Alpha: TextureFX.func_783_a() is empty base implementation
	// Subclasses override this to update animation
}

void TextureFX::func_782_a(Textures &textures)
{
	// Alpha: TextureFX.func_782_a() binds the appropriate texture
	// field_1128_f: 0 = terrain.png, 1 = items.png
	if (field_1128_f == 0)
	{
		glBindTexture(GL_TEXTURE_2D, textures.loadTexture(u"/terrain.png"));
	}
	else if (field_1128_f == 1)
	{
		glBindTexture(GL_TEXTURE_2D, textures.loadTexture(u"/gui/items.png"));
	}
}
