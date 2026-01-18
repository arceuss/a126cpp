#pragma once

#include "client/gui/Screen.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include <memory>

// Beta 1.2: TextEditScreen.java
// Reference: newb12/net/minecraft/client/gui/inventory/TextEditScreen.java
class TextEditScreen : public Screen
{
protected:
	jstring title = u"Edit sign message:";
	std::shared_ptr<SignTileEntity> sign;
	int_t frame = 0;
	int_t line = 0;

public:
	TextEditScreen(Minecraft &minecraft, std::shared_ptr<SignTileEntity> sign);
	
	virtual void init() override;
	virtual void removed() override;
	virtual void tick() override;
	virtual void buttonClicked(Button &button) override;
	virtual void keyPressed(char_t eventCharacter, int_t eventKey) override;
	virtual void render(int_t xm, int_t ym, float a) override;
};
