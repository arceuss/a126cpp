#pragma once

#include <array>
#include <limits>

#include "world/level/tile/entity/TileEntity.h"

#include "java/String.h"

class Font;

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

private:
	bool renderTextDirty = true;
	bool renderTextListDirty = true;
	int_t renderTextList = 0;
	int_t cachedSelectedLine = std::numeric_limits<int_t>::lowest();
	int_t cachedRenderTextColor = std::numeric_limits<int_t>::lowest();
	std::array<jstring, 4> cachedRenderLines = {};
	std::array<int_t, 4> cachedXOffsets = {};
	void refreshRenderTextCache(Font &font);

public:
	SignTileEntity();
	
	virtual jstring getEncodeId() const override { return u"Sign"; }
	
	virtual void load(CompoundTag &tag) override;
	virtual void save(CompoundTag &tag) override;
	void invalidateRenderCache();
	void getRenderText(Font &font, jstring outLines[4], int_t outXOffsets[4]);
	void renderCachedText(Font &font, int_t baseColor);
	
	// Beta: SignTileEntity.getUpdatePacket() returns SignUpdatePacket (SignTileEntity.java:33-41)
	// Note: Returns nullptr for now since Packet system not yet implemented
	virtual void *getUpdatePacket() override { return nullptr; }
};
