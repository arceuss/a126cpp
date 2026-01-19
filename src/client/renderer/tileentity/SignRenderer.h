#pragma once

#include "client/renderer/tileentity/TileEntityRenderer.h"
#include "client/model/SignModel.h"
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <memory>

class SignTileEntity;
class Font;
class BufferedImage;

// Beta 1.2: SignRenderer.java
class SignRenderer : public TileEntityRenderer
{
private:
	SignModel signModel;
	
	// Performance: Rebuild budget to avoid stalls when many signs load at once
	// Rebuild at most MAX_REBUILDS_PER_FRAME sign textures per frame
	// Increased to allow more signs to queue for parallel baking
	static constexpr int_t MAX_REBUILDS_PER_FRAME = 16;
	static int_t rebuildsThisFrame;
	static int_t lastFrameNumber;  // Track frame number to reset counter
	
	// Performance: Pre-built VBO and VAO for sign text quad (OpenGL 4.6 optimization)
	// Replaces immediate mode glBegin/glEnd and deprecated client-side vertex attributes
	static GLuint quadVBO;
	static GLuint quadVAO;  // VAO for faster attribute binding
	static bool quadVBOInitialized;
	static void initQuadVBO();
	
	// Performance: Batch rendering state
	struct SignRenderData {
		SignTileEntity *sign;
		double x, y, z;
		float a;
		float xf, yf, zf;  // Pre-calculated float positions
		float rot;
		bool isWallSign;
		GLuint textureId;
		float textScale;
		float textYOffset;
		float textZOffset;
	};
	
	// Per-frame batch collection
	static std::vector<SignRenderData> signBatch;
	static bool batchStarted;
	static int_t frameNumber;
	
	
	// Deferred texture deletion queue (render thread safety)
	static std::vector<GLuint> textureDeletionQueue;
	
	// Multithreaded baking: CPU-side work done off-thread, GL uploads on render thread
	struct BakeRequest {
		SignTileEntity *sign;
		Font *font;  // Not used off-thread, just for reference
		// Use cached font data (font never changes, so we can reference it safely)
		const unsigned char *atlasPixels;  // Pointer to cached data (font never changes)
		int_t atlasW, atlasH;
		std::vector<int_t> charWidths;  // Small vector, worth copying
		std::vector<int_t> colorCodeRGBs;  // Small vector, worth copying
		bool forceRebuild;
		
		// Move constructor/assignment for queue
		BakeRequest() = default;
		BakeRequest(BakeRequest &&) = default;
		BakeRequest &operator=(BakeRequest &&) = default;
		BakeRequest(const BakeRequest &) = delete;
		BakeRequest &operator=(const BakeRequest &) = delete;
	};
	
	struct BakeResult {
		SignTileEntity *sign;
		std::unique_ptr<unsigned char[]> buffer;  // RGBA texture data
		int_t texW, texH;
		bool textChanged;
		
		// Move constructor/assignment for queue
		BakeResult() = default;
		BakeResult(BakeResult &&) = default;
		BakeResult &operator=(BakeResult &&) = default;
		BakeResult(const BakeResult &) = delete;
		BakeResult &operator=(const BakeResult &) = delete;
	};
	
	// Baking queues
	static std::mutex bakeMutex;
	static std::queue<BakeRequest> pendingBakes;
	static std::queue<BakeResult> completedBakes;
	static std::atomic<bool> shouldStopBaking;
	static std::vector<std::thread> bakingThreads;  // Multiple threads for parallel baking
	static bool bakingThreadStarted;
	static constexpr int_t BAKING_THREAD_COUNT = 2;  // Use 2 threads for parallel baking
	
	// Cached font data (loaded once, reused for all signs)
	// Font atlas never changes, so we cache it once to avoid repeated access
	static std::unique_ptr<unsigned char[]> cachedFontAtlasPixels;
	static int_t cachedFontAtlasW;
	static int_t cachedFontAtlasH;
	static std::vector<int_t> cachedCharWidths;
	static std::vector<int_t> cachedColorCodeRGBs;
	static bool fontDataCached;
	static void cacheFontData(Font *font);
	
	// Worker thread function
	static void bakingWorkerThread();
	
	// Process completed bakes on render thread (uploads to GL)
	static void processCompletedBakes();
	
	// Bake sign text into a texture (CPU-side rendering)
	// Returns true if texture was successfully created/updated
	bool bakeSignTexture(SignTileEntity &sign, Font *font);
	
	// Internal: CPU-side baking work (can run off-thread)
	static std::unique_ptr<unsigned char[]> doCPUBaking(const BakeRequest &request, int_t &texW, int_t &texH);
	
	// Internal render implementation (called by both immediate and batch rendering)
	void renderSignImmediate(SignTileEntity &sign, double x, double y, double z, float a);
	
	// Batch rendering methods
	static void beginBatch();
	static void addToBatch(SignTileEntity &sign, double x, double y, double z, float a);
	static void flushBatch();

public:
	SignRenderer();
	
	// Beta: SignRenderer.render() (SignRenderer.java:12-68)
	// Can be used for immediate rendering or batching
	void render(SignTileEntity &sign, double x, double y, double z, float a);
	
	// Helper to render from TileEntity*
	void renderEntity(TileEntity *entity, double x, double y, double z, float a);
	
	// Batch rendering control (called from TileEntityRenderDispatcher)
	static void beginFrame();
	static void endFrame();
	
	
	// Deferred texture deletion methods
	static void enqueueTextureDeletion(GLuint textureId);
	static void processTextureDeletionQueue();
};
