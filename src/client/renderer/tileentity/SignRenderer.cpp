#include "client/renderer/tileentity/SignRenderer.h"

#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "client/gui/Font.h"
#include "java/Resource.h"
#include "java/BufferedImage.h"
#include "SharedConstants.h"
#include "OpenGL.h"
#include <memory>
#include <cstring>

// Static members for rebuild budget tracking
int_t SignRenderer::rebuildsThisFrame = 0;
int_t SignRenderer::lastFrameNumber = -1;

// Static members for quad VBO (OpenGL 4.6 optimization)
GLuint SignRenderer::quadVBO = 0;
bool SignRenderer::quadVBOInitialized = false;

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
SignRenderer::SignRenderer()
{
	// newb12: SignModel is initialized in member initializer
}

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
void SignRenderer::render(SignTileEntity &sign, double x, double y, double z, float a)
{
	// Performance optimization: Early exit for signs with no visible text
	bool hasVisibleText = false;
	for (int_t i = 0; i < 4; i++)
	{
		if (!sign.messages[i].empty())
		{
			hasVisibleText = true;
			break;
		}
	}
	// If no text and not being edited (selectedLine < 0), skip rendering text part
	// But we still need to render the sign model
	
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
	
	// Performance optimization: Pre-calculate constants
	static constexpr float SIZE = 0.6666667f;  // newb12: float size = 0.6666667F (SignRenderer.java:15)
	static constexpr float TEXT_SCALE_MULT = 0.016666668f;  // Pre-calculated constant
	const float size = SIZE;
	const float textScale = TEXT_SCALE_MULT * size;  // Pre-calculate once
	const float signYOffset = 0.75f * size;  // Pre-calculate once
	const float textYOffset = 0.5f * size;  // Pre-calculate once
	const float textZOffset = 0.07f * size;  // Pre-calculate once
	
	// Performance optimization: Pre-calculate position components
	const float xf = static_cast<float>(x) + 0.5f;
	const float yf = static_cast<float>(y) + signYOffset;
	const float zf = static_cast<float>(z) + 0.5f;
	
	int_t signData = sign.getData();  // Cache getData() call
	float rot = 0.0f;
	bool isWallSign = (tile.id != Tile::sign.id);
	
	if (!isWallSign)  // Sign post
	{
		// newb12: Sign post rendering (SignRenderer.java:17-20)
		glTranslatef(xf, yf, zf);  // Optimized: use pre-calculated values
		rot = signData * 22.5f;  // Performance: Pre-calculate 360/16 = 22.5
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);  // newb12: GL11.glRotatef(-rot, 0.0F, 1.0F, 0.0F) (SignRenderer.java:19)
		signModel.cube2.visible = true;  // newb12: this.signModel.cube2.visible = true (SignRenderer.java:20)
	}
	else
	{
		// newb12: Wall sign rendering (SignRenderer.java:22-39)
		// Performance optimization: Use switch or lookup table instead of multiple ifs
		// Pre-calculated rotation values for each face
		switch (signData)
		{
			case 2: rot = 180.0f; break;
			case 4: rot = 90.0f; break;
			case 5: rot = -90.0f; break;
			default: rot = 0.0f; break;
		}
		
		glTranslatef(xf, yf, zf);  // Optimized: use pre-calculated values
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);  // newb12: GL11.glRotatef(-rot, 0.0F, 1.0F, 0.0F) (SignRenderer.java:37)
		glTranslatef(0.0f, -0.3125f, -0.4375f);  // newb12: GL11.glTranslatef(0.0F, -0.3125F, -0.4375F) (SignRenderer.java:38)
		signModel.cube2.visible = false;  // newb12: this.signModel.cube2.visible = false (SignRenderer.java:39)
	}
	
	// Render sign model
	bindTexture(u"/item/sign.png");  // newb12: this.bindTexture("/item/sign.png") (SignRenderer.java:42)
	glPushMatrix();  // newb12: GL11.glPushMatrix() (SignRenderer.java:43)
	glScalef(size, -size, -size);  // newb12: GL11.glScalef(size, -size, -size) (SignRenderer.java:44)
	signModel.render();  // newb12: this.signModel.render() (SignRenderer.java:45)
	glPopMatrix();  // newb12: GL11.glPopMatrix() (SignRenderer.java:46)
	
	// newb12: Render sign text (SignRenderer.java:47-64)
	// Performance optimization: Distance-based LOD - only render text on nearby signs
	// Calculate distance to player for text LOD
	// Use sign's actual world position (sign.x, sign.y, sign.z) directly for reliable distance calculation
	auto &dispatcher = TileEntityRenderDispatcher::instance;
	
	double dx = sign.x - dispatcher.xPlayer;
	double dy = sign.y - dispatcher.yPlayer;
	double dz = sign.z - dispatcher.zPlayer;
	double distSq = dx * dx + dy * dy + dz * dz;
	
	// Render text only if within 32 blocks (1024.0) for better performance and readability
	// Sign model is always rendered, but text is LOD-based
	// 32 blocks is a good balance: readable text while still providing LOD performance benefits
	constexpr double TEXT_RENDER_DISTANCE_SQ = 1024.0;  // 32 blocks squared
	bool renderText = (hasVisibleText || sign.selectedLine >= 0) && distSq < TEXT_RENDER_DISTANCE_SQ;
	
		if (renderText)
	{
		Font *font = getFont();  // newb12: Font font = this.getFont() (SignRenderer.java:47)
		
		// Performance optimization: Update cached widths if needed
		if (sign.widthsDirty)
		{
			for (int_t i = 0; i < 4; i++)
			{
				if (sign.messages[i].empty())
					sign.cachedWidths[i] = 0;
				else
					sign.cachedWidths[i] = font->width(sign.messages[i]);
			}
			sign.widthsDirty = false;
		}
		
		// Performance optimization: Check if baked texture needs rebuilding
		// Cache is invalidated when messages change (via Packet130) or when loading from NBT
		// selectedLine changes only during editing (rare), so we check it efficiently here
		bool needsRebuild = sign.textDirty;
		
		// Only check selectedLine if texture exists and we're not already rebuilding
		// selectedLine only changes during editing (TextEditScreen), so this is rare
		if (!needsRebuild && sign.textTexId != 0 && sign.cachedSelectedLine != sign.selectedLine)
		{
			needsRebuild = true;  // selectedLine changed, need to rebuild
		}
		
		// Bake texture if needed (with rebuild budget throttling)
		if (needsRebuild)
		{
			bakeSignTexture(sign, font);
		}
		
		// Render baked texture as a single quad
		if (sign.textTexId != 0)
		{
			// Use pre-calculated text scale
			glTranslatef(0.0f, textYOffset, textZOffset);  // Optimized: use pre-calculated values
			glScalef(textScale, -textScale, textScale);  // Optimized: use pre-calculated value
			glNormal3f(0.0f, 0.0f, -textScale);  // Optimized: use pre-calculated value
			glDepthMask(false);  // newb12: GL11.glDepthMask(false) (SignRenderer.java:52)
			
			// Bind baked texture
			glBindTexture(GL_TEXTURE_2D, sign.textTexId);
			
			// Enable blending (matching font rendering)
			// Note: Blending may already be enabled, but we ensure it's set correctly
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			// Set color to white (text color is baked into texture)
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			
			// Initialize quad VBO if needed (one-time setup)
			if (!quadVBOInitialized)
			{
				initQuadVBO();
			}
			
			// Render quad using VBO (faster than immediate mode)
			// Quad vertices are in font coordinate space (before textScale is applied)
			if (quadVBO != 0)
			{
				// Bind quad VBO
				glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
				
				// Setup vertex attributes
				glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(0));
				glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(3 * sizeof(float)));
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				
				// Draw quad (4 vertices)
				glDrawArrays(GL_QUADS, 0, 4);
				
				// Clean up
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				// Fallback to immediate mode if VBO initialization failed
				constexpr float TEXTURE_COORD_WIDTH = 128.0f;
				constexpr float TEXTURE_COORD_HEIGHT = 64.0f;
				float halfW = TEXTURE_COORD_WIDTH / 2.0f;
				float halfH = TEXTURE_COORD_HEIGHT / 2.0f;
				
				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(-halfW, halfH, 0.0f);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(halfW, halfH, 0.0f);
				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(halfW, -halfH, 0.0f);
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(-halfW, -halfH, 0.0f);
				glEnd();
			}
			
			glDepthMask(true);  // newb12: GL11.glDepthMask(true) (SignRenderer.java:65)
		}
		else
		{
			// Fallback to immediate rendering if texture baking failed
			// This should rarely happen, but provides safety
			constexpr int_t col = 0;
			constexpr int_t LINE_HEIGHT = 10;
			constexpr int_t LINE_OFFSET = -20;
			
			glTranslatef(0.0f, textYOffset, textZOffset);
			glScalef(textScale, -textScale, textScale);
			glNormal3f(0.0f, 0.0f, -textScale);
			glDepthMask(false);
			
			static int_t SELECTOR_PREFIX_WIDTH = -1;
			static int_t SELECTOR_SUFFIX_WIDTH = -1;
			if (SELECTOR_PREFIX_WIDTH < 0)
			{
				SELECTOR_PREFIX_WIDTH = font->width(u"> ");
				SELECTOR_SUFFIX_WIDTH = font->width(u" <");
			}
			
			for (int_t i = 0; i < 4; i++)
			{
				jstring msg = sign.messages[i];
				if (msg.empty())
					continue;
				
				jstring textLine;
				int_t textWidth;
				
				if (i == sign.selectedLine)
				{
					textLine = u"> " + msg + u" <";
					textWidth = sign.cachedWidths[i] + SELECTOR_PREFIX_WIDTH + SELECTOR_SUFFIX_WIDTH;
				}
				else
				{
					textLine = msg;
					textWidth = sign.cachedWidths[i];
				}
				
				font->draw(textLine, -textWidth / 2, i * LINE_HEIGHT + LINE_OFFSET, col);
			}
			
			glDepthMask(true);
		}
	}
	
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

// Performance optimization: Bake sign text into a texture (CPU-side rendering)
// This eliminates per-character rendering overhead by pre-rendering text once
// Returns true if texture was successfully created/updated
bool SignRenderer::bakeSignTexture(SignTileEntity &sign, Font *font)
{
	if (font == nullptr)
		return false;
	
	// Reset rebuild counter if we're in a new frame
	// We use a simple frame counter - increment on each render call
	// This is approximate but sufficient for throttling
	static int_t frameCounter = 0;
	frameCounter++;
	if (frameCounter != lastFrameNumber)
	{
		rebuildsThisFrame = 0;
		lastFrameNumber = frameCounter;
	}
	
	// Check rebuild budget
	if (rebuildsThisFrame >= MAX_REBUILDS_PER_FRAME)
		return false;  // Over budget, try again next frame
	
	// Load font atlas image
	BufferedImage fontAtlas = font->getFontAtlasImage();
	if (fontAtlas.getWidth() == 0 || fontAtlas.getHeight() == 0)
		return false;
	
	const unsigned char *atlasPixels = fontAtlas.getRawPixels();
	const int_t atlasW = fontAtlas.getWidth();
	const int_t atlasH = fontAtlas.getHeight();
	const auto &charWidths = font->getCharWidths();
	const auto &colorCodeRGBs = font->getColorCodeRGBs();
	
	// Create RGBA buffer for baked text (initialized to transparent)
	const int_t texW = SignTileEntity::TEXT_TEX_WIDTH;
	const int_t texH = SignTileEntity::TEXT_TEX_HEIGHT;
	std::unique_ptr<unsigned char[]> buffer(new unsigned char[texW * texH * 4]);
	std::memset(buffer.get(), 0, texW * texH * 4);  // Initialize to transparent
	
	// Constants matching current sign rendering
	constexpr int_t LINE_HEIGHT = 10;
	constexpr int_t LINE_OFFSET = -20;  // -4 * 5
	constexpr int_t col = 0;  // Default color (white)
	
	// Parse color code helper
	static const jstring colorCodes = u"0123456789abcdef";
	
	// Render each line of text
	for (int_t lineIdx = 0; lineIdx < 4; lineIdx++)
	{
		jstring msg = sign.messages[lineIdx];
		if (msg.empty())
			continue;
		
		// Add selector if this is the selected line
		jstring textLine;
		int_t textWidth;
		if (lineIdx == sign.selectedLine)
		{
			textLine = u"> " + msg + u" <";
			// Calculate width including selectors
			int_t selectorPrefixWidth = font->width(u"> ");
			int_t selectorSuffixWidth = font->width(u" <");
			textWidth = sign.cachedWidths[lineIdx] + selectorPrefixWidth + selectorSuffixWidth;
		}
		else
		{
			textLine = msg;
			textWidth = sign.cachedWidths[lineIdx];
		}
		
		// Calculate line position (centered, matching current rendering)
		// Font coordinate system: Y ranges from -20 to 10 (LINE_OFFSET = -20, LINE_HEIGHT = 10)
		// Texture coordinate system: Y ranges from 0 to 64
		// Map font Y to texture Y: textureY = fontY + 32 (to center in 64px texture)
		// This centers the text vertically in the 64px texture
		int_t lineYFont = lineIdx * LINE_HEIGHT + LINE_OFFSET;  // Font coordinate: -20, -10, 0, 10
		int_t lineY = lineYFont + 32;  // Map to texture coordinate: 12, 22, 32, 42 (centered in 64px)
		int_t startX = (texW / 2) - (textWidth / 2);  // Center the line horizontally (0-128 range)
		int_t currentX = startX;
		
		// Current color (default white)
		int_t currentColor = col;
		float alpha = 1.0f;
		
		// Process color codes and render characters
		for (int_t i = 0; i < textLine.length(); i++)
		{
			char_t ch = textLine[i];
			
			// Handle color codes
			if (ch == 167 && i + 1 < textLine.length())  // 167 = §
			{
				char_t codeChar = textLine[i + 1];
				char_t lowerCode = codeChar;
				if (codeChar >= u'A' && codeChar <= u'F')
					lowerCode = codeChar + (u'a' - u'A');
				else if (codeChar >= u'a' && codeChar <= u'f')
					lowerCode = codeChar;
				else if (codeChar >= u'0' && codeChar <= u'9')
					lowerCode = codeChar;
				
				int_t codeIndex = colorCodes.find(lowerCode);
				if (codeIndex == jstring::npos || codeIndex > 15)
					codeIndex = 15;
				
				// Get color from color code (no darken for signs)
				int_t rgb = colorCodeRGBs[codeIndex];
				int_t r = (rgb >> 16) & 0xFF;
				int_t g = (rgb >> 8) & 0xFF;
				int_t b = rgb & 0xFF;
				
				// Update current color (pack as RGBA)
				currentColor = (0xFF << 24) | (r << 16) | (g << 8) | b;
				
				i++;  // Skip color code character
				continue;
			}
			
			// Render character
			int_t chIndex = SharedConstants::acceptableLetters.find(ch);
			if (chIndex == jstring::npos)
				continue;
			
			// Get character width
			int_t charWidth = charWidths[chIndex + 32];
			
			// Get glyph position in atlas (16x16 grid, 8x8 pixels per glyph)
			int_t glyphX = (chIndex + 32) % 16 * 8;
			int_t glyphY = (chIndex + 32) / 16 * 8;
			
			// Extract color components
			int_t r = (currentColor >> 16) & 0xFF;
			int_t g = (currentColor >> 8) & 0xFF;
			int_t b = currentColor & 0xFF;
			
			// Render glyph pixels to buffer (nearest-neighbor sampling)
			for (int_t gy = 0; gy < 8; gy++)
			{
				int_t destY = lineY + gy;
				if (destY < 0 || destY >= texH)
					continue;
				
				for (int_t gx = 0; gx < 8; gx++)
				{
					int_t destX = currentX + gx;
					if (destX < 0 || destX >= texW)
						continue;
					
					// Sample from font atlas (RGBA)
					int_t atlasX = glyphX + gx;
					int_t atlasY = glyphY + gy;
					int_t atlasIdx = (atlasY * atlasW + atlasX) * 4;
					
					// Get alpha from font atlas (font is grayscale, alpha channel is the mask)
					unsigned char glyphAlpha = atlasPixels[atlasIdx + 3];
					if (glyphAlpha == 0)
						continue;  // Skip transparent pixels
					
					// Font atlas is grayscale - RGB channels are the same
					// We use the alpha channel as the intensity mask
					// Apply text color with glyph alpha as intensity
					// This matches how Font::draw() renders: text color × glyph alpha
					int_t finalR = (r * glyphAlpha) / 255;
					int_t finalG = (g * glyphAlpha) / 255;
					int_t finalB = (b * glyphAlpha) / 255;
					
					// Write to buffer
					int_t destIdx = (destY * texW + destX) * 4;
					buffer[destIdx] = finalR;
					buffer[destIdx + 1] = finalG;
					buffer[destIdx + 2] = finalB;
					buffer[destIdx + 3] = glyphAlpha;
				}
			}
			
			currentX += charWidth;
		}
	}
	
	// Upload to GL texture
	if (sign.textTexId == 0)
	{
		// Create new texture
		glGenTextures(1, &sign.textTexId);
		if (sign.textTexId == 0)
			return false;
		
		glBindTexture(GL_TEXTURE_2D, sign.textTexId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// Use GL_CLAMP (GL 1.1 compatible) - wraps to edge for safety
		// GL_CLAMP_TO_EDGE is GL 1.2+, so we use GL_CLAMP which is safe
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.get());
	}
	else
	{
		// Update existing texture
		glBindTexture(GL_TEXTURE_2D, sign.textTexId);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_BYTE, buffer.get());
	}
	
	sign.textDirty = false;
	sign.cachedSelectedLine = sign.selectedLine;
	rebuildsThisFrame++;
	return true;
}

// Initialize quad VBO for sign text rendering (OpenGL 4.6 optimization)
// Replaces immediate mode glBegin/glEnd with VBO for better performance
void SignRenderer::initQuadVBO()
{
	if (quadVBOInitialized)
		return;
	
	// Quad vertices in font coordinate space: X from -64 to +64, Y from -32 to +32
	// Format: [x, y, z, u, v] per vertex
	// Texture coordinates are flipped V for Y flip (as in original immediate mode)
	constexpr float TEXTURE_COORD_WIDTH = 128.0f;
	constexpr float TEXTURE_COORD_HEIGHT = 64.0f;
	float halfW = TEXTURE_COORD_WIDTH / 2.0f;  // 64.0
	float halfH = TEXTURE_COORD_HEIGHT / 2.0f;  // 32.0
	
	// Quad data: 4 vertices × 5 floats (x, y, z, u, v) = 20 floats
	float quadData[20] = {
		// Top-left: x, y, z, u, v
		-halfW,  halfH, 0.0f,  0.0f, 1.0f,
		// Top-right: x, y, z, u, v
		 halfW,  halfH, 0.0f,  1.0f, 1.0f,
		// Bottom-right: x, y, z, u, v
		 halfW, -halfH, 0.0f,  1.0f, 0.0f,
		// Bottom-left: x, y, z, u, v
		-halfW, -halfH, 0.0f,  0.0f, 0.0f
	};
	
	// Create VBO
	glGenBuffers(1, &quadVBO);
	if (quadVBO != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		quadVBOInitialized = true;
	}
}
