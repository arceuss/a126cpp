#include "client/gui/ChestScreen.h"

#include <cmath>

#include "client/Minecraft.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/entity/player/Player.h"
#include "world/entity/player/InventoryPlayer.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/tile/entity/InventoryBasic.h"
#include "world/CompoundContainer.h"
#include "client/gamemode/MultiplayerMode.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"
#include "lwjgl/Keyboard.h"

// Beta 1.2 ChestScreen constructor for single chest (ChestScreen.java:10-19)
ChestScreen::ChestScreen(Minecraft &minecraft, std::shared_ptr<ChestTileEntity> chestEntity) 
	: Screen(minecraft), chestEntity(chestEntity), compoundContainer(nullptr), inventoryBasic(nullptr)
{
	this->passEvents = false;  // Beta: this.passEvents = false (ChestScreen.java:14)
	
	// Alpha 1.2.6: Calculate imageHeight based on chest rows (GuiChest.java:15-18)
	// Alpha: short var3 = 222; int var4 = var3 - 108; this.ySize = var4 + this.inventoryRows * 18;
	int_t chestSize = chestEntity->getContainerSize();  // 27 for single chest
	inventoryRows = chestSize / 9;  // Alpha: this.inventoryRows = var2.getSizeInventory() / 9 (GuiChest.java:17)
	int_t var4 = 114;  // Alpha: int var4 = var3 - 108 = 222 - 108 = 114 (GuiChest.java:16)
	imageHeight = var4 + inventoryRows * 18;  // Alpha: this.ySize = var4 + this.inventoryRows * 18 (GuiChest.java:18)
}

// Beta 1.2 ChestScreen constructor for double chest
ChestScreen::ChestScreen(Minecraft &minecraft, std::shared_ptr<CompoundContainer> compoundContainer) 
	: Screen(minecraft), chestEntity(nullptr), compoundContainer(compoundContainer), inventoryBasic(nullptr)
{
	this->passEvents = false;
	
	// Alpha 1.2.6: Calculate imageHeight based on chest rows (GuiChest.java:15-18)
	// Alpha: short var3 = 222; int var4 = var3 - 108; this.ySize = var4 + this.inventoryRows * 18;
	int_t chestSize = compoundContainer->getContainerSize();  // 54 for double chest
	inventoryRows = chestSize / 9;  // 6 rows for double chest
	int_t var4 = 114;  // Alpha: int var4 = var3 - 108 = 222 - 108 = 114 (GuiChest.java:16)
	imageHeight = var4 + inventoryRows * 18;  // Alpha: this.ySize = var4 + this.inventoryRows * 18 (GuiChest.java:18)
}

// Alpha 1.2.6: ChestScreen constructor for multiplayer chest (uses InventoryBasic)
ChestScreen::ChestScreen(Minecraft &minecraft, std::shared_ptr<InventoryBasic> inventory) 
	: Screen(minecraft), chestEntity(nullptr), compoundContainer(nullptr), inventoryBasic(inventory)
{
	this->passEvents = false;
	
	// Alpha 1.2.6: Calculate imageHeight based on chest rows (GuiChest.java:15-18)
	// Alpha: short var3 = 222; int var4 = var3 - 108; this.ySize = var4 + this.inventoryRows * 18;
	int_t chestSize = inventory->getSizeInventory();  // 27 for single chest, 54 for double chest
	inventoryRows = chestSize / 9;  // Alpha: this.inventoryRows = var2.getSizeInventory() / 9 (GuiChest.java:17)
	int_t var4 = 114;  // Alpha: int var4 = var3 - 108 = 222 - 108 = 114 (GuiChest.java:16)
	imageHeight = var4 + inventoryRows * 18;  // Alpha: this.ySize = var4 + this.inventoryRows * 18 (GuiChest.java:18)
}

// Beta: ChestScreen.render() - main render method
void ChestScreen::render(int_t xm, int_t ym, float a)
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
	
	// Beta: Render chest slots (ContainerMenu.java:14-18)
	// Chest slots at 8 + col*18, 18 + row*18
	for (int_t row = 0; row < inventoryRows; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			int_t slotIndex = col + row * 9;
			int_t slotX = 8 + col * 18;
			int_t slotY = 18 + row * 18;
			
			std::shared_ptr<ItemStack> itemPtr = nullptr;
			if (compoundContainer != nullptr)
				itemPtr = compoundContainer->getItem(slotIndex);
			else if (chestEntity != nullptr)
				itemPtr = chestEntity->getItem(slotIndex);
			else if (inventoryBasic != nullptr)
				itemPtr = inventoryBasic->getStackInSlot(slotIndex);
			ItemStack item = (itemPtr != nullptr) ? *itemPtr : ItemStack();
			renderSlot(slotIndex, slotX, slotY, item);
			
			if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
			{
				hoveredSlot = slotIndex;
				hoveredSlotX = slotX;
				hoveredSlotY = slotY;
			}
		}
	}
	
	// Beta: Render player inventory slots (ContainerMenu.java:20-28)
	// Main inventory slots (9-35) in 3 rows
	int_t var4 = (inventoryRows - 4) * 18;
	for (int_t row = 0; row < 3; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			int_t slotIndex = 9 + col + row * 9;
			int_t slotX = 8 + col * 18;
			int_t slotY = 103 + row * 18 + var4;
			
			ItemStack &stack = minecraft.player->inventory.mainInventory[slotIndex];
			renderSlot(slotIndex, slotX, slotY, stack);
			
			if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
			{
				hoveredSlot = slotIndex + 1000;  // Offset to distinguish from chest slots
				hoveredSlotX = slotX;
				hoveredSlotY = slotY;
			}
		}
	}
	
	// Hotbar slots (0-8) at bottom
	for (int_t i = 0; i < 9; ++i)
	{
		int_t slotX = 8 + i * 18;
		int_t slotY = 161 + var4;
		
		ItemStack &stack = minecraft.player->inventory.mainInventory[i];
		renderSlot(i, slotX, slotY, stack);
		
		if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
		{
			hoveredSlot = i + 1000;
			hoveredSlotX = slotX;
			hoveredSlotY = slotY;
		}
	}
	
	// Beta: Highlight hovered slot
	if (hoveredSlot >= 0)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		fillGradient(hoveredSlotX - 1, hoveredSlotY - 1, hoveredSlotX + 17, hoveredSlotY + 17, 0x80FFFFFF, 0x80FFFFFF);
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	}
	
	// Beta: Render carried item
	ItemStack *carried = minecraft.player->inventory.getCarried();
	if (carried != nullptr && !carried->isEmpty())
	{
		int_t mouseX = relX - 8;
		int_t mouseY = relY - 8;
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

// Beta: ChestScreen.renderBg() - renders chest background texture (ChestScreen.java:26-34)
void ChestScreen::renderBg(float a)
{
	int_t tex = minecraft.textures.loadTexture(u"/gui/container.png");
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	minecraft.textures.bind(tex);
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	// Beta: Render chest area (ChestScreen.java:32)
	blit(xo, yo, 0, 0, imageWidth, inventoryRows * 18 + 17);
	// Beta: Render player inventory area (ChestScreen.java:33)
	blit(xo, yo + inventoryRows * 18 + 17, 0, 126, imageWidth, 96);
}

// Alpha 1.2.6: ChestScreen.renderLabels() - renders text labels (GuiChest.java:21-24)
// Alpha: drawGuiContainerForegroundLayer() is called after glPopMatrix, so coordinates are screen-relative
void ChestScreen::renderLabels()
{
	int_t xo = (width - imageWidth) / 2;  // Alpha: int var3 = (this.width - this.xSize) / 2 (GuiChest.java:30)
	int_t yo = (height - imageHeight) / 2;  // Alpha: int var4 = (this.height - this.ySize) / 2 (GuiChest.java:31)
	
	// Alpha: Draw chest name label (GuiChest.java:22)
	// Alpha: Coordinates are relative to GUI area: 8, 6 (GuiChest.java:22)
	// Alpha: this.fontRenderer.drawString(this.lowerChestInventory.getInvName(), 8, 6, 4210752)
	jstring chestName;
	if (compoundContainer != nullptr)
	{
		chestName = compoundContainer->getName();
	}
	else if (chestEntity != nullptr)
	{
		chestName = chestEntity->getName();
	}
	else if (inventoryBasic != nullptr)
	{
		// Alpha 1.2.6: Use server-provided container name for multiplayer
		chestName = inventoryBasic->getInvName();
	}
	else
	{
		chestName = u"Chest";
	}
	font.draw(chestName, xo + 8, yo + 6, 0x00404040);
	
	// Alpha: Draw "Inventory" label (GuiChest.java:23)
	// Alpha: Coordinates are relative to GUI area: 8, this.ySize - 96 + 2 (GuiChest.java:23)
	// Alpha: this.fontRenderer.drawString(this.upperChestInventory.getInvName(), 8, this.ySize - 96 + 2, 4210752)
	// Alpha: ySize = 114 + inventoryRows * 18, so: 8, (114 + inventoryRows * 18) - 96 + 2 = 8, 20 + inventoryRows * 18
	font.draw(u"Inventory", xo + 8, yo + imageHeight - 96 + 2, 0x00404040);
}

// Beta: ChestScreen.renderSlot() - renders a single inventory slot
void ChestScreen::renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack)
{
	if (!itemStack.isEmpty())
	{
		EntityRenderDispatcher::itemRenderer.renderGuiItem(font, minecraft.textures, itemStack, x, y);
		EntityRenderDispatcher::itemRenderer.renderGuiItemDecorations(font, minecraft.textures, itemStack, x, y);
	}
}

// Beta: ChestScreen.keyPressed() - handles key press
void ChestScreen::keyPressed(char_t eventCharacter, int_t eventKey)
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
int_t ChestScreen::getChestSize() const
{
	if (compoundContainer != nullptr)
		return compoundContainer->getContainerSize();
	else if (chestEntity != nullptr)
		return chestEntity->getContainerSize();
	else if (inventoryBasic != nullptr)
		return inventoryBasic->getSizeInventory();
	return 27;  // Default single chest size
}

void ChestScreen::setChestSlot(int_t slot, std::shared_ptr<ItemStack> stack)
{
	// Slot indexing: 0 to (chestSize - 1) for chest slots
	int_t chestSize = getChestSize();
	
	if (slot >= 0 && slot < chestSize)
	{
		if (compoundContainer != nullptr)
		{
			compoundContainer->setItem(slot, stack);
		}
		else if (chestEntity != nullptr)
		{
			chestEntity->setItem(slot, stack);
		}
		else if (inventoryBasic != nullptr)
		{
			inventoryBasic->setInventorySlotContents(slot, stack);
		}
	}
}

// Beta: ChestScreen.mouseClicked() - handles mouse clicks
void ChestScreen::mouseClicked(int_t x, int_t y, int_t buttonNum)
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
	
	// Beta: Check chest slots
	for (int_t row = 0; row < inventoryRows; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			int_t slotX = xo + 8 + col * 18;
			int_t slotY = yo + 18 + row * 18;
			if (x >= slotX - 1 && x < slotX + 17 && y >= slotY - 1 && y < slotY + 17)
			{
				int_t slotIndex = col + row * 9;
				std::shared_ptr<ItemStack> itemPtr = nullptr;
				if (compoundContainer != nullptr)
					itemPtr = compoundContainer->getItem(slotIndex);
				else if (chestEntity != nullptr)
					itemPtr = chestEntity->getItem(slotIndex);
				else if (inventoryBasic != nullptr)
					itemPtr = inventoryBasic->getStackInSlot(slotIndex);
				ItemStack slotStack = (itemPtr != nullptr) ? *itemPtr : ItemStack();
				
				// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
				if (minecraft.isOnline())
				{
					// Capture "before" state BEFORE processing locally (Java's func_27280_a returns what was in slot before click)
					// Java: ItemStack var13 = var12.getStack(); ... var5 = var13.copy(); ... return var5;
					std::unique_ptr<ItemStack> itemBefore = (!slotStack.isEmpty()) ? 
						std::make_unique<ItemStack>(slotStack) : nullptr;
					
					// Process locally first for immediate visual feedback
				handleSlotClick(slotStack, carried, buttonNum, true);
				
				// Update chest entity, compound container, or inventoryBasic
				if (compoundContainer != nullptr)
				{
					if (slotStack.isEmpty())
						compoundContainer->setItem(slotIndex, nullptr);
					else
						compoundContainer->setItem(slotIndex, std::make_shared<ItemStack>(slotStack));
				}
				else if (chestEntity != nullptr)
				{
					if (slotStack.isEmpty())
						chestEntity->setItem(slotIndex, nullptr);
					else
						chestEntity->setItem(slotIndex, std::make_shared<ItemStack>(slotStack));
				}
				else if (inventoryBasic != nullptr)
				{
					if (slotStack.isEmpty())
						inventoryBasic->setInventorySlotContents(slotIndex, nullptr);
					else
						inventoryBasic->setInventorySlotContents(slotIndex, std::make_shared<ItemStack>(slotStack));
					}
					
					// Send packet to server - chest slot maps directly to container slot
					// Java sends what was in the slot BEFORE the click (Container.func_27280_a returns var5 which is var13.copy() from before processing)
					MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
					if (mpMode != nullptr)
					{
						mpMode->handleWindowClick(windowId, slotIndex, buttonNum, false, minecraft.player, std::move(itemBefore));
					}
				}
				else
				{
					// Single-player: process locally
					handleSlotClick(slotStack, carried, buttonNum, true);
					
					// Update chest entity, compound container, or inventoryBasic
					if (compoundContainer != nullptr)
					{
						if (slotStack.isEmpty())
							compoundContainer->setItem(slotIndex, nullptr);
						else
							compoundContainer->setItem(slotIndex, std::make_shared<ItemStack>(slotStack));
					}
					else if (chestEntity != nullptr)
					{
						if (slotStack.isEmpty())
							chestEntity->setItem(slotIndex, nullptr);
						else
							chestEntity->setItem(slotIndex, std::make_shared<ItemStack>(slotStack));
					}
					else if (inventoryBasic != nullptr)
					{
						if (slotStack.isEmpty())
							inventoryBasic->setInventorySlotContents(slotIndex, nullptr);
						else
							inventoryBasic->setInventorySlotContents(slotIndex, std::make_shared<ItemStack>(slotStack));
					}
				}
				
				return;
			}
		}
	}
	
	// Beta: Check player inventory slots
	int_t var4 = (inventoryRows - 4) * 18;
	int_t chestSize = getChestSize();  // Chest container size (27 for single, 54 for double)
	
	// Check hotbar slots (0-8)
	if (y >= yo + 161 + var4 && y < yo + 161 + var4 + 18 && x >= xo + 8 && x < xo + 8 + 9 * 18)
	{
		int_t col = (x - (xo + 8)) / 18;
		if (col >= 0 && col < 9)
		{
			int_t playerSlot = col;  // Player hotbar slot 0-8
			ItemStack &slotStack = minecraft.player->inventory.mainInventory[playerSlot];
			
			// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
			if (minecraft.isOnline())
			{
				// Capture "before" state BEFORE processing locally (Java's func_27280_a returns what was in slot before click)
				std::unique_ptr<ItemStack> itemBefore = (!slotStack.isEmpty()) ? 
					std::make_unique<ItemStack>(slotStack) : nullptr;
				
				// Process locally first for immediate visual feedback
				handleSlotClick(slotStack, carried, buttonNum, false);
				
				// Map player hotbar slot to container slot (ContainerChest: hotbar slots are at chestSize + 27 + playerSlot)
				// For single chest (27 slots): hotbar slots 0-8 map to container slots 54-62
				// For double chest (54 slots): hotbar slots 0-8 map to container slots 81-89
				int_t containerSlot = chestSize + 27 + playerSlot;
				
				// Send packet to server
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
	
	// Check main inventory slots (9-35)
	for (int_t row = 0; row < 3; ++row)
	{
		int_t rowY = yo + 103 + row * 18 + var4;
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
					// Capture "before" state BEFORE processing locally (Java's func_27280_a returns what was in slot before click)
					std::unique_ptr<ItemStack> itemBefore = (!slotStack.isEmpty()) ? 
						std::make_unique<ItemStack>(slotStack) : nullptr;
					
					// Process locally first for immediate visual feedback
					handleSlotClick(slotStack, carried, buttonNum, false);
					
					// Map player main inventory slot to container slot (ContainerChest: main inventory slots are at chestSize + (playerSlot - 9))
					// For single chest (27 slots): player slots 9-35 map to container slots 27-53
					// For double chest (54 slots): player slots 9-35 map to container slots 54-80
					int_t containerSlot = chestSize + (playerSlot - 9);
					
					// Send packet to server
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

// Beta: Helper method to handle slot clicks - same as InventoryScreen
void ChestScreen::handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isChestSlot)
{
	if (slotStack.isEmpty() && carried == nullptr)
		return;
	
	ItemStack *carriedPtr = carried;
	if (carriedPtr == nullptr)
		carriedPtr = minecraft.player->inventory.getCarried();
	
	if (!slotStack.isEmpty() && (carriedPtr == nullptr || carriedPtr->isEmpty()))
	{
		int_t toTake = (buttonNum == 0) ? slotStack.stackSize : (slotStack.stackSize + 1) / 2;
		ItemStack pickedUp = slotStack;
		pickedUp.stackSize = toTake;
		minecraft.player->inventory.setCarried(std::move(pickedUp));
		
		slotStack.stackSize -= toTake;
		if (slotStack.stackSize <= 0)
			slotStack = ItemStack();
		return;
	}
	
	if (slotStack.isEmpty() && carriedPtr != nullptr && !carriedPtr->isEmpty())
	{
		// Java: Container.func_27280_a() line 92-95
		// var7 = var2 == 0 ? var14.stackSize : 1;
		// if(var7 > var12.getSlotStackLimit()) {
		//     var7 = var12.getSlotStackLimit();
		// }
		// Use slot stack limit (64) not item max stack size!
		int_t toPlace = (buttonNum == 0) ? carriedPtr->stackSize : 1;
		int_t slotStackLimit = 64;  // Chest slots have standard inventory limit of 64
		if (toPlace > slotStackLimit)
			toPlace = slotStackLimit;
		
		slotStack = ItemStack(carriedPtr->itemID, toPlace, carriedPtr->itemDamage);
		carriedPtr->stackSize -= toPlace;
		if (carriedPtr->stackSize <= 0)
			minecraft.player->inventory.setCarriedNull();
		return;
	}
	
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
		}
	}
}

void ChestScreen::collectSlotSnapPoints(std::vector<std::pair<int_t, int_t>> &points)
{
	// Collect all slot center positions for snapping (Controlify-style)
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	// Chest slots (inventoryRows rows of 9)
	for (int_t row = 0; row < inventoryRows; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			int_t slotX = xo + 8 + col * 18;
			int_t slotY = yo + 18 + row * 18;
			points.push_back({slotX + 8, slotY + 8}); // Center of 16x16 slot
		}
	}
	
	// Player inventory slots (3 rows of 9, starting at yo + imageHeight - 82)
	int_t playerInvStartY = yo + imageHeight - 82;
	for (int_t row = 0; row < 3; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			int_t slotX = xo + 8 + col * 18;
			int_t slotY = playerInvStartY + row * 18;
			points.push_back({slotX + 8, slotY + 8});
		}
	}
	
	// Hotbar slots (9 slots, starting at yo + imageHeight - 26)
	int_t hotbarStartY = yo + imageHeight - 26;
	for (int_t i = 0; i < 9; ++i)
	{
		int_t slotX = xo + 8 + i * 18;
		int_t slotY = hotbarStartY;
		points.push_back({slotX + 8, slotY + 8});
	}
}
