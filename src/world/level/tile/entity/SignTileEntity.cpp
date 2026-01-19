#include "world/level/tile/entity/SignTileEntity.h"

#include "nbt/CompoundTag.h"
#include "pc/OpenGL.h"
#include "client/renderer/tileentity/SignRenderer.h"

// Beta: SignTileEntity() - constructor (SignTileEntity.java:7-9)
SignTileEntity::SignTileEntity()
{
	// Beta: messages = new String[]{"", "", "", ""}, selectedLine = -1
	messages[0] = u"";
	messages[1] = u"";
	messages[2] = u"";
	messages[3] = u"";
	selectedLine = -1;
}

// Destructor: Delete texture immediately (we're on render thread during entity destruction)
SignTileEntity::~SignTileEntity()
{
	// CRITICAL: Delete texture immediately when sign is destroyed
	// When a sign is destroyed and immediately replaced, we need to clear the old texture
	// to prevent stale textures from appearing on the new sign
	if (textTexId != 0)
	{
		GLuint oldTexId = textTexId;
		textTexId = 0;  // Clear immediately to prevent reuse
		
		// CRITICAL: Unbind texture before deletion to prevent stale bindings
		// Check if this texture is currently bound (only if OpenGL context is valid)
		// glGetIntegerv can fail with GL_INVALID_OPERATION if called before context is ready
		GLenum glError = glGetError();  // Clear any previous errors
		GLint currentTexBinding = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexBinding);
		GLenum checkError = glGetError();
		if (checkError == GL_NO_ERROR && static_cast<GLuint>(currentTexBinding) == oldTexId)
		{
			glBindTexture(GL_TEXTURE_2D, 0);  // Unbind before deletion
		}
		else if (checkError != GL_NO_ERROR)
		{
			// OpenGL context not ready or invalid - just unbind directly without checking
			// This is safe even if the texture isn't bound
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		// Delete texture immediately (we're on render thread during entity destruction)
		glDeleteTextures(1, &oldTexId);
		
		// Also enqueue for deletion as backup (in case it wasn't fully deleted)
		SignRenderer::enqueueTextureDeletion(oldTexId);
	}
}

// Beta: SignTileEntity.save() - saves Text1-4 to NBT (SignTileEntity.java:12-18)
void SignTileEntity::save(CompoundTag &tag)
{
	TileEntity::save(tag);
	tag.putString(u"Text1", messages[0]);
	tag.putString(u"Text2", messages[1]);
	tag.putString(u"Text3", messages[2]);
	tag.putString(u"Text4", messages[3]);
}

// Beta: SignTileEntity.load() - loads Text1-4 from NBT, truncates to 15 chars (SignTileEntity.java:21-30)
void SignTileEntity::load(CompoundTag &tag)
{
	TileEntity::load(tag);
	
	// Beta: for (int var2 = 0; var2 < 4; var2++) (SignTileEntity.java:24)
	for (int_t i = 0; i < 4; i++)
	{
		// Beta: this.messages[var2] = var1.getString("Text" + (var2 + 1)) (SignTileEntity.java:25)
		jstring textKey = u"Text" + String::toString(i + 1, 10);
		messages[i] = tag.getString(textKey);
		
		// Beta: if (this.messages[var2].length() > 15) this.messages[var2] = this.messages[var2].substring(0, 15) (SignTileEntity.java:26-28)
		if (messages[i].length() > 15)
		{
			messages[i] = messages[i].substr(0, 15);
		}
	}
	
	// Performance: Invalidate caches after loading
	invalidateWidthCache();
	invalidateTextDisplayList();  // This now invalidates baked texture
}
