#pragma once

#include "client/gui/Screen.h"
#include "client/Lighting.h"
#include "client/renderer/entity/ItemRenderer.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/item/ItemStack.h"
#include "world/item/crafting/CraftingContainer.h"
#include "world/item/crafting/Recipes.h"
#include "java/String.h"

class Player;
class Level;

// Beta 1.2 CraftingScreen - workbench/crafting table GUI with 3x3 crafting grid
// Reference: beta1.2/minecraft/src/net/minecraft/client/gui/inventory/CraftingScreen.java
// Alpha 1.2.6: GuiWorkbench
class WorkbenchScreen : public Screen
{
private:
	float xMouse = 0.0f;
	float yMouse = 0.0f;
	
	// Beta: imageWidth = 176, imageHeight = 166 (AbstractContainerScreen.java:16-17)
	static constexpr int_t imageWidth = 176;
	static constexpr int_t imageHeight = 166;
	
	// Beta: 3x3 crafting grid (CraftingMenu.java:12)
	std::array<ItemStack, 9> craftSlots;  // 3x3 grid = 9 slots
	ItemStack resultSlot;  // Beta: Result slot (CraftingMenu.java:13)
	
	// Alpha 1.2.6: Public access for multiplayer inventory synchronization
public:
	void setCraftSlot(int_t slot, const ItemStack& stack);
	void setResultSlot(const ItemStack& stack);
	
	// Alpha 1.2.6: Public getters for reading slot state (needed for Packet102WindowClick)
	const ItemStack& getCraftSlot(int_t slot) const;
	const ItemStack& getResultSlot() const { return resultSlot; }
private:
	
	// Workbench position
	int_t workbenchX, workbenchY, workbenchZ;
	
	// Alpha 1.2.6: windowId for multiplayer inventory synchronization
public:
	int_t windowId = 0;
	// Alpha 1.2.6: Transaction counter for multiplayer (Container.field_20917_a)
	short_t transactionCounter = 0;
private:
	
	// CraftingContainer implementation for recipe matching
	class WorkbenchCraftingContainer : public CraftingContainer
	{
	private:
		std::array<ItemStack, 9> &slots;  // Reference to craftSlots
		
	public:
		WorkbenchCraftingContainer(std::array<ItemStack, 9> &slots) : slots(slots) {}
		
		// Beta: getItem(x, y) - maps 3x3 coordinates to 3x3 grid (CraftingContainer.java:29-36)
		// Java: getItem(int var1, int var2) where var1=x (column), var2=y (row)
		// Java: int var3 = var1 + var2 * this.width; -> index = x + y * width
		// Our slots are stored as: col + row * 3, which is x + y * 3
		// This matches the Java implementation
		ItemStack getItem(int_t x, int_t y) override
		{
			if (x < 0 || x >= 3 || y < 0 || y >= 3)
				return ItemStack();  // Out of bounds = empty
			int_t index = x + y * 3;
			return slots[index];
		}
	};
	
	WorkbenchCraftingContainer craftingContainer;  // Container for recipe matching
	
private:
	// Beta: Update crafting result when grid changes (CraftingMenu.java:46-48)
	void updateCraftingResult();
	
	// Beta: Helper method to handle slot clicks - matches AbstractContainerMenu.clicked() logic
	void handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isCraftingSlot);

public:
	WorkbenchScreen(Minecraft &minecraft, Level &level, int_t x, int_t y, int_t z);
	
	void render(int_t xm, int_t ym, float a) override;
	
	bool isPauseScreen() override { return false; }  // Beta: WorkbenchScreen doesn't pause game
	
	// Beta: removed() - called when screen is closed (CraftingScreen.java:14-17)
	void removed() override;
	
protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;
	void mouseClicked(int_t x, int_t y, int_t buttonNum) override;
	
private:
	// Beta: renderBg() - renders background texture (CraftingScreen.java:26-33)
	void renderBg(float a);
	
	// Beta: renderLabels() - renders text labels (CraftingScreen.java:20-23)
	void renderLabels();
	
	// Beta: renderSlot() - renders a single inventory slot
	void renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack);
	
	// Controller slot snapping support (Controlify-style)
	void collectSlotSnapPoints(std::vector<std::pair<int_t, int_t>> &points) override;
	
	// Container screens have slots
	bool hasSlots() const override { return true; }
};
