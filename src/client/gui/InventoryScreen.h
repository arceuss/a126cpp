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

// Beta 1.2 InventoryScreen - player inventory GUI
// Reference: beta1.2/minecraft/src/net/minecraft/client/gui/inventory/InventoryScreen.java
class InventoryScreen : public Screen
{
private:
	float xMouse = 0.0f;
	float yMouse = 0.0f;
	
	// Beta: imageWidth = 176, imageHeight = 166 (AbstractContainerScreen.java:16-17)
	static constexpr int_t imageWidth = 176;
	static constexpr int_t imageHeight = 166;
	
	// Beta: 2x2 crafting grid (InventoryMenu.java:12)
	std::array<ItemStack, 4> craftSlots;  // 2x2 grid = 4 slots
	ItemStack resultSlot;  // Beta: Result slot (InventoryMenu.java:13)
	
	// Alpha 1.2.6: Transaction counter for multiplayer (Container.field_20917_a)
	short_t transactionCounter = 0;
	
	// CraftingContainer implementation for recipe matching
	class InventoryCraftingContainer : public CraftingContainer
	{
	private:
		std::array<ItemStack, 4> &slots;  // Reference to craftSlots
		
	public:
		InventoryCraftingContainer(std::array<ItemStack, 4> &slots) : slots(slots) {}
		
		// Beta: getItem(x, y) - maps 3x3 coordinates to 2x2 grid (CraftingContainer.java:29-36)
		// For 2x2 grid: only positions (0,0), (0,1), (1,0), (1,1) are valid
		ItemStack getItem(int_t x, int_t y) override
		{
			if (x < 0 || x >= 2 || y < 0 || y >= 2)
				return ItemStack();  // Out of bounds = empty
			int_t index = x + y * 2;
			return slots[index];
		}
	};
	
	InventoryCraftingContainer craftingContainer;  // Container for recipe matching
	
private:
	// Beta: Update crafting result when grid changes (InventoryMenu.java:62-64)
	void updateCraftingResult();
	
	// Beta: Helper method to handle slot clicks - matches AbstractContainerMenu.clicked() logic (AbstractContainerMenu.java:92-179)
	void handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isCraftingSlot);

public:
	InventoryScreen(Minecraft &minecraft);
	
	void render(int_t xm, int_t ym, float a) override;
	
	bool isPauseScreen() override { return false; }  // Beta: InventoryScreen doesn't pause game
	
	// Alpha 1.2.6: Public methods for multiplayer inventory synchronization
	void setCraftSlot(int_t slot, const ItemStack& stack);
	void setResultSlot(const ItemStack& stack);
	
	// Alpha 1.2.6: Public getters for reading slot state (needed for Packet102WindowClick)
	const ItemStack& getCraftSlot(int_t slot) const;
	const ItemStack& getResultSlot() const { return resultSlot; }
	
protected:
	void keyPressed(char_t eventCharacter, int_t eventKey) override;
	void mouseClicked(int_t x, int_t y, int_t buttonNum) override;
	
private:
	// Beta: renderBg() - renders background texture (InventoryScreen.java:30-64)
	void renderBg(float a);
	
	// Beta: renderLabels() - renders text labels (InventoryScreen.java:18-20)
	void renderLabels();
	
	// Beta: renderSlot() - renders a single inventory slot
	void renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack, float a = 0.0f);
	
	// Controller slot snapping support (Controlify-style)
	void collectSlotSnapPoints(std::vector<std::pair<int_t, int_t>> &points) override;
	
	// Container screens have slots
	bool hasSlots() const override { return true; }
};