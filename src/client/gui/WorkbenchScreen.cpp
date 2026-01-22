#include "client/gui/WorkbenchScreen.h"

#include <cmath>

#include "client/Minecraft.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/entity/player/Player.h"
#include "world/entity/player/InventoryPlayer.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "network/Packet102WindowClick.h"
#include "network/NetClientHandler.h"
#include "client/gamemode/MultiplayerMode.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"
#include "lwjgl/Keyboard.h"

// Beta 1.2 CraftingScreen constructor (CraftingScreen.java:9-11)
WorkbenchScreen::WorkbenchScreen(Minecraft &minecraft, Level &level, int_t x, int_t y, int_t z) 
	: Screen(minecraft), craftingContainer(craftSlots), workbenchX(x), workbenchY(y), workbenchZ(z)
{
	this->passEvents = true;  // Beta: this.passEvents = true
	
	// Initialize crafting slots and result slot
	for (auto &slot : craftSlots)
	{
		slot = ItemStack();
	}
	resultSlot = ItemStack();
}

// Beta: CraftingScreen.render() - main render method (similar to InventoryScreen)
void WorkbenchScreen::render(int_t xm, int_t ym, float a)
{
	this->xMouse = (float)xm;
	this->yMouse = (float)ym;
	
	// Beta: Render background
	renderBackground();
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	// Beta: renderBg() - renders workbench background texture
	renderBg(a);
	
	// Beta: Setup lighting
	glPushMatrix();
	glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
	Lighting::turnOn();
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef((float)xo, (float)yo, 0.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_RESCALE_NORMAL);
	
	// Beta: Find hovered slot based on mouse position
	int_t hoveredSlot = -1;
	int_t hoveredSlotX = -1;
	int_t hoveredSlotY = -1;
	
	// Mouse position relative to GUI area
	int_t relX = (int_t)xMouse - xo;
	int_t relY = (int_t)yMouse - yo;
	
	// Beta: Render 3x3 crafting grid (CraftingMenu.java:26-30)
	// Crafting slots at 30 + col*18, 17 + row*18
	for (int_t row = 0; row < 3; ++row)
	{
		for (int_t col = 0; col < 3; ++col)
		{
			int_t slotIndex = col + row * 3;
			int_t slotX = 30 + col * 18;
			int_t slotY = 17 + row * 18;
			renderSlot(-1, slotX, slotY, craftSlots[slotIndex]);
			
			// Beta: Check if hovering crafting slot
			if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
			{
				hoveredSlot = -1;
				hoveredSlotX = slotX;
				hoveredSlotY = slotY;
			}
		}
	}
	
	// Beta: Render result slot (CraftingMenu.java:24)
	int_t resultSlotX = 124;
	int_t resultSlotY = 35;
	renderSlot(-2, resultSlotX, resultSlotY, resultSlot);
	
	// Beta: Check if hovering result slot
	if (relX >= resultSlotX - 1 && relX < resultSlotX + 17 && relY >= resultSlotY - 1 && relY < resultSlotY + 17)
	{
		hoveredSlot = -2;
		hoveredSlotX = resultSlotX;
		hoveredSlotY = resultSlotY;
	}
	
	// Beta: Render inventory slots (CraftingMenu.java:32-40)
	// Main inventory slots (9-35) in 3 rows
	for (int_t row = 0; row < 3; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			int_t slotIndex = 9 + col + row * 9;
			int_t slotX = 8 + col * 18;
			int_t slotY = 84 + row * 18;
			
			ItemStack &stack = minecraft.player->inventory.mainInventory[slotIndex];
			renderSlot(slotIndex, slotX, slotY, stack);
			
			// Beta: Check if hovering
			if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
			{
				hoveredSlot = slotIndex;
				hoveredSlotX = slotX;
				hoveredSlotY = slotY;
			}
		}
	}
	
	// Hotbar slots (0-8) at bottom
	for (int_t i = 0; i < 9; ++i)
	{
		int_t slotX = 8 + i * 18;
		int_t slotY = 142;
		
		ItemStack &stack = minecraft.player->inventory.mainInventory[i];
		renderSlot(i, slotX, slotY, stack);
		
		// Beta: Check if hovering
		if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
		{
			hoveredSlot = i;
			hoveredSlotX = slotX;
			hoveredSlotY = slotY;
		}
	}
	
	// Beta: Highlight hovered slot - only if actually hovering a slot
	if (hoveredSlotX >= 0 && hoveredSlotY >= 0)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		fillGradient(hoveredSlotX - 1, hoveredSlotY - 1, hoveredSlotX + 17, hoveredSlotY + 17, 0x80FFFFFF, 0x80FFFFFF);
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	}
	
	// Beta: Render carried item (if any) at mouse position (AbstractContainerScreen.java:62-65)
	ItemStack *carried = minecraft.player->inventory.getCarried();
	if (carried != nullptr && !carried->isEmpty())
	{
		// Beta: Translate forward in Z to render on top (AbstractContainerScreen.java:63)
		glTranslatef(0.0f, 0.0f, 32.0f);  // Beta: GL11.glTranslatef(0.0F, 0.0F, 32.0F) (AbstractContainerScreen.java:63)
		int_t mouseX = relX - 8;  // Beta: var1 - var4 - 8 (AbstractContainerScreen.java:64)
		int_t mouseY = relY - 8;  // Beta: var2 - var5 - 8 (AbstractContainerScreen.java:64)
		EntityRenderDispatcher::itemRenderer.renderGuiItem(font, minecraft.textures, *carried, mouseX, mouseY);
		EntityRenderDispatcher::itemRenderer.renderGuiItemDecorations(font, minecraft.textures, *carried, mouseX, mouseY);
	}
	
	glDisable(GL_RESCALE_NORMAL);
	Lighting::turnOff();
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glPopMatrix();
	// Beta: renderLabels() is called AFTER glPopMatrix, so coordinates are screen-relative (AbstractContainerScreen.java:72)
	renderLabels();
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
}

// Beta: CraftingScreen.renderBg() - renders workbench background texture (CraftingScreen.java:26-33)
void WorkbenchScreen::renderBg(float a)
{
	int_t tex = minecraft.textures.loadTexture(u"/gui/crafting.png");
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	minecraft.textures.bind(tex);
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	blit(xo, yo, 0, 0, imageWidth, imageHeight);
}

// Beta: CraftingScreen.renderLabels() - renders text labels (CraftingScreen.java:20-23)
// Beta: renderLabels() is called AFTER glPopMatrix, so coordinates are screen-relative (AbstractContainerScreen.java:72)
void WorkbenchScreen::renderLabels()
{
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	// Beta: Draw "Crafting" label (CraftingScreen.java:21)
	// Beta: Coordinates are screen-relative: xo + 28, yo + 6
	font.draw(u"Crafting", xo + 28, yo + 6, 0x00404040);
	// Beta: Draw "Inventory" label (CraftingScreen.java:22)
	// Beta: Coordinates are screen-relative: xo + 8, yo + imageHeight - 96 + 2
	font.draw(u"Inventory", xo + 8, yo + imageHeight - 96 + 2, 0x00404040);
}

// Beta: CraftingScreen.renderSlot() - renders a single inventory slot
void WorkbenchScreen::renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack)
{
	if (!itemStack.isEmpty())
	{
		EntityRenderDispatcher::itemRenderer.renderGuiItem(font, minecraft.textures, itemStack, x, y);
		EntityRenderDispatcher::itemRenderer.renderGuiItemDecorations(font, minecraft.textures, itemStack, x, y);
	}
}

// Beta: CraftingScreen.keyPressed() - handles key press
void WorkbenchScreen::keyPressed(char_t eventCharacter, int_t eventKey)
{
	// Beta: Close workbench on ESC
	if (eventKey == lwjgl::Keyboard::KEY_ESCAPE)
	{
		minecraft.setScreen(nullptr);
		minecraft.grabMouse();
		return;
	}
	
	Screen::keyPressed(eventCharacter, eventKey);
}

// Beta: CraftingScreen.mouseClicked() - handles mouse clicks for slot interactions
void WorkbenchScreen::mouseClicked(int_t x, int_t y, int_t buttonNum)
{
	if (buttonNum != 0 && buttonNum != 1)
		return;
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	bool outside = x < xo || y < yo || x >= xo + imageWidth || y >= yo + imageHeight;
	
	ItemStack *carried = minecraft.player->inventory.getCarried();
	
	// Beta: Click outside = drop carried item
	if (outside && carried != nullptr && !carried->isEmpty())
	{
		if (buttonNum == 0)
		{
			minecraft.player->drop(*carried);
			minecraft.player->inventory.setCarriedNull();
		}
		else if (buttonNum == 1)
		{
			ItemStack oneItem = ItemStack(carried->itemID, 1, carried->itemDamage);
			minecraft.player->drop(oneItem);
			carried->stackSize--;
			if (carried->stackSize <= 0)
				minecraft.player->inventory.setCarriedNull();
		}
		return;
	}
	
	// Beta: Check if clicking on crafting grid (3x3) at 30 + col*18, 17 + row*18
	for (int_t row = 0; row < 3; ++row)
	{
		for (int_t col = 0; col < 3; ++col)
		{
			int_t slotX = xo + 30 + col * 18;
			int_t slotY = yo + 17 + row * 18;
			if (x >= slotX - 1 && x < slotX + 17 && y >= slotY - 1 && y < slotY + 17)
			{
				int_t craftSlotIndex = col + row * 3;
				ItemStack &slotStack = craftSlots[craftSlotIndex];
				
				// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
				if (minecraft.isOnline())
				{
					// Workbench container slot mapping: crafting grid slots 0-8 map to slots 1-9
					int_t containerSlot = 1 + craftSlotIndex;  // Map 0->1, 1->2, ..., 8->9
					
					// Capture "before" state before processing locally
					std::unique_ptr<ItemStack> itemBefore = (!slotStack.isEmpty()) ? 
						std::make_unique<ItemStack>(slotStack) : nullptr;
					
					// Process locally first for immediate visual feedback
					handleSlotClick(slotStack, carried, buttonNum, true);
					updateCraftingResult();
					
					// Then send packet to server with "before" state
					MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
					if (mpMode != nullptr)
					{
						mpMode->handleWindowClick(windowId, containerSlot, buttonNum, false, minecraft.player, std::move(itemBefore));
					}
				}
				else
				{
					// Single-player: process locally
					handleSlotClick(slotStack, carried, buttonNum, true);
					updateCraftingResult();
				}
				return;
			}
		}
	}
	
	// Check result slot at 124, 35
	int_t resultX = xo + 124;
	int_t resultY = yo + 35;
	if (x >= resultX - 1 && x < resultX + 17 && y >= resultY - 1 && y < resultY + 17)
	{
		// Beta: ResultSlot.mayPlace() returns false - can only take items, not place them (ResultSlot.java:15-17)
		if (!resultSlot.isEmpty())
		{
			ItemStack *carriedPtr = carried;
			if (carriedPtr == nullptr)
				carriedPtr = minecraft.player->inventory.getCarried();
			
			// Allow crafting if:
			// 1. No carried item, OR
			// 2. Carried item is the same as result and can be stacked
			bool canCraft = false;
			if (carriedPtr == nullptr || carriedPtr->isEmpty())
			{
				// Empty cursor - craft new item
				canCraft = true;
			}
			else
			{
				// Check if carried item matches result and can be stacked
				bool canMerge = (carriedPtr->itemID == resultSlot.itemID &&
				                 carriedPtr->itemDamage == resultSlot.itemDamage &&
				                 carriedPtr->isStackable() && resultSlot.isStackable());
				
				if (canMerge && carriedPtr->stackSize < carriedPtr->getMaxStackSize())
				{
					// Same item, stackable, and there's room for more
					canCraft = true;
				}
			}
			
			if (canCraft)
			{
				// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
				if (minecraft.isOnline())
				{
					// Workbench container slot 0 is the result slot
					int_t containerSlot = 0;
					
					// Capture "before" state before processing locally
					std::unique_ptr<ItemStack> itemBefore = (!resultSlot.isEmpty()) ? 
						std::make_unique<ItemStack>(resultSlot) : nullptr;
					
					// Process locally first for immediate visual feedback
					// Beta: Craft the item - put it on cursor/carried slot or merge with existing
					ItemStack toCraft = resultSlot;
					
					if (carriedPtr != nullptr && !carriedPtr->isEmpty())
					{
						// Merge with existing carried item
						int_t toAdd = resultSlot.stackSize;
						int_t maxStack = carriedPtr->getMaxStackSize();
						if (carriedPtr->stackSize + toAdd > maxStack)
							toAdd = maxStack - carriedPtr->stackSize;
						
						if (toAdd > 0)
						{
							carriedPtr->stackSize += toAdd;
						}
					}
					else
					{
						// Put new item on cursor
						minecraft.player->inventory.setCarried(std::move(toCraft));
					}
					
					// Consume one item from each crafting slot
					for (int_t i = 0; i < 9; ++i)
					{
						if (!craftSlots[i].isEmpty())
						{
							craftSlots[i].stackSize--;
							if (craftSlots[i].stackSize <= 0)
								craftSlots[i] = ItemStack();
						}
					}
					updateCraftingResult();
					
					// Then send packet to server with "before" state
					MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
					if (mpMode != nullptr)
					{
						mpMode->handleWindowClick(windowId, containerSlot, buttonNum, false, minecraft.player, std::move(itemBefore));
					}
				}
				else
				{
					// Single-player: process locally
					// Beta: Craft the item - put it on cursor/carried slot or merge with existing
					// Java: SlotCrafting.onPickupFromSlot() - consumes ingredients when result is picked up
					ItemStack toCraft = resultSlot;
					
					if (carriedPtr != nullptr && !carriedPtr->isEmpty())
					{
						// Merge with existing carried item
						// Add the full recipe output (e.g., 4 planks per log)
						int_t toAdd = resultSlot.stackSize;
						int_t maxStack = carriedPtr->getMaxStackSize();
						if (carriedPtr->stackSize + toAdd > maxStack)
							toAdd = maxStack - carriedPtr->stackSize;
						
						if (toAdd > 0)
						{
							carriedPtr->stackSize += toAdd;
						}
					}
					else
					{
						// Put new item on cursor
						minecraft.player->inventory.setCarried(std::move(toCraft));
					}
					
					// Consume one item from each crafting slot
					for (int_t i = 0; i < 9; ++i)
					{
						if (!craftSlots[i].isEmpty())
						{
							craftSlots[i].stackSize--;
							if (craftSlots[i].stackSize <= 0)
								craftSlots[i] = ItemStack();
						}
					}
					updateCraftingResult();
				}
			}
		}
		// Beta: If clicking with something carried that doesn't match, do nothing (ResultSlot.mayPlace() returns false)
		return;
	}
	
	// Beta: Find which inventory slot was clicked
	int_t clickedSlot = -1;
	
	// Check hotbar slots (0-8) at y = 142
	if (y >= yo + 142 && y < yo + 142 + 18 && x >= xo + 8 && x < xo + 8 + 9 * 18)
	{
		int_t col = (x - (xo + 8)) / 18;
		if (col >= 0 && col < 9)
			clickedSlot = col;
	}
	
	// Check main inventory slots (9-35) in 3 rows
	if (clickedSlot == -1)
	{
		for (int_t row = 0; row < 3; ++row)
		{
			int_t rowY = yo + 84 + row * 18;
			if (y >= rowY && y < rowY + 18 && x >= xo + 8 && x < xo + 8 + 9 * 18)
			{
				int_t col = (x - (xo + 8)) / 18;
				if (col >= 0 && col < 9)
				{
					clickedSlot = 9 + col + row * 9;
					break;
				}
			}
		}
	}
	
	// Beta: Handle inventory slot click
	if (clickedSlot >= 0 && clickedSlot < 36)
	{
		ItemStack &slotStack = minecraft.player->inventory.mainInventory[clickedSlot];
		
		// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
		if (minecraft.isOnline())
		{
			// ContainerWorkbench slot mapping (from ContainerWorkbench.java):
			// Slot 0: Result
			// Slots 1-9: Crafting matrix (3x3)
			// Slots 10-36: Main inventory (player slots 9-35)
			// Slots 37-45: Hotbar (player slots 0-8)
			int_t containerSlot;
			if (clickedSlot < 9)
			{
				containerSlot = 37 + clickedSlot;  // Hotbar: 0-8 -> 37-45
			}
			else
			{
				containerSlot = 10 + (clickedSlot - 9);  // Main inventory: 9-35 -> 10-36
			}
			
			// Capture "before" state before processing locally
			std::unique_ptr<ItemStack> itemBefore = (!slotStack.isEmpty()) ? 
				std::make_unique<ItemStack>(slotStack) : nullptr;
			
			// Process locally first for immediate visual feedback
			handleSlotClick(slotStack, carried, buttonNum, false);
			
			// Then send packet to server with "before" state
			MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
			if (mpMode != nullptr)
			{
				mpMode->handleWindowClick(windowId, containerSlot, buttonNum, false, minecraft.player, std::move(itemBefore));
			}
		}
		else
		{
			// Single-player: process locally
			handleSlotClick(slotStack, carried, buttonNum, false);
		}
	}
	
	Screen::mouseClicked(x, y, buttonNum);
}

// Beta: Helper method to handle slot clicks - same as InventoryScreen
void WorkbenchScreen::handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isCraftingSlot)
{
	if (slotStack.isEmpty() && carried == nullptr)
		return;
	
	ItemStack *carriedPtr = carried;
	if (carriedPtr == nullptr)
		carriedPtr = minecraft.player->inventory.getCarried();
	
	// Beta: Slot has item, nothing carried - pick up
	if (!slotStack.isEmpty() && (carriedPtr == nullptr || carriedPtr->isEmpty()))
	{
		int_t toTake = (buttonNum == 0) ? slotStack.stackSize : (slotStack.stackSize + 1) / 2;
		ItemStack pickedUp = slotStack;
		pickedUp.stackSize = toTake;
		minecraft.player->inventory.setCarried(std::move(pickedUp));
		
		slotStack.stackSize -= toTake;
		if (slotStack.stackSize <= 0)
			slotStack = ItemStack();
		
		if (isCraftingSlot)
			updateCraftingResult();
		return;
	}
	
	// Beta: Slot empty, something carried - place item
	if (slotStack.isEmpty() && carriedPtr != nullptr && !carriedPtr->isEmpty())
	{
		int_t toPlace = (buttonNum == 0) ? carriedPtr->stackSize : 1;
		int_t maxStack = slotStack.getMaxStackSize();
		if (toPlace > maxStack)
			toPlace = maxStack;
		
		slotStack = ItemStack(carriedPtr->itemID, toPlace, carriedPtr->itemDamage);
		carriedPtr->stackSize -= toPlace;
		if (carriedPtr->stackSize <= 0)
			minecraft.player->inventory.setCarriedNull();
		
		if (isCraftingSlot)
			updateCraftingResult();
		return;
	}
	
	// Beta: Both slot and carried have items - swap or merge
	if (!slotStack.isEmpty() && carriedPtr != nullptr && !carriedPtr->isEmpty())
	{
		bool canMerge = (slotStack.itemID == carriedPtr->itemID && 
		                 slotStack.itemDamage == carriedPtr->itemDamage &&
		                 slotStack.isStackable() && carriedPtr->isStackable());
		
		if (!canMerge)
		{
			if (carriedPtr->stackSize <= slotStack.getMaxStackSize())
			{
				ItemStack temp = slotStack;
				slotStack = *carriedPtr;
				minecraft.player->inventory.setCarried(std::move(temp));
				if (isCraftingSlot)
					updateCraftingResult();
			}
			return;
		}
		
		if (canMerge)
		{
			if (buttonNum == 0)
			{
				int_t toAdd = carriedPtr->stackSize;
				int_t maxStack = slotStack.getMaxStackSize();
				if (toAdd > maxStack - slotStack.stackSize)
					toAdd = maxStack - slotStack.stackSize;
				
				carriedPtr->stackSize -= toAdd;
				if (carriedPtr->stackSize <= 0)
					minecraft.player->inventory.setCarriedNull();
				slotStack.stackSize += toAdd;
			}
			else if (buttonNum == 1)
			{
				int_t toAdd = 1;
				int_t maxStack = slotStack.getMaxStackSize();
				if (toAdd > maxStack - slotStack.stackSize)
					toAdd = maxStack - slotStack.stackSize;
				
				carriedPtr->stackSize -= toAdd;
				if (carriedPtr->stackSize <= 0)
					minecraft.player->inventory.setCarriedNull();
				slotStack.stackSize += toAdd;
			}
			
			if (isCraftingSlot)
				updateCraftingResult();
		}
	}
}

// Beta: Update crafting result when grid changes (CraftingMenu.java:46-48)
void WorkbenchScreen::updateCraftingResult()
{
	// Beta: Use Recipes system to find matching recipe
	resultSlot = Recipes::getInstance().getItemFor(craftingContainer);
}

// Beta: removed() - called when screen is closed (CraftingScreen.java:14-17, CraftingMenu.java:51-60)
void WorkbenchScreen::removed()
{
	// Beta: Drop items from crafting grid when closing (CraftingMenu.java:54-59)
	for (int_t i = 0; i < 9; ++i)
	{
		if (!craftSlots[i].isEmpty())
		{
			minecraft.player->drop(craftSlots[i]);
			craftSlots[i] = ItemStack();
		}
	}
	
	Screen::removed();
}

// Alpha 1.2.6: Public methods for multiplayer inventory synchronization
void WorkbenchScreen::setCraftSlot(int_t slot, const ItemStack& stack)
{
	if (slot >= 0 && slot < 9)
	{
		craftSlots[slot] = stack;
		updateCraftingResult();  // Update result when crafting grid changes
	}
}

void WorkbenchScreen::setResultSlot(const ItemStack& stack)
{
	resultSlot = stack;
}

const ItemStack& WorkbenchScreen::getCraftSlot(int_t slot) const
{
	if (slot >= 0 && slot < 9)
		return craftSlots[slot];
	static const ItemStack empty;
	return empty;
}
