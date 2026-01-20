#pragma once

#include "client/gui/Screen.h"
#include "client/Lighting.h"
#include "client/renderer/entity/ItemRenderer.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/item/ItemStack.h"
#include "java/String.h"
#include <memory>

class Player;
class ChestTileEntity;
class CompoundContainer;
class InventoryBasic;

// Beta 1.2 ChestScreen - chest inventory GUI
// Reference: beta1.2/minecraft/src/net/minecraft/client/gui/inventory/ChestScreen.java
// Alpha 1.2.6: GuiChest
class ChestScreen : public Screen
{
private:
	float xMouse = 0.0f;
	float yMouse = 0.0f;
	
	// Beta: imageWidth = 176, imageHeight varies based on chest rows (ChestScreen.java:15-18)
	static constexpr int_t imageWidth = 176;
	int_t imageHeight;  // Calculated based on chest rows
	
	// Chest inventory - can be single chest, double chest (CompoundContainer), or multiplayer inventory (InventoryBasic)
	std::shared_ptr<ChestTileEntity> chestEntity;  // Single chest
	std::shared_ptr<CompoundContainer> compoundContainer;  // Double chest (null if single)
	std::shared_ptr<InventoryBasic> inventoryBasic;  // Multiplayer chest inventory (Alpha 1.2.6)
	int_t inventoryRows;  // Beta: inventoryRows = chestSize / 9 (ChestScreen.java:17)
	
	
	// Alpha 1.2.6: windowId for multiplayer inventory synchronization
public:
	int_t windowId = 0;
	
private:
	// Beta: Helper method to handle slot clicks
	void handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isChestSlot);

public:
	// Constructor for single chest
	ChestScreen(Minecraft &minecraft, std::shared_ptr<ChestTileEntity> chestEntity);
	
	// Constructor for double chest
	ChestScreen(Minecraft &minecraft, std::shared_ptr<CompoundContainer> compoundContainer);
	
	// Constructor for multiplayer chest (Alpha 1.2.6: uses InventoryBasic)
	ChestScreen(Minecraft &minecraft, std::shared_ptr<InventoryBasic> inventory);
	
	void render(int_t xm, int_t ym, float a) override;
	
	bool isPauseScreen() override { return false; }
	
	// Alpha 1.2.6: Public methods for multiplayer inventory synchronization
	void setChestSlot(int_t slot, std::shared_ptr<ItemStack> stack);
	int_t getChestSize() const;  // Get chest container size
	
protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;
	void mouseClicked(int_t x, int_t y, int_t buttonNum) override;
	
private:
	// Beta: renderBg() - renders chest background texture (ChestScreen.java:26-34)
	void renderBg(float a);
	
	// Beta: renderLabels() - renders text labels (ChestScreen.java:21-24)
	void renderLabels();
	
	// Beta: renderSlot() - renders a single inventory slot
	void renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack);
	
	// Controller slot snapping support (Controlify-style)
	void collectSlotSnapPoints(std::vector<std::pair<int_t, int_t>> &points) override;
	
	// Container screens have slots
	bool hasSlots() const override { return true; }
};
