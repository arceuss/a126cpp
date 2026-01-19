#pragma once

#include "world/level/tile/entity/TileEntity.h"

#include "java/String.h"
#include "pc/OpenGL.h"

// Beta 1.2 SignTileEntity
// Reference: newb12/net/minecraft/world/level/tile/entity/SignTileEntity.java
// Alpha 1.2.6 equivalent: TileEntitySign
class SignTileEntity : public TileEntity
{
public:
	// Beta: SignTileEntity.messages = new String[]{"", "", "", ""} (SignTileEntity.java:8)
	jstring messages[4] = { u"", u"", u"", u"" };
	
	// Beta: SignTileEntity.selectedLine = -1 (SignTileEntity.java:9)
	int_t selectedLine = -1;
	
	// Performance optimization: Cache font widths to avoid recalculating every frame
	// Only recalculate when messages change
	mutable int_t cachedWidths[4] = { -1, -1, -1, -1 };
	mutable bool widthsDirty = true;
	
	// Performance optimization: Baked text texture for sign rendering
	// Text is baked into a texture once when it changes, then rendered as a single quad
	// This eliminates per-character rendering overhead and state churn
	mutable GLuint textTexId = 0;  // GL texture ID for baked text (0 if not created)
	mutable bool textDirty = true;  // True when text needs to be rebaked
	mutable int_t cachedSelectedLine = -1;  // Track selectedLine for cache invalidation
	
	// Texture dimensions (fixed constants for all signs)
	static constexpr int_t TEXT_TEX_WIDTH = 128;
	static constexpr int_t TEXT_TEX_HEIGHT = 64;
	
	// Cleanup texture on destruction
	~SignTileEntity();
	
	// Invalidate cache when text changes
	void invalidateWidthCache() { widthsDirty = true; }
	void invalidateTextDisplayList() { textDirty = true; }  // Renamed for clarity

public:
	SignTileEntity();
	
	virtual jstring getEncodeId() const override { return u"Sign"; }
	
	virtual void load(CompoundTag &tag) override;
	virtual void save(CompoundTag &tag) override;
	
	// Beta: SignTileEntity.getUpdatePacket() returns SignUpdatePacket (SignTileEntity.java:33-41)
	// Note: Returns nullptr for now since Packet system not yet implemented
	virtual void *getUpdatePacket() override { return nullptr; }
};
