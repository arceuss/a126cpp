#include "world/level/tile/entity/SignTileEntity.h"

#include "nbt/CompoundTag.h"

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
}
