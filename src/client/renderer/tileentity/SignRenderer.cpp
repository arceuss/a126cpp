#include "client/renderer/tileentity/SignRenderer.h"

#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "client/gui/Font.h"
#include "OpenGL.h"

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
SignRenderer::SignRenderer()
{
	// newb12: SignModel is initialized in member initializer
}

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
void SignRenderer::render(SignTileEntity &sign, double x, double y, double z, float a)
{
	// Safety check: Skip rendering if tile entity has no valid level
	// This can happen during world unloading or when chunk is unloaded
	if (sign.level == nullptr)
	{
		return;  // Don't render if level is null
	}
	
	// Safety check: Ensure chunk is loaded before trying to get tile
	// When chunks unload, tile entities may still be in render list but chunk data is unavailable
	if (!sign.level->hasChunkAt(sign.x, sign.y, sign.z))
	{
		return;  // Don't render if chunk is not loaded
	}
	
	// Safety: Get tile ID and validate tile pointer before calling getTile()
	// This prevents dereferencing null pointers
	int_t tileId = sign.level->getTile(sign.x, sign.y, sign.z);
	
	// Validate tile ID is in valid range
	if (tileId < 0 || tileId >= 256)
	{
		// Invalid tile ID - use air tile (ID 0) as fallback
		tileId = 0;
	}
	
	// Check if tile pointer is null - if so, use air tile
	Tile *tilePtr = Tile::tiles[tileId];
	if (tilePtr == nullptr)
	{
		// Tile is null - use air tile as fallback
		tilePtr = Tile::tiles[0];
		if (tilePtr == nullptr)
		{
			return;  // Even air tile is null - skip rendering (critical error)
		}
	}
	
	Tile &tile = *tilePtr;  // Now safe to use
	glPushMatrix();  // newb12: GL11.glPushMatrix() (SignRenderer.java:14)
	float size = 0.6666667f;  // newb12: float size = 0.6666667F (SignRenderer.java:15)
	
	if (tile.id == Tile::sign.id)  // newb12: if (tile == Tile.sign) (SignRenderer.java:16)
	{
		// newb12: Sign post rendering (SignRenderer.java:17-20)
		glTranslatef((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);  // newb12: GL11.glTranslatef((float)x + 0.5F, (float)y + 0.75F * size, (float)z + 0.5F) (SignRenderer.java:17)
		float rot = sign.getData() * 360 / 16.0f;  // newb12: float rot = sign.getData() * 360 / 16.0F (SignRenderer.java:18)
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);  // newb12: GL11.glRotatef(-rot, 0.0F, 1.0F, 0.0F) (SignRenderer.java:19)
		signModel.cube2.visible = true;  // newb12: this.signModel.cube2.visible = true (SignRenderer.java:20)
	}
	else
	{
		// newb12: Wall sign rendering (SignRenderer.java:22-39)
		int_t face = sign.getData();  // newb12: int face = sign.getData() (SignRenderer.java:22)
		float rot = 0.0f;  // newb12: float rot = 0.0F (SignRenderer.java:23)
		
		if (face == 2)  // newb12: if (face == 2) (SignRenderer.java:24)
		{
			rot = 180.0f;  // newb12: rot = 180.0F (SignRenderer.java:25)
		}
		
		if (face == 4)  // newb12: if (face == 4) (SignRenderer.java:28)
		{
			rot = 90.0f;  // newb12: rot = 90.0F (SignRenderer.java:29)
		}
		
		if (face == 5)  // newb12: if (face == 5) (SignRenderer.java:32)
		{
			rot = -90.0f;  // newb12: rot = -90.0F (SignRenderer.java:33)
		}
		
		glTranslatef((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);  // newb12: GL11.glTranslatef((float)x + 0.5F, (float)y + 0.75F * size, (float)z + 0.5F) (SignRenderer.java:36)
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);  // newb12: GL11.glRotatef(-rot, 0.0F, 1.0F, 0.0F) (SignRenderer.java:37)
		glTranslatef(0.0f, -0.3125f, -0.4375f);  // newb12: GL11.glTranslatef(0.0F, -0.3125F, -0.4375F) (SignRenderer.java:38)
		signModel.cube2.visible = false;  // newb12: this.signModel.cube2.visible = false (SignRenderer.java:39)
	}
	
	bindTexture(u"/item/sign.png");  // newb12: this.bindTexture("/item/sign.png") (SignRenderer.java:42)
	glPushMatrix();  // newb12: GL11.glPushMatrix() (SignRenderer.java:43)
	glScalef(size, -size, -size);  // newb12: GL11.glScalef(size, -size, -size) (SignRenderer.java:44)
	signModel.render();  // newb12: this.signModel.render() (SignRenderer.java:45)
	glPopMatrix();  // newb12: GL11.glPopMatrix() (SignRenderer.java:46)
	
	Font *font = getFont();  // newb12: Font font = this.getFont() (SignRenderer.java:47)
	float s = 0.016666668f * size;  // newb12: float s = 0.016666668F * size (SignRenderer.java:48)
	glTranslatef(0.0f, 0.5f * size, 0.07f * size);  // newb12: GL11.glTranslatef(0.0F, 0.5F * size, 0.07F * size) (SignRenderer.java:49)
	glScalef(s, -s, s);  // newb12: GL11.glScalef(s, -s, s) (SignRenderer.java:50)
	glNormal3f(0.0f, 0.0f, -1.0f * s);  // newb12: GL11.glNormal3f(0.0F, 0.0F, -1.0F * s) (SignRenderer.java:51)
	glDepthMask(false);  // newb12: GL11.glDepthMask(false) (SignRenderer.java:52)
	int_t col = 0;  // newb12: int col = 0 (SignRenderer.java:53)
	
	// Optimized batch rendering: Prepare all lines and render in one call
	jstring processedLines[4];
	int_t xOffsets[4];
	int_t yOffsets[4];
	
	for (int_t i = 0; i < 4; i++)  // newb12: for (int i = 0; i < sign.messages.length; i++) (SignRenderer.java:55)
	{
		jstring msg = sign.messages[i];  // newb12: String msg = sign.messages[i] (SignRenderer.java:56)
		
		if (i == sign.selectedLine)  // newb12: if (i == sign.selectedLine) (SignRenderer.java:57)
		{
			msg = u"> " + msg + u" <";  // newb12: msg = "> " + msg + " <" (SignRenderer.java:58)
		}
		
		processedLines[i] = msg;
		
		// Calculate x offset (centered)
		int_t msgWidth = font->width(msg);
		xOffsets[i] = -msgWidth / 2;
		
		// Calculate y offset (line spacing)
		yOffsets[i] = i * 10 - 4 * 5;
	}
	
	// Render all lines in a single batched draw call
	font->drawSignTextBatched(processedLines, xOffsets, yOffsets, col);
	
	glDepthMask(true);  // newb12: GL11.glDepthMask(true) (SignRenderer.java:65)
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // newb12: GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F) (SignRenderer.java:66)
	glPopMatrix();  // newb12: GL11.glPopMatrix() (SignRenderer.java:67)
}

void SignRenderer::renderEntity(TileEntity *entity, double x, double y, double z, float a)
{
	SignTileEntity *sign = dynamic_cast<SignTileEntity *>(entity);
	if (sign != nullptr)
	{
		render(*sign, x, y, z, a);
	}
}
