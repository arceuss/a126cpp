#include "client/gui/FurnaceScreen.h"

#include <cmath>

#include "client/Minecraft.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/entity/player/Player.h"
#include "world/entity/player/InventoryPlayer.h"
#include "world/level/tile/entity/FurnaceTileEntity.h"
#include "client/gamemode/MultiplayerMode.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"
#include "lwjgl/Keyboard.h"

// Beta 1.2 FurnaceScreen constructor (FurnaceScreen.java:11-14)
FurnaceScreen::FurnaceScreen(Minecraft &minecraft, std::shared_ptr<FurnaceTileEntity> furnace) 
	: Screen(minecraft), furnace(furnace)
{
	this->passEvents = false;
}

// Beta: FurnaceScreen.render() - main render method
void FurnaceScreen::render(int_t xm, int_t ym, float a)
{
	this->xMouse = (float)xm;
	this->yMouse = (float)ym;
	
	renderBackground();
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	renderBg(a);
	
	// Beta: Turn on lighting before rendering items (AbstractContainerScreen.java:36-39)
	glPushMatrix();
	glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
	Lighting::turnOn();
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef((float)xo, (float)yo, 0.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_RESCALE_NORMAL);
	
	// Beta: Find hovered slot
	int_t hoveredSlot = -1;
	int_t hoveredSlotX = -1;
	int_t hoveredSlotY = -1;
	
	int_t relX = (int_t)xMouse - xo;
	int_t relY = (int_t)yMouse - yo;
	
	// Beta: Render furnace slots (FurnaceMenu.java:15-17)
	// Slot 0: Input (56, 17)
	// Slot 1: Fuel (56, 53)
	// Slot 2: Output (116, 35)
	std::shared_ptr<ItemStack> item0 = furnace->getItem(0);
	std::shared_ptr<ItemStack> item1 = furnace->getItem(1);
	std::shared_ptr<ItemStack> item2 = furnace->getItem(2);
	
	ItemStack stack0 = (item0 != nullptr) ? *item0 : ItemStack();
	ItemStack stack1 = (item1 != nullptr) ? *item1 : ItemStack();
	ItemStack stack2 = (item2 != nullptr) ? *item2 : ItemStack();
	
	renderSlot(0, 56, 17, stack0);
	renderSlot(1, 56, 53, stack1);
	renderSlot(2, 116, 35, stack2);
	
	// Check if hovering furnace slots
	if (relX >= 56 - 1 && relX < 56 + 17 && relY >= 17 - 1 && relY < 17 + 17)
	{
		hoveredSlot = 0;
		hoveredSlotX = 56;
		hoveredSlotY = 17;
	}
	else if (relX >= 56 - 1 && relX < 56 + 17 && relY >= 53 - 1 && relY < 53 + 17)
	{
		hoveredSlot = 1;
		hoveredSlotX = 56;
		hoveredSlotY = 53;
	}
	else if (relX >= 116 - 1 && relX < 116 + 17 && relY >= 35 - 1 && relY < 35 + 17)
	{
		hoveredSlot = 2;
		hoveredSlotX = 116;
		hoveredSlotY = 35;
	}
	
	// Beta: Render player inventory slots (FurnaceMenu.java:19-27)
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
			
			if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
			{
				hoveredSlot = slotIndex + 1000;
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
		
		if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
		{
			hoveredSlot = i + 1000;
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
	
	// Beta: Render carried item (AbstractContainerScreen.java:62-65)
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

// Beta: FurnaceScreen.renderBg() - renders furnace background texture (FurnaceScreen.java:23-37)
void FurnaceScreen::renderBg(float a)
{
	int_t tex = minecraft.textures.loadTexture(u"/gui/furnace.png");
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	minecraft.textures.bind(tex);
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	// Beta: Render base GUI (FurnaceScreen.java:29)
	blit(xo, yo, 0, 0, imageWidth, imageHeight);
	
	// Beta: Render burn progress (FurnaceScreen.java:30-33)
	if (furnace->isLit())
	{
		int_t p = furnace->getLitProgress(12);  // Beta: int p = this.furnace.getLitProgress(12) (FurnaceScreen.java:31)
		// Beta: this.blit(xo + 56, yo + 36 + 12 - p, 176, 12 - p, 14, p + 2) (FurnaceScreen.java:32)
		blit(xo + 56, yo + 36 + 12 - p, 176, 12 - p, 14, p + 2);
	}
	
	// Beta: Render cook progress (FurnaceScreen.java:35-36)
	int_t p = furnace->getBurnProgress(24);  // Beta: int p = this.furnace.getBurnProgress(24) (FurnaceScreen.java:35)
	// Beta: this.blit(xo + 79, yo + 34, 176, 14, p + 1, 16) (FurnaceScreen.java:36)
	blit(xo + 79, yo + 34, 176, 14, p + 1, 16);
}

// Beta: FurnaceScreen.renderLabels() - renders text labels (FurnaceScreen.java:17-20)
// Beta: renderLabels() is called AFTER glPopMatrix, so coordinates are screen-relative (AbstractContainerScreen.java:72)
void FurnaceScreen::renderLabels()
{
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	// Beta: Draw "Furnace" label (FurnaceScreen.java:18)
	// Beta: Coordinates are screen-relative: xo + 60, yo + 6
	font.draw(u"Furnace", xo + 60, yo + 6, 0x00404040);
	// Beta: Draw "Inventory" label (FurnaceScreen.java:19)
	// Beta: Coordinates are screen-relative: xo + 8, yo + imageHeight - 96 + 2
	font.draw(u"Inventory", xo + 8, yo + imageHeight - 96 + 2, 0x00404040);
}

// Beta: FurnaceScreen.renderSlot() - renders a single inventory slot
void FurnaceScreen::renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack)
{
	if (!itemStack.isEmpty())
	{
		EntityRenderDispatcher::itemRenderer.renderGuiItem(font, minecraft.textures, itemStack, x, y);
		EntityRenderDispatcher::itemRenderer.renderGuiItemDecorations(font, minecraft.textures, itemStack, x, y);
	}
}

// Beta: FurnaceScreen.keyPressed() - handles key press
void FurnaceScreen::keyPressed(char_t eventCharacter, int_t eventKey)
{
	if (eventKey == lwjgl::Keyboard::KEY_ESCAPE)
	{
		minecraft.setScreen(nullptr);
		minecraft.grabMouse();
		return;
	}
	
	Screen::keyPressed(eventCharacter, eventKey);
}

// Alpha 1.2.6: Public method for multiplayer inventory synchronization
void FurnaceScreen::setFurnaceSlot(int_t slot, std::shared_ptr<ItemStack> stack)
{
	// Furnace has 3 slots: 0 = input, 1 = fuel, 2 = output
	if (slot >= 0 && slot < 3 && furnace != nullptr)
	{
		furnace->setItem(slot, stack);
	}
}

// Beta: FurnaceScreen.mouseClicked() - handles mouse clicks
void FurnaceScreen::mouseClicked(int_t x, int_t y, int_t buttonNum)
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
	
	// Beta: Check furnace slots
	// Slot 0: Input (56, 17)
	int_t slot0X = xo + 56;
	int_t slot0Y = yo + 17;
	if (x >= slot0X - 1 && x < slot0X + 17 && y >= slot0Y - 1 && y < slot0Y + 17)
	{
		std::shared_ptr<ItemStack> itemPtr = furnace->getItem(0);
		ItemStack slotStack = (itemPtr != nullptr) ? *itemPtr : ItemStack();
		
		// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
		if (minecraft.isOnline())
		{
			// Java: Container.func_27280_a() line 84-87:
			//       ItemStack var13 = var12.getStack();  // Can be null
			//       if(var13 != null) {
			//           var5 = var13.copy();
			//       }
			// Java checks var13 != null (pointer check), not isEmpty()
			// So we MUST check itemPtr != nullptr to match Java's null check!
			std::unique_ptr<ItemStack> itemBefore = (itemPtr != nullptr) ? 
				std::make_unique<ItemStack>(*itemPtr) : nullptr;
			
			// Process locally first for immediate visual feedback
			handleSlotClick(slotStack, carried, buttonNum, true);
			
			// Update local furnace entity
			if (slotStack.isEmpty())
				furnace->setItem(0, nullptr);
			else
				furnace->setItem(0, std::make_shared<ItemStack>(slotStack));
			
			// Send packet to server - furnace slot 0 maps to container slot 0
			// Send the BEFORE state for server validation (Container.func_27280_a returns BEFORE state)
			MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
			if (mpMode != nullptr)
			{
				mpMode->handleWindowClick(windowId, 0, buttonNum, false, minecraft.player, std::move(itemBefore));
			}
		}
		else
		{
			// Single-player: process locally
			handleSlotClick(slotStack, carried, buttonNum, true);
			if (slotStack.isEmpty())
				furnace->setItem(0, nullptr);
			else
				furnace->setItem(0, std::make_shared<ItemStack>(slotStack));
		}
		return;
	}
	
	// Slot 1: Fuel (56, 53)
	int_t slot1X = xo + 56;
	int_t slot1Y = yo + 53;
	if (x >= slot1X - 1 && x < slot1X + 17 && y >= slot1Y - 1 && y < slot1Y + 17)
	{
		std::shared_ptr<ItemStack> itemPtr = furnace->getItem(1);
		ItemStack slotStack = (itemPtr != nullptr) ? *itemPtr : ItemStack();
		
		// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
		if (minecraft.isOnline())
		{
			// Java: Container.func_27280_a() line 84-87:
			//       ItemStack var13 = var12.getStack();  // Can be null
			//       if(var13 != null) {
			//           var5 = var13.copy();
			//       }
			// Java checks var13 != null (pointer check), not isEmpty()
			// So we MUST check itemPtr != nullptr to match Java's null check!
			std::unique_ptr<ItemStack> itemBefore = (itemPtr != nullptr) ? 
				std::make_unique<ItemStack>(*itemPtr) : nullptr;
			
			// Process locally first for immediate visual feedback
			handleSlotClick(slotStack, carried, buttonNum, true);
			
			// Update local furnace entity
			if (slotStack.isEmpty())
				furnace->setItem(1, nullptr);
			else
				furnace->setItem(1, std::make_shared<ItemStack>(slotStack));
			
			// Send packet to server - furnace slot 1 maps to container slot 1
			// Send the BEFORE state for server validation (Container.func_27280_a returns BEFORE state)
			MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
			if (mpMode != nullptr)
			{
				mpMode->handleWindowClick(windowId, 1, buttonNum, false, minecraft.player, std::move(itemBefore));
			}
		}
		else
		{
			// Single-player: process locally
			handleSlotClick(slotStack, carried, buttonNum, true);
			if (slotStack.isEmpty())
				furnace->setItem(1, nullptr);
			else
				furnace->setItem(1, std::make_shared<ItemStack>(slotStack));
		}
		return;
	}
	
	// Slot 2: Output (116, 35) - like ResultSlot, can only take items, not place them
	int_t slot2X = xo + 116;
	int_t slot2Y = yo + 35;
	if (x >= slot2X - 1 && x < slot2X + 17 && y >= slot2Y - 1 && y < slot2Y + 17)
	{
		// Beta: Output slot can only take items, not place them (like ResultSlot.mayPlace() returns false)
		std::shared_ptr<ItemStack> itemPtr = furnace->getItem(2);
		if (itemPtr != nullptr && !itemPtr->isEmpty())
		{
			ItemStack *carriedPtr = carried;
			if (carriedPtr == nullptr)
				carriedPtr = minecraft.player->inventory.getCarried();
			
			// Allow taking from output slot if:
			// 1. No carried item, OR
			// 2. Carried item is the same as output and can be stacked
			bool canTake = false;
			if (carriedPtr == nullptr || carriedPtr->isEmpty())
			{
				// Empty cursor - take output item
				canTake = true;
			}
			else
			{
				// Check if carried item matches output and can be stacked
				bool canMerge = (carriedPtr->itemID == itemPtr->itemID &&
				                 carriedPtr->itemDamage == itemPtr->itemDamage &&
				                 carriedPtr->isStackable() && itemPtr->isStackable());
				
				if (canMerge && carriedPtr->stackSize < carriedPtr->getMaxStackSize())
				{
					// Same item, stackable, and there's room for more
					canTake = true;
				}
			}
			
			if (canTake)
			{
				int_t toTake = (buttonNum == 0) ? itemPtr->stackSize : (itemPtr->stackSize + 1) / 2;
				
				// Java: Container.func_27280_a() captures var5 = var13.copy() (BEFORE state) at line 87,
				//       then processes the click, then returns var5 (BEFORE state)
				//       Server validates: if (ItemStack.equals(packet.itemStack, serverResult)) accept
				// So we MUST send the BEFORE state, not the after state!
				std::unique_ptr<ItemStack> itemBefore = std::make_unique<ItemStack>(*itemPtr);
				
				bool itemChanged = false;
				if (carriedPtr != nullptr && !carriedPtr->isEmpty())
				{
					// Merge with existing carried item
					int_t maxStack = carriedPtr->getMaxStackSize();
					int_t availableSpace = maxStack - carriedPtr->stackSize;
					if (toTake > availableSpace)
						toTake = availableSpace;
					
					if (toTake > 0)
					{
						carriedPtr->stackSize += toTake;
						itemPtr->stackSize -= toTake;
						
						if (itemPtr->stackSize <= 0)
							furnace->setItem(2, nullptr);
						else
							furnace->setItem(2, itemPtr);
						
						itemChanged = true;
					}
				}
				else
				{
					// Put output item on cursor
					ItemStack pickedUp = *itemPtr;
					pickedUp.stackSize = toTake;
					minecraft.player->inventory.setCarried(std::move(pickedUp));
					
					itemPtr->stackSize -= toTake;
					if (itemPtr->stackSize <= 0)
						furnace->setItem(2, nullptr);
					else
						furnace->setItem(2, itemPtr);
					
					itemChanged = true;
				}
				
				// Alpha 1.2.6: Send packet to server - furnace slot 2 maps to container slot 2
				// Only send if we actually made a change
				// Send the BEFORE state for server validation (Container.func_27280_a returns BEFORE state)
				if (itemChanged && minecraft.isOnline())
				{
					MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
					if (mpMode != nullptr)
					{
						mpMode->handleWindowClick(windowId, 2, buttonNum, false, minecraft.player, std::move(itemBefore));
					}
				}
			}
		}
		// Beta: If clicking with something carried that doesn't match, do nothing (output slot doesn't accept items)
		return;
	}
	
	// Beta: Check player inventory slots
	// Alpha 1.2.6: ContainerFurnace slot mapping (ContainerFurnace.java:9-24):
	// Slot 0: Input (furnace slot 0)
	// Slot 1: Fuel (furnace slot 1)
	// Slot 2: Output (furnace slot 2)
	// Slots 3-29: Player main inventory slots 9-35 (from line 18: var4 + var3 * 9 + 9 where var3=0-2, var4=0-8)
	// Slots 30-38: Player hotbar slots 0-8 (from line 23: var3 where var3=0-8)
	
	// Check hotbar slots (0-8) - map to container slots 30-38
	if (y >= yo + 142 && y < yo + 142 + 18 && x >= xo + 8 && x < xo + 8 + 9 * 18)
	{
		int_t col = (x - (xo + 8)) / 18;
		if (col >= 0 && col < 9)
		{
			int_t playerSlot = col;  // Player hotbar slot 0-8
			ItemStack &slotStack = minecraft.player->inventory.mainInventory[playerSlot];
			
			// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
			if (minecraft.isOnline())
			{
				// ContainerFurnace slot mapping: player hotbar slots 0-8 map to container slots 30-38
				int_t containerSlot = 30 + playerSlot;  // Map 0->30, 1->31, ..., 8->38
				
				// Capture "before" state BEFORE processing locally (Java's func_27280_a returns what was in slot before click)
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
			return;
		}
	}
	
	// Check main inventory slots (9-35) - map to container slots 3-29
	for (int_t row = 0; row < 3; ++row)
	{
		int_t rowY = yo + 84 + row * 18;
		if (y >= rowY && y < rowY + 18 && x >= xo + 8 && x < xo + 8 + 9 * 18)
		{
			int_t col = (x - (xo + 8)) / 18;
			if (col >= 0 && col < 9)
			{
				int_t playerSlot = 9 + col + row * 9;  // Player main inventory slot 9-35
				ItemStack &slotStack = minecraft.player->inventory.mainInventory[playerSlot];
				
				// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
				if (minecraft.isOnline())
				{
					// ContainerFurnace slot mapping: player main inventory slots 9-35 map to container slots 3-29
					// Java: var4 + var3 * 9 + 9 where var3=0-2, var4=0-8
					// Player slot 9 → container slot 3
					// Player slot 18 → container slot 12
					// Player slot 27 → container slot 21
					// Formula: containerSlot = 3 + (playerSlot - 9) = playerSlot - 6
					int_t containerSlot = 3 + (playerSlot - 9);  // Map 9->3, 10->4, ..., 35->29
					
					// Capture "before" state BEFORE processing locally (Java's func_27280_a returns what was in slot before click)
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
				return;
			}
		}
	}
	
	Screen::mouseClicked(x, y, buttonNum);
}

// Alpha 1.2.6: Helper method to handle slot clicks - matches Container.func_27280_a() exactly
// Java: Container.func_27280_a(int var1, int var2, boolean var3, EntityPlayer var4)
// var1 = slot, var2 = mouseClick (0 or 1), var3 = shiftClick (always false for furnaces), var4 = player
// Returns var5 (BEFORE state) - ItemStack that was in slot before click, or null if empty
void FurnaceScreen::handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isFurnaceSlot)
{
	// Java: InventoryPlayer var6 = var4.inventory; (line 50)
	ItemStack *carriedPtr = carried;
	if (carriedPtr == nullptr)
		carriedPtr = minecraft.player->inventory.getCarried();
	
	// Java: Slot var12 = (Slot)this.slots.get(var1); (line 81)
	// Java: if(var12 != null) { (line 82)
	// Java: var12.onSlotChanged(); (line 83) - we skip this as we don't have Slot objects
	// Java: ItemStack var13 = var12.getStack(); (line 84) - this is slotStack (can be empty/null)
	// Java: ItemStack var14 = var6.getItemStack(); (line 85) - this is carriedPtr (can be null)
	// Java: if(var13 != null) { var5 = var13.copy(); } (line 86-88) - we do this BEFORE calling handleSlotClick
	
	int_t var7;  // Java: int var7; (line 66, 92, 103, 118, 135)
	int_t slotStackLimit = 64;  // Java: var12.getSlotStackLimit() - furnace slots have limit of 64
	
	// Java: if(var13 == null) { (line 90) - slot is empty
	if (slotStack.isEmpty())
	{
		// Java: if(var14 != null && var12.isItemValid(var14)) { (line 91)
		// For furnace slots 0 and 1, isItemValid() returns true (Slot.isItemValid returns true)
		// Slot 2 (output) uses SlotFurnace which returns false, but we handle output separately
		if (carriedPtr != nullptr && !carriedPtr->isEmpty())
		{
			// Java: var7 = var2 == 0 ? var14.stackSize : 1; (line 92)
			var7 = (buttonNum == 0) ? carriedPtr->stackSize : 1;
			
			// Java: if(var7 > var12.getSlotStackLimit()) { var7 = var12.getSlotStackLimit(); } (line 93-95)
			if (var7 > slotStackLimit)
				var7 = slotStackLimit;
			
			// Java: var12.putStack(var14.splitStack(var7)); (line 97)
			// splitStack modifies var14.stackSize and returns new ItemStack with var7 count
			slotStack = ItemStack(carriedPtr->itemID, var7, carriedPtr->itemDamage);
			carriedPtr->stackSize -= var7;
			
			// Java: if(var14.stackSize == 0) { var6.setItemStack((ItemStack)null); } (line 98-100)
			if (carriedPtr->stackSize <= 0)
				minecraft.player->inventory.setCarriedNull();
		}
		return;
	}
	
	// Java: } else if(var14 == null) { (line 102) - slot has item, cursor is empty
	if (carriedPtr == nullptr || carriedPtr->isEmpty())
	{
		// Java: var7 = var2 == 0 ? var13.stackSize : (var13.stackSize + 1) / 2; (line 103)
		var7 = (buttonNum == 0) ? slotStack.stackSize : (slotStack.stackSize + 1) / 2;
		
		// Java: ItemStack var11 = var12.decrStackSize(var7); (line 104)
		// decrStackSize reduces var13.stackSize by var7 and returns new ItemStack with var7 count
		ItemStack pickedUp = slotStack;
		pickedUp.stackSize = var7;
		minecraft.player->inventory.setCarried(std::move(pickedUp));
		
		slotStack.stackSize -= var7;
		
		// Java: if(var13.stackSize == 0) { var12.putStack((ItemStack)null); } (line 106-108)
		if (slotStack.stackSize <= 0)
			slotStack = ItemStack();
		
		// Java: var12.onPickupFromSlot(var6.getItemStack()); (line 110) - we skip this
		return;
	}
	
	// Java: } else if(var12.isItemValid(var14)) { (line 111) - slot has item, cursor has item, item is valid
	// For furnace slots 0 and 1, isItemValid() returns true (Slot.isItemValid returns true)
	// Java: if(var13.itemID != var14.itemID || var13.getHasSubtypes() && var13.getItemDamage() != var14.getItemDamage()) { (line 112)
	// Java: ItemStack.getHasSubtypes() returns Item.itemsList[this.itemID].getHasSubtypes() (ItemStack.java:169-171)
	// Simplified: Check if items match (same ID and damage) - if not, swap
	// In Alpha 1.2.6, most items don't have subtypes, so the hasSubtypes check is usually false
	// The check effectively becomes: swap if different item ID or different damage
	bool itemsMatch = (slotStack.itemID == carriedPtr->itemID && slotStack.itemDamage == carriedPtr->itemDamage);
	if (!itemsMatch)
	{
		// Java: if(var14.stackSize <= var12.getSlotStackLimit()) { (line 113)
		if (carriedPtr->stackSize <= slotStackLimit)
		{
			// Java: var12.putStack(var14); var6.setItemStack(var13); (line 114-115) - swap
			ItemStack temp = slotStack;
			slotStack = *carriedPtr;
			minecraft.player->inventory.setCarried(std::move(temp));
		}
		return;
	}
	
	// Java: } else { (line 117) - same item type, merge
	// Java: var7 = var2 == 0 ? var14.stackSize : 1; (line 118)
	var7 = (buttonNum == 0) ? carriedPtr->stackSize : 1;
	
	// Java: if(var7 > var12.getSlotStackLimit() - var13.stackSize) { var7 = var12.getSlotStackLimit() - var13.stackSize; } (line 119-121)
	if (var7 > slotStackLimit - slotStack.stackSize)
		var7 = slotStackLimit - slotStack.stackSize;
	
	// Java: if(var7 > var14.getMaxStackSize() - var13.stackSize) { var7 = var14.getMaxStackSize() - var13.stackSize; } (line 123-125)
	if (var7 > carriedPtr->getMaxStackSize() - slotStack.stackSize)
		var7 = carriedPtr->getMaxStackSize() - slotStack.stackSize;
	
	// Java: var14.splitStack(var7); (line 127)
	carriedPtr->stackSize -= var7;
	
	// Java: if(var14.stackSize == 0) { var6.setItemStack((ItemStack)null); } (line 128-130)
	if (carriedPtr->stackSize <= 0)
		minecraft.player->inventory.setCarriedNull();
	
	// Java: var13.stackSize += var7; (line 132)
	slotStack.stackSize += var7;
	
	// Java: } else if(var13.itemID == var14.itemID && var14.getMaxStackSize() > 1 && (!var13.getHasSubtypes() || var13.getItemDamage() == var14.getItemDamage())) { (line 134)
	// This case handles reverse merging (taking from slot to cursor), but only if the first merge attempt didn't happen
	// Since we already handled the merge above, this case shouldn't be reached, but we include it for completeness
	// Actually, this case is for when isItemValid() returns false but items match - we handle it above, so we don't need this
}
