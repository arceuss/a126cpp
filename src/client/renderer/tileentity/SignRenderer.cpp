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
#include <map>
#include <vector>

// Static members for rebuild budget tracking
int_t SignRenderer::rebuildsThisFrame = 0;
int_t SignRenderer::lastFrameNumber = -1;

// Static members for quad VBO and VAO (OpenGL 4.6 optimization)
GLuint SignRenderer::quadVBO = 0;
GLuint SignRenderer::quadVAO = 0;
bool SignRenderer::quadVBOInitialized = false;

// Static members for batch rendering
std::vector<SignRenderer::SignRenderData> SignRenderer::signBatch;
bool SignRenderer::batchStarted = false;
int_t SignRenderer::frameNumber = 0;

// Deferred texture deletion queue (render thread safety)
std::vector<GLuint> SignRenderer::textureDeletionQueue;

// Multithreaded baking (simple worker thread approach)
std::mutex SignRenderer::bakeMutex;
std::queue<SignRenderer::BakeRequest> SignRenderer::pendingBakes;
std::queue<SignRenderer::BakeResult> SignRenderer::completedBakes;
std::atomic<bool> SignRenderer::shouldStopBaking(false);
std::vector<std::thread> SignRenderer::bakingThreads;
bool SignRenderer::bakingThreadStarted = false;

// Cached font data (loaded once, reused for all signs)
std::unique_ptr<unsigned char[]> SignRenderer::cachedFontAtlasPixels;
int_t SignRenderer::cachedFontAtlasW = 0;
int_t SignRenderer::cachedFontAtlasH = 0;
std::vector<int_t> SignRenderer::cachedCharWidths;
std::vector<int_t> SignRenderer::cachedColorCodeRGBs;
bool SignRenderer::fontDataCached = false;

// Cache font data once (font never changes, so load it once and reuse)
void SignRenderer::cacheFontData(Font *font)
{
	if (fontDataCached || font == nullptr)
		return;
	
	const BufferedImage &fontAtlas = font->getFontAtlasImage();  // Get reference (no copy)
	if (fontAtlas.getWidth() == 0 || fontAtlas.getHeight() == 0)
		return;
	
	const unsigned char *atlasPixels = fontAtlas.getRawPixels();
	const int_t atlasW = fontAtlas.getWidth();
	const int_t atlasH = fontAtlas.getHeight();
	const auto &charWidths = font->getCharWidths();
	const auto &colorCodeRGBs = font->getColorCodeRGBs();
	
	// Cache font atlas pixels
	const size_t atlasSize = atlasW * atlasH * 4;
	cachedFontAtlasPixels = std::make_unique<unsigned char[]>(atlasSize);
	std::memcpy(cachedFontAtlasPixels.get(), atlasPixels, atlasSize);
	
	cachedFontAtlasW = atlasW;
	cachedFontAtlasH = atlasH;
	
	// Cache character widths and color codes
	cachedCharWidths = std::vector<int_t>(charWidths.begin(), charWidths.end());
	cachedColorCodeRGBs = std::vector<int_t>(colorCodeRGBs.begin(), colorCodeRGBs.end());
	
	fontDataCached = true;
}

// Worker thread: Does CPU-side baking (text rendering to buffer) off the render thread
void SignRenderer::bakingWorkerThread()
{
	while (!shouldStopBaking)
	{
		BakeRequest request;
		bool hasRequest = false;
		
		// Get next baking request (use move to avoid copy assignment)
		{
			std::lock_guard<std::mutex> lock(bakeMutex);
			if (!pendingBakes.empty())
			{
				request = std::move(pendingBakes.front());
				pendingBakes.pop();
				hasRequest = true;
			}
		}
		
		if (!hasRequest)
		{
			// No work, sleep briefly
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		
		// Do CPU-side baking (no GL calls)
		int_t texW = 0, texH = 0;
		std::unique_ptr<unsigned char[]> buffer = doCPUBaking(request, texW, texH);
		
		if (buffer != nullptr)
		{
			// Queue completed bake for GL upload on render thread
			BakeResult result;
			result.sign = request.sign;
			result.buffer = std::move(buffer);
			result.texW = texW;
			result.texH = texH;
			result.textChanged = request.forceRebuild;
			
			std::lock_guard<std::mutex> lock(bakeMutex);
			completedBakes.push(std::move(result));
		}
	}
}

// Process completed bakes on render thread (uploads to GL)
void SignRenderer::processCompletedBakes()
{
	// Limit GL uploads per frame to avoid stalls, but allow more when many signs are baking
	// Start with a conservative limit, but can be increased if needed
	constexpr int_t MAX_UPLOADS_PER_FRAME = 8;  // Increased from 4 to handle more signs in parallel
	int_t uploadsThisFrame = 0;
	
	std::lock_guard<std::mutex> lock(bakeMutex);
	
	while (!completedBakes.empty() && uploadsThisFrame < MAX_UPLOADS_PER_FRAME)
	{
		BakeResult result = std::move(completedBakes.front());  // Use move to avoid copy
		completedBakes.pop();
		
		if (result.sign == nullptr || result.buffer == nullptr)
			continue;
		
		SignTileEntity &sign = *result.sign;
		bool textChanged = result.textChanged;
		
		// Delete old texture if needed
		if (textChanged && sign.textTexId != 0)
		{
			GLuint oldTexId = sign.textTexId;
			sign.textTexId = 0;
			
			// Unbind if needed
			GLenum glError = glGetError();
			GLint currentTexBinding = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexBinding);
			GLenum checkError = glGetError();
			if (checkError == GL_NO_ERROR && static_cast<GLuint>(currentTexBinding) == oldTexId)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else if (checkError != GL_NO_ERROR)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			
			glDeleteTextures(1, &oldTexId);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		// Create/update texture
		if (sign.textTexId == 0)
		{
			glGenTextures(1, &sign.textTexId);
			if (sign.textTexId == 0)
				continue;
			
			glBindTexture(GL_TEXTURE_2D, sign.textTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result.texW, result.texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, result.buffer.get());
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, sign.textTexId);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result.texW, result.texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, result.buffer.get());
		}
		
		sign.textDirty = false;
		sign.cachedSelectedLine = sign.selectedLine;
		sign.bakingQueued = false;  // Mark baking complete (can queue again if needed)
		rebuildsThisFrame++;
		uploadsThisFrame++;
	}
}

// CPU-side baking work (can run off-thread, no GL calls)
std::unique_ptr<unsigned char[]> SignRenderer::doCPUBaking(const BakeRequest &request, int_t &texW, int_t &texH)
{
	if (request.sign == nullptr || request.atlasPixels == nullptr)
		return nullptr;
	
	texW = SignTileEntity::TEXT_TEX_WIDTH;
	texH = SignTileEntity::TEXT_TEX_HEIGHT;
	
	// Create RGBA buffer
	std::unique_ptr<unsigned char[]> buffer(new unsigned char[texW * texH * 4]);
	std::memset(buffer.get(), 0, texW * texH * 4);
	
	// Use cached font atlas pixels directly (font never changes, so this is safe)
	const unsigned char *atlasPixels = request.atlasPixels;
	const int_t atlasW = request.atlasW;
	const int_t atlasH = request.atlasH;
	const auto &charWidths = request.charWidths;
	const auto &colorCodeRGBs = request.colorCodeRGBs;
	
	// Constants
	constexpr int_t LINE_HEIGHT = 10;
	constexpr int_t LINE_OFFSET = -20;
	constexpr int_t col = 0;
	static const jstring colorCodes = u"0123456789abcdef";
	
	// Render each line
	for (int_t lineIdx = 0; lineIdx < 4; lineIdx++)
	{
		jstring msg = request.sign->messages[lineIdx];
		if (msg.empty())
			continue;
		
		jstring textLine;
		int_t textWidth;
		if (lineIdx == request.sign->selectedLine)
		{
			textLine = u"> " + msg + u" <";
			// Approximate width (can't call font->width() off-thread, use cached)
			textWidth = request.sign->cachedWidths[lineIdx] + 16;  // Rough estimate for "> " and " <"
		}
		else
		{
			textLine = msg;
			textWidth = request.sign->cachedWidths[lineIdx];
		}
		
		int_t lineYFont = lineIdx * LINE_HEIGHT + LINE_OFFSET;
		int_t lineY = lineYFont + 32;
		int_t startX = (texW / 2) - (textWidth / 2);
		int_t currentX = startX;
		int_t currentColor = col;
		
		// Process characters
		for (int_t i = 0; i < textLine.length(); i++)
		{
			char_t ch = textLine[i];
			
			// Handle color codes
			if (ch == 167 && i + 1 < textLine.length())
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
				
				int_t rgb = colorCodeRGBs[codeIndex];
				int_t r = (rgb >> 16) & 0xFF;
				int_t g = (rgb >> 8) & 0xFF;
				int_t b = rgb & 0xFF;
				currentColor = (0xFF << 24) | (r << 16) | (g << 8) | b;
				i++;
				continue;
			}
			
			// Render character
			int_t chIndex = SharedConstants::acceptableLetters.find(ch);
			if (chIndex == jstring::npos)
				continue;
			
			int_t charWidth = charWidths[chIndex + 32];
			int_t glyphX = (chIndex + 32) % 16 * 8;
			int_t glyphY = (chIndex + 32) / 16 * 8;
			
			int_t r = (currentColor >> 16) & 0xFF;
			int_t g = (currentColor >> 8) & 0xFF;
			int_t b = currentColor & 0xFF;
			
			// Render glyph pixels
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
					
					int_t atlasX = glyphX + gx;
					int_t atlasY = glyphY + gy;
					int_t atlasIdx = (atlasY * atlasW + atlasX) * 4;
					
					unsigned char glyphAlpha = atlasPixels[atlasIdx + 3];
					if (glyphAlpha == 0)
						continue;
					
					int_t finalR = (r * glyphAlpha) / 255;
					int_t finalG = (g * glyphAlpha) / 255;
					int_t finalB = (b * glyphAlpha) / 255;
					
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
	
	return buffer;
}

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
SignRenderer::SignRenderer()
{
	// newb12: SignModel is initialized in member initializer
}

// Begin frame - reset batch state
void SignRenderer::beginFrame()
{
	batchStarted = true;
	signBatch.clear();
	frameNumber++;
	
	// Start baking threads if not already started (multiple threads for parallel baking)
	if (!bakingThreadStarted)
	{
		shouldStopBaking = false;
		bakingThreads.clear();
		bakingThreads.reserve(BAKING_THREAD_COUNT);
		// Create multiple worker threads for parallel baking
		for (int_t i = 0; i < BAKING_THREAD_COUNT; i++)
		{
			bakingThreads.emplace_back(bakingWorkerThread);
		}
		bakingThreadStarted = true;
	}
	
	// Cache font data once on first frame (font never changes)
	// We need Font* to cache, so we'll cache it when we have access to Font
	// This will be called from render() when Font is available
	
	// Process completed bakes from worker thread (upload to GL on render thread)
	processCompletedBakes();
	
	// Process deferred texture deletion queue (render thread safe)
	processTextureDeletionQueue();
}

// End frame - flush any remaining batched signs
void SignRenderer::endFrame()
{
	if (batchStarted && !signBatch.empty())
	{
		flushBatch();
	}
	batchStarted = false;
	
	// Process any remaining completed bakes
	processCompletedBakes();
}

// Add sign to batch - pre-calculates all transform data needed for rendering
// Called from render() after model is rendered, stores data for later batched text rendering
void SignRenderer::addToBatch(SignTileEntity &sign, double x, double y, double z, float a)
{
	// Pre-calculate positions and transforms (same as immediate rendering)
	static constexpr float SIZE = 0.6666667f;
	const float signYOffset = 0.75f * SIZE;
	const float textYOffset = 0.5f * SIZE;
	const float textZOffset = 0.07f * SIZE;
	const float textScale = 0.016666668f * SIZE;
	
	const float xf = static_cast<float>(x) + 0.5f;
	const float yf = static_cast<float>(y) + signYOffset;
	const float zf = static_cast<float>(z) + 0.5f;
	
	int_t signData = sign.getData();
	float rot = 0.0f;
	
	// Check if wall sign (same logic as immediate rendering)
	// Critical: Also verify the tile is actually still a sign before adding to batch
	bool isWallSign = false;
	if (sign.level != nullptr && sign.level->hasChunkAt(sign.x, sign.y, sign.z))
	{
		int_t tileId = sign.level->getTile(sign.x, sign.y, sign.z);
		if (tileId >= 0 && tileId < 256)
		{
			Tile *tile = Tile::tiles[tileId];
			if (tile != nullptr)
			{
				// Verify tile is actually still a sign or wall sign
				// This prevents batching signs that have been destroyed/replaced
				if (tile->id != Tile::sign.id && tile->id != Tile::wallSign.id)
					return;  // Block is no longer a sign - don't add to batch
				
				isWallSign = (tile->id != Tile::sign.id);
			}
			else
			{
				return;  // Tile is null - sign was destroyed
			}
		}
		else
		{
			return;  // Invalid tile ID - sign was destroyed
		}
	}
	else
	{
		return;  // Chunk not loaded - can't verify sign exists
	}
	
	if (!isWallSign)
	{
		rot = signData * 22.5f;
	}
	else
	{
		switch (signData)
		{
			case 2: rot = 180.0f; break;
			case 4: rot = 90.0f; break;
			case 5: rot = -90.0f; break;
			default: rot = 0.0f; break;
		}
	}
	
	// Get texture ID (should already be baked by now)
	GLuint textureId = sign.textTexId;
	
	// Only add to batch if texture is valid
	if (textureId != 0)
	{
		// Add to batch
		SignRenderData data;
		data.sign = &sign;
		data.x = x;
		data.y = y;
		data.z = z;
		data.a = a;
		data.xf = xf;
		data.yf = yf;
		data.zf = zf;
		data.rot = rot;
		data.isWallSign = isWallSign;
		data.textureId = textureId;
		data.textScale = textScale;
		data.textYOffset = textYOffset;
		data.textZOffset = textZOffset;
		
		signBatch.push_back(data);
	}
}

// Flush batched signs - render them in original order, minimizing texture binds
// Preserves exact render order while reducing state changes
void SignRenderer::flushBatch()
{
	if (signBatch.empty())
		return;
	
	// Initialize quad VBO once if needed
	if (!quadVBOInitialized)
	{
		initQuadVBO();
	}
	
	// Setup rendering state once per flush (optimization: reduce per-sign GL state setup)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(false);
	
	// Bind quad VBO once per flush (optimization: reduce per-sign VBO binds)
	// Setup VAO once per flush (OpenGL 4.6 optimization: VAO is more efficient than client-side attributes)
	// VAO stores vertex attribute configuration on GPU, avoiding per-draw-call attribute setup
	// Bind VAO once for the entire batch (avoids per-sign bind/unbind overhead)
	bool useVAO = (quadVAO != 0);
	if (useVAO)
	{
		glBindVertexArray(quadVAO);
		// Re-enable client states (VAO stores pointers but not enabled state in compatibility profile)
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else if (quadVBO != 0)
	{
		// Fallback: Setup client-side attributes once if VAO not available
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(0));
		glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(3 * sizeof(float)));
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	
	// Track current texture to minimize binds
	GLuint currentTexture = 0;
	
	// Render all signs in original order (preserves exact visual parity)
	for (const auto &data : signBatch)
	{
		if (data.textureId == 0)
			continue;  // Skip signs with no texture
		
		// Bind texture only if it changed (minimizes texture binds)
		if (data.textureId != currentTexture)
		{
			glBindTexture(GL_TEXTURE_2D, data.textureId);
			currentTexture = data.textureId;
		}
		
		// Apply transforms (same as immediate rendering, preserving order)
		// Note: Matrix stack operations (glPushMatrix/glPopMatrix) are necessary for visual parity
		// Replacing with shader uniforms would require full shader-based rendering system
		glPushMatrix();
		
		glTranslatef(data.xf, data.yf, data.zf);
		glRotatef(-data.rot, 0.0f, 1.0f, 0.0f);
		
		if (data.isWallSign)
		{
			glTranslatef(0.0f, -0.3125f, -0.4375f);
		}
		
		// Render text quad
		glTranslatef(0.0f, data.textYOffset, data.textZOffset);
		glScalef(data.textScale, -data.textScale, data.textScale);
		glNormal3f(0.0f, 0.0f, -data.textScale);
		
		// Draw using VAO (already bound) or VBO with client-side attributes (already set up)
		if (useVAO || quadVBO != 0)
		{
			glDrawArrays(GL_QUADS, 0, 4);
		}
		
		glPopMatrix();
	}
	
	// Clean up VAO/VBO state once per flush
	if (useVAO)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glBindVertexArray(0);
	}
	else if (quadVBO != 0)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	// Restore state
	glDepthMask(true);
	
	// Clear batch for next frame
	signBatch.clear();
}

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
// Now supports batching - renders model immediately, batches text separately
void SignRenderer::render(SignTileEntity &sign, double x, double y, double z, float a)
{
	// Always render the sign model immediately (not batched)
	// Sign model is rendered first, then text is either batched or rendered immediately
	
	// Safety checks (same as before)
	if (sign.level == nullptr)
		return;
	
	if (!sign.level->hasChunkAt(sign.x, sign.y, sign.z))
		return;
	
	int_t tileId = sign.level->getTile(sign.x, sign.y, sign.z);
	if (tileId < 0 || tileId >= 256)
		return;  // Invalid tile ID - sign was destroyed
	
	Tile *tilePtr = Tile::tiles[tileId];
	if (tilePtr == nullptr)
		return;  // Tile is null - sign was destroyed
	
	// Critical: Verify the tile is actually still a sign or wall sign
	// This prevents rendering signs that have been destroyed/replaced (e.g., with fire)
	// Signs can be either SignTile (id == Tile::sign.id) or WallSignTile (id == Tile::wallSign.id)
	if (tilePtr->id != Tile::sign.id && tilePtr->id != Tile::wallSign.id)
	{
		// Block is no longer a sign - invalidate texture to prevent rendering stale data
		if (sign.textTexId != 0)
		{
			// CRITICAL: Delete texture immediately when sign is destroyed/replaced
			// This prevents old textures from appearing when sign is replaced at same location
			glDeleteTextures(1, &sign.textTexId);
			sign.textTexId = 0;
		}
		return;  // Block is no longer a sign - tile entity cleanup may be pending
	}
	
	Tile &tile = *tilePtr;
	glPushMatrix();
	
	// Pre-calculate constants (same as before)
	static constexpr float SIZE = 0.6666667f;
	const float size = SIZE;
	const float textScale = 0.016666668f * size;
	const float signYOffset = 0.75f * size;
	const float textYOffset = 0.5f * size;
	const float textZOffset = 0.07f * size;
	
	const float xf = static_cast<float>(x) + 0.5f;
	const float yf = static_cast<float>(y) + signYOffset;
	const float zf = static_cast<float>(z) + 0.5f;
	
	int_t signData = sign.getData();
	float rot = 0.0f;
	bool isWallSign = (tile.id != Tile::sign.id);
	
	if (!isWallSign)  // Sign post
	{
		glTranslatef(xf, yf, zf);
		rot = signData * 22.5f;
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);
		signModel.cube2.visible = true;
	}
	else
	{
		switch (signData)
		{
			case 2: rot = 180.0f; break;
			case 4: rot = 90.0f; break;
			case 5: rot = -90.0f; break;
			default: rot = 0.0f; break;
		}
		
		glTranslatef(xf, yf, zf);
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, -0.3125f, -0.4375f);
		signModel.cube2.visible = false;
	}
	
	// Render sign model (always immediate)
	bindTexture(u"/item/sign.png");
	glPushMatrix();
	glScalef(size, -size, -size);
	signModel.render();
	glPopMatrix();
	
	// Render sign text (either batched or immediate)
	// CRITICAL: Always create texture when textTexId == 0, even if sign is blank
	// This ensures old textures are cleared when a sign is replaced at the same location
	// New signs start with textTexId = 0 and textDirty = true, so we need to bake immediately
	bool hasVisibleText = false;
	for (int_t i = 0; i < 4; i++)
	{
		if (!sign.messages[i].empty())
		{
			hasVisibleText = true;
			break;
		}
	}
	
	// CRITICAL: Always bake texture if it doesn't exist yet (textTexId == 0)
	// This ensures new signs (even blank ones) get a clean transparent texture
	// This prevents old textures from appearing when a sign is replaced at the same location
	bool needsTexture = (sign.textTexId == 0);
	bool renderText = (hasVisibleText || sign.selectedLine >= 0);
	
	// Always process signs that need textures or have visible text (no distance culling)
	if (renderText || needsTexture)
	{
		Font *font = getFont();
		
		// Update cached widths if needed
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
		
		// Check if baked texture needs rebuilding
		// Rebuild if:
		// 1. Text is dirty (text changed)
		// 2. SelectedLine changed (cursor blinking or editing)
		// 3. No texture exists yet (new sign) - CRITICAL for replacing signs
		bool needsRebuild = sign.textDirty || needsTexture;
		if (!needsRebuild && sign.cachedSelectedLine != sign.selectedLine)
		{
			needsRebuild = true;  // Cursor position changed, rebuild to show/hide selector
		}
		
		// Bake texture if needed (only when dirty - no per-frame uploads)
		// CRITICAL: Always bake when textTexId == 0 to create clean texture for new signs
		if (needsRebuild)
		{
			bakeSignTexture(sign, font);
		}
		
		// If batching is enabled, add text to batch (model already rendered above)
		// Note: We need to pop the matrix before batching since flushBatch() will apply transforms fresh
		// CRITICAL: Only add to batch if texture is valid (non-zero and not deleted)
		if (batchStarted && sign.textTexId != 0)
		{
			// Store current transform state before popping
			// The transform was already applied for the model, so we'll re-apply it in flushBatch()
			addToBatch(sign, x, y, z, a);
			// Note: We don't pop the matrix here - it will be popped after this if block
		}
		else if (sign.textTexId != 0)
		{
			// Immediate text rendering (fallback if batching disabled)
			glTranslatef(0.0f, textYOffset, textZOffset);
			glScalef(textScale, -textScale, textScale);
			glNormal3f(0.0f, 0.0f, -textScale);
			glDepthMask(false);
			
			glBindTexture(GL_TEXTURE_2D, sign.textTexId);
			
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			
			if (!quadVBOInitialized)
			{
				initQuadVBO();
			}
			
			if (quadVAO != 0)
			{
				// Use VAO for maximum performance
				glBindVertexArray(quadVAO);
				glDrawArrays(GL_QUADS, 0, 4);
				glBindVertexArray(0);
			}
			else if (quadVBO != 0)
			{
				// Fallback: Use VBO with deprecated client-side attributes
				glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
				glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(0));
				glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(3 * sizeof(float)));
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glDrawArrays(GL_QUADS, 0, 4);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			
			glDepthMask(true);
		}
	}
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPopMatrix();
}

// Internal immediate rendering (original render logic)
// This renders both the sign model and text immediately (used when batching disabled)
void SignRenderer::renderSignImmediate(SignTileEntity &sign, double x, double y, double z, float a)
{
	// Always render the sign model (not batched)
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
		return;  // Invalid tile ID - sign was destroyed
	
	// Check if tile pointer is null
	Tile *tilePtr = Tile::tiles[tileId];
	if (tilePtr == nullptr)
		return;  // Tile is null - sign was destroyed
	
	// Critical: Verify the tile is actually still a sign or wall sign
	// This prevents rendering signs that have been destroyed/replaced (e.g., with fire)
	if (tilePtr->id != Tile::sign.id && tilePtr->id != Tile::wallSign.id)
	{
		// Block is no longer a sign - invalidate texture to prevent rendering stale data
		if (sign.textTexId != 0)
		{
			// CRITICAL: Delete texture immediately when sign is destroyed/replaced
			// This prevents old textures from appearing when sign is replaced at same location
			glDeleteTextures(1, &sign.textTexId);
			sign.textTexId = 0;
		}
		return;  // Block is no longer a sign - tile entity cleanup may be pending
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
	// CRITICAL: Always bake texture if it doesn't exist yet (textTexId == 0)
	// This ensures new signs (even blank ones) get a clean transparent texture
	// This prevents old textures from appearing when a sign is replaced at the same location
	bool needsTexture = (sign.textTexId == 0);
	bool renderText = (hasVisibleText || sign.selectedLine >= 0);
	
	// Always process signs that need textures or have visible text (no distance culling)
	if (renderText || needsTexture)
	{
		Font *font = getFont();  // newb12: Font font = this.getFont() (SignRenderer.java:47)
		
		// Cache font data once (font never changes, so load it once and reuse)
		// This avoids expensive Font access and data copying on every bake request
		cacheFontData(font);
		
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
		// selectedLine changes only during editing (TextEditScreen), so we check it efficiently here
		// CRITICAL: Always rebuild when textTexId == 0 to create clean texture for new signs
		bool needsRebuild = sign.textDirty || needsTexture;
		
		// Rebuild if selectedLine changed (cursor blinking during editing)
		// This ensures the cursor selector (> text <) appears/disappears in real-time
		if (!needsRebuild && sign.cachedSelectedLine != sign.selectedLine)
		{
			needsRebuild = true;  // selectedLine changed, need to rebuild to show cursor
		}
		
		// Bake texture if needed (with rebuild budget throttling)
		// CRITICAL: Always bake when textTexId == 0 to create clean texture for new signs
		if (needsRebuild)
		{
			bakeSignTexture(sign, font);
		}
		
		// Render baked texture as a single quad
		// CRITICAL: Only render if texture ID is valid (non-zero and not deleted)
		// This prevents rendering stale textures when signs are destroyed/replaced
		if (sign.textTexId != 0)
		{
			// Use pre-calculated text scale
			glTranslatef(0.0f, textYOffset, textZOffset);  // Optimized: use pre-calculated values
			glScalef(textScale, -textScale, textScale);  // Optimized: use pre-calculated value
			glNormal3f(0.0f, 0.0f, -textScale);  // Optimized: use pre-calculated value
			glDepthMask(false);  // newb12: GL11.glDepthMask(false) (SignRenderer.java:52)
			
			// Bind baked texture (only if valid)
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
			
			// Render quad using VAO (fastest) or VBO fallback
			// Quad vertices are in font coordinate space (before textScale is applied)
			if (quadVAO != 0)
			{
				// Use VAO for maximum performance (OpenGL 4.6)
				glBindVertexArray(quadVAO);
				// Re-enable client states (VAO stores pointers but not enabled state in compatibility profile)
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glDrawArrays(GL_QUADS, 0, 4);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
				glBindVertexArray(0);
			}
			else if (quadVBO != 0)
			{
				// Fallback: Use VBO with deprecated client-side attributes
				glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
				glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(0));
				glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(3 * sizeof(float)));
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glDrawArrays(GL_QUADS, 0, 4);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
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

// Performance optimization: Bake sign text into a texture (multithreaded)
// Queues CPU-side baking work to a worker thread, GL uploads happen on render thread
// Returns true if baking was queued (texture will be uploaded later)
bool SignRenderer::bakeSignTexture(SignTileEntity &sign, Font *font)
{
	if (font == nullptr)
		return false;
	
	// Reset rebuild counter if we're in a new frame
	if (frameNumber != lastFrameNumber)
	{
		rebuildsThisFrame = 0;
		lastFrameNumber = frameNumber;
	}
	
	// Check rebuild budget - but ALWAYS allow rebuilds when text is dirty or selectedLine changes
	bool forceRebuild = sign.textDirty || (sign.cachedSelectedLine != sign.selectedLine);
	if (!forceRebuild && rebuildsThisFrame >= MAX_REBUILDS_PER_FRAME)
		return false;  // Over budget, try again next frame
	
	// CRITICAL: Avoid duplicate queue entries
	// If baking is already queued, don't queue again (prevents queue spam and performance issues)
	if (sign.bakingQueued)
		return false;  // Already queued, wait for it to complete
	
	// Cache font data once (font never changes, so load it once and reuse)
	// This avoids expensive Font access and data copying on every bake request
	cacheFontData(font);
	
	if (!fontDataCached)
		return false;  // Font data not available
	
	// Create baking request with reference to cached font data (safe for off-thread access)
	// Since font data is cached and never changes, we can safely reference it
	BakeRequest request;
	request.sign = &sign;
	request.font = font;  // Not used off-thread, just for reference
	
	// Use cached font data instead of copying (font never changes, so this is safe)
	request.atlasPixels = cachedFontAtlasPixels.get();  // Use cached data directly
	request.atlasW = cachedFontAtlasW;
	request.atlasH = cachedFontAtlasH;
	request.charWidths = cachedCharWidths;  // Copy vector (small, worth copying)
	request.colorCodeRGBs = cachedColorCodeRGBs;  // Copy vector (small, worth copying)
	
	request.forceRebuild = forceRebuild;
	
	// Queue for worker thread (use move to avoid copy)
	{
		std::lock_guard<std::mutex> lock(bakeMutex);
		pendingBakes.push(std::move(request));  // Use move to avoid copy assignment
	}
	
	// Mark as queued to prevent duplicate entries
	sign.bakingQueued = true;
	
	return true;  // Queued, will be processed by worker thread
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
	
	// Create VBO and VAO for OpenGL 4.6 (VAO is more efficient than deprecated client-side attributes)
	glGenBuffers(1, &quadVBO);
	if (quadVBO != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
		
		// Create VAO to store vertex attribute configuration (OpenGL 4.6 optimization)
		glGenVertexArrays(1, &quadVAO);
		if (quadVAO != 0)
		{
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			
			// Setup vertex attributes using modern OpenGL (more efficient than glVertexPointer/glTexCoordPointer)
			// Note: We use compatibility profile attributes (GL_VERTEX_ARRAY, GL_TEXTURE_COORD_ARRAY) for visual parity
			// but store them in VAO for better performance
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(0));
			glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<GLvoid *>(3 * sizeof(float)));
			
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		else
		{
			// VAO creation failed, fallback to VBO-only
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		
		quadVBOInitialized = true;
	}
}

// Deferred texture deletion queue (render thread safety)
// Enqueue texture for deletion on render thread
void SignRenderer::enqueueTextureDeletion(GLuint textureId)
{
	if (textureId == 0)
		return;  // Don't queue invalid texture IDs
	
	// Thread-safe enqueue (assuming single-threaded for now, but can be made thread-safe with mutex if needed)
	textureDeletionQueue.push_back(textureId);
}

// Process deferred texture deletion queue (called from render thread)
void SignRenderer::processTextureDeletionQueue()
{
	if (textureDeletionQueue.empty())
		return;
	
	// Delete all queued textures (called on render thread)
	// CRITICAL: Unbind textures before deleting to prevent stale bindings
	for (GLuint textureId : textureDeletionQueue)
	{
		if (textureId != 0)
		{
			// Check if this texture is currently bound and unbind it if so
			// glGetIntegerv can fail with GL_INVALID_OPERATION if called before context is ready
			GLenum glError = glGetError();  // Clear any previous errors
			GLint currentTexBinding = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexBinding);
			GLenum checkError = glGetError();
			if (checkError == GL_NO_ERROR && static_cast<GLuint>(currentTexBinding) == textureId)
			{
				glBindTexture(GL_TEXTURE_2D, 0);  // Unbind before deletion
			}
			else if (checkError != GL_NO_ERROR)
			{
				// OpenGL context not ready or invalid - just unbind directly without checking
				// This is safe even if the texture isn't bound
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			
			glDeleteTextures(1, &textureId);
		}
	}
	
	// Clear queue after processing
	textureDeletionQueue.clear();
}
