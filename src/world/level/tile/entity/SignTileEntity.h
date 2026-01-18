#pragma once

#include "world/level/tile/entity/TileEntity.h"

#include "java/String.h"

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

public:
	SignTileEntity();
	
	virtual jstring getEncodeId() const override { return u"Sign"; }
	
	virtual void load(CompoundTag &tag) override;
	virtual void save(CompoundTag &tag) override;
	
	// Beta: SignTileEntity.getUpdatePacket() returns SignUpdatePacket (SignTileEntity.java:33-41)
	// Note: Returns nullptr for now since Packet system not yet implemented
	virtual void *getUpdatePacket() override { return nullptr; }
};
