#pragma once

#include "client/gui/Screen.h"
#include "client/Lighting.h"
#include "client/renderer/entity/ItemRenderer.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/item/ItemStack.h"
#include "java/String.h"
#include <memory>

class Player;
class FurnaceTileEntity;

// Beta 1.2 FurnaceScreen - furnace GUI
// Reference: beta1.2/minecraft/src/net/minecraft/client/gui/inventory/FurnaceScreen.java
// Alpha 1.2.6: GuiFurnace
class FurnaceScreen : public Screen
{
private:
	float xMouse = 0.0f;
	float yMouse = 0.0f;
	
	// Beta: imageWidth = 176, imageHeight = 166 (AbstractContainerScreen.java:16-17)
	static constexpr int_t imageWidth = 176;
	static constexpr int_t imageHeight = 166;
	
	// Furnace tile entity
	std::shared_ptr<FurnaceTileEntity> furnace;
	
	// Alpha 1.2.6: windowId for multiplayer inventory synchronization
public:
	int_t windowId = 0;
	
private:
	// Beta: Helper method to handle slot clicks
	void handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isFurnaceSlot);

public:
	FurnaceScreen(Minecraft &minecraft, std::shared_ptr<FurnaceTileEntity> furnace);
	
	void render(int_t xm, int_t ym, float a) override;
	
	bool isPauseScreen() override { return false; }
	
	// Alpha 1.2.6: Public methods for multiplayer inventory synchronization
	void setFurnaceSlot(int_t slot, std::shared_ptr<ItemStack> stack);
	
	// Alpha 1.2.6: Getter for furnace tile entity (for progress bar updates)
	std::shared_ptr<FurnaceTileEntity> getFurnace() { return furnace; }
	
protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;
	void mouseClicked(int_t x, int_t y, int_t buttonNum) override;
	
private:
	// Beta: renderBg() - renders furnace background texture (FurnaceScreen.java:23-37)
	void renderBg(float a);
	
	// Beta: renderLabels() - renders text labels (FurnaceScreen.java:17-20)
	void renderLabels();
	
	// Beta: renderSlot() - renders a single inventory slot
	void renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack);
	
	// Controller slot snapping support (Controlify-style)
	void collectSlotSnapPoints(std::vector<std::pair<int_t, int_t>> &points) override;
	
	// Container screens have slots
	bool hasSlots() const override { return true; }
};
