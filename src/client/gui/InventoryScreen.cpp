#include "client/gui/InventoryScreen.h"

#include <cmath>
#include <unordered_map>

#include "client/Minecraft.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/entity/player/Player.h"
#include "world/entity/player/InventoryPlayer.h"
#include "world/item/ItemArmor.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/TreeTile.h"
#include "world/level/tile/WoodTile.h"
#include "network/Packet102WindowClick.h"
#include "network/NetClientHandler.h"
#include "client/gamemode/MultiplayerMode.h"
#include "util/Memory.h"
#include "pc/OpenGL.h"
#include "lwjgl/Keyboard.h"

// Beta 1.2 InventoryScreen constructor (InventoryScreen.java:12-15)
InventoryScreen::InventoryScreen(Minecraft &minecraft) : Screen(minecraft), craftingContainer(craftSlots)
{
	this->passEvents = true;  // Beta: this.passEvents = true
	
	// Initialize crafting slots and result slot
	for (auto &slot : craftSlots)
	{
		slot = ItemStack();
	}
	resultSlot = ItemStack();
}

// Beta: InventoryScreen.render() - main render method (InventoryScreen.java:23-27)
void InventoryScreen::render(int_t xm, int_t ym, float a)
{
	this->xMouse = (float)xm;
	this->yMouse = (float)ym;
	
	// Beta: Render background (AbstractContainerScreen.java:32)
	renderBackground();
	
	int_t xo = (width - imageWidth) / 2;  // Beta: int var4 = (this.width - this.imageWidth) / 2
	int_t yo = (height - imageHeight) / 2;  // Beta: int var5 = (this.height - this.imageHeight) / 2
	
	// Beta: renderBg() - renders inventory background texture (InventoryScreen.java:30-64)
	renderBg(a);
	
	// Beta: Setup lighting and rotation for 3D player model (AbstractContainerScreen.java:36-39)
	glPushMatrix();
	glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
	Lighting::turnOn();
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef((float)xo, (float)yo, 0.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_RESCALE_NORMAL);  // Beta: GL11.glEnable(32826)
	
	// Beta: Find hovered slot based on mouse position (AbstractContainerScreen.java:44-59, 124-130)
	int_t hoveredSlot = -1;
	int_t hoveredSlotX = -1;
	int_t hoveredSlotY = -1;
	
	// Mouse position relative to GUI area (after glTranslatef, so coordinates are relative to xo, yo)
	int_t relX = (int_t)xMouse - xo;
	int_t relY = (int_t)yMouse - yo;
	
	// Beta: Render 2x2 crafting grid (InventoryMenu.java:24-28)
	// Crafting slots at 88 + col*18, 26 + row*18
	for (int_t row = 0; row < 2; ++row)
	{
		for (int_t col = 0; col < 2; ++col)
		{
			int_t slotIndex = col + row * 2;
			int_t slotX = 88 + col * 18;
			int_t slotY = 26 + row * 18;
			renderSlot(-1, slotX, slotY, craftSlots[slotIndex], a);  // Use -1 as slot index for crafting slots
			
			// Beta: Check if hovering crafting slot (AbstractContainerScreen.java:124-130)
			if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
			{
				hoveredSlot = -1;  // Mark as crafting slot
				hoveredSlotX = slotX;
				hoveredSlotY = slotY;
			}
		}
	}
	
	// Beta: Render result slot (InventoryMenu.java:22)
	int_t resultSlotX = 144;
	int_t resultSlotY = 36;
	renderSlot(-2, resultSlotX, resultSlotY, resultSlot, a);  // Use -2 as slot index for result slot
	
	// Beta: Check if hovering result slot
	if (relX >= resultSlotX - 1 && relX < resultSlotX + 17 && relY >= resultSlotY - 1 && relY < resultSlotY + 17)
	{
		hoveredSlot = -2;  // Mark as result slot
		hoveredSlotX = resultSlotX;
		hoveredSlotY = resultSlotY;
	}
	
	// Beta: Render armor slots (InventoryMenu.java:30-46)
	// Armor slots at 8, 8 + var5 * 18 for slots 0-3
	// Container slots 5-8 map to armorInventory[3], armorInventory[2], armorInventory[1], armorInventory[0]
	// Slot 0 = boots (armorInventory[3]), Slot 1 = leggings (armorInventory[2]), Slot 2 = chestplate (armorInventory[1]), Slot 3 = helmet (armorInventory[0])
	for (int_t i = 0; i < 4; ++i)
	{
		int_t armorIndex = 3 - i;  // Map container slot i to armorInventory[3-i]
		int_t slotX = 8;  // Beta: 8 (InventoryMenu.java:32)
		int_t slotY = 8 + i * 18;  // Beta: 8 + var5 * 18 (InventoryMenu.java:32)
		int_t containerSlot = 5 + i;  // Container slots 5-8
		
		ItemStack &armorStack = minecraft.player->inventory.armorInventory[armorIndex];
		renderSlot(containerSlot, slotX, slotY, armorStack, a);
		
		// Beta: Check if hovering armor slot (AbstractContainerScreen.java:124-130)
		if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
		{
			hoveredSlot = containerSlot;  // Use container slot number for armor slots
			hoveredSlotX = slotX;
			hoveredSlotY = slotY;
		}
	}
	
	// Beta: Render inventory slots (Beta 1.2 InventoryMenu.java slot layout)
	// Main inventory slots (9-35) in 3 rows: rows at y=84, 102, 120 (Beta InventoryMenu.java:50-52)
	for (int_t row = 0; row < 3; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			int_t slotIndex = 9 + col + row * 9;  // Beta: var8 + (var6 + 1) * 9
			int_t slotX = 8 + col * 18;  // Beta: 8 + var8 * 18 (relative to xo)
			int_t slotY = 84 + row * 18;  // Beta: 84 + var6 * 18 (relative to yo)
			
			ItemStack &stack = minecraft.player->inventory.mainInventory[slotIndex];
			renderSlot(slotIndex, slotX, slotY, stack, a);
			
			// Beta: Check if hovering (AbstractContainerScreen.java:124-130) - slotX/slotY already relative to xo/yo
			if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)  // Beta: var2 >= var1.x - 1 && var2 < var1.x + 16 + 1 && var3 >= var1.y - 1 && var3 < var1.y + 16 + 1
			{
				hoveredSlot = slotIndex;
				hoveredSlotX = slotX;
				hoveredSlotY = slotY;
			}
		}
	}
	
	// Hotbar slots (0-8) at bottom (Beta InventoryMenu.java:55-57)
	for (int_t i = 0; i < 9; ++i)
	{
		int_t slotX = 8 + i * 18;  // Beta: 8 + var7 * 18 (relative to xo)
		int_t slotY = 142;  // Beta: 142 (relative to yo)
		
		ItemStack &stack = minecraft.player->inventory.mainInventory[i];
		renderSlot(i, slotX, slotY, stack, a);
		
		// Beta: Check if hovering (AbstractContainerScreen.java:124-130)
		if (relX >= slotX - 1 && relX < slotX + 17 && relY >= slotY - 1 && relY < slotY + 17)
		{
			hoveredSlot = i;
			hoveredSlotX = slotX;
			hoveredSlotY = slotY;
		}
	}
	
	// Beta: Highlight hovered slot (AbstractContainerScreen.java:49-58) - inside glPushMatrix so coordinates are relative
	// Only highlight if actually hovering a slot (hoveredSlotX/Y are valid)
	if (hoveredSlotX >= 0 && hoveredSlotY >= 0)
	{
		// Beta: Always highlight hovered slot, regardless of whether it has an item (AbstractContainerScreen.java:49-58)
		glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896)
		glDisable(GL_DEPTH_TEST);  // Beta: GL11.glDisable(2929)
		fillGradient(hoveredSlotX - 1, hoveredSlotY - 1, hoveredSlotX + 17, hoveredSlotY + 17, 0x80FFFFFF, 0x80FFFFFF);  // Beta: -2130706433 = 0x80FFFFFF
		glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896)
		glEnable(GL_DEPTH_TEST);  // Beta: GL11.glEnable(2929)
	}
	
	// Beta: Render carried item (if any) at mouse position (AbstractContainerScreen.java:62-65)
	ItemStack *carried = minecraft.player->inventory.getCarried();
	if (carried != nullptr && !carried->isEmpty())
	{
		// Beta: Translate forward in Z to render on top (AbstractContainerScreen.java:63)
		glTranslatef(0.0f, 0.0f, 32.0f);  // Beta: GL11.glTranslatef(0.0F, 0.0F, 32.0F) (AbstractContainerScreen.java:63)
		// Beta: Render at mouse position (relative to translated matrix: var1 - var4 - 8, var2 - var5 - 8)
		// Since we're inside glTranslatef(xo, yo), coordinates are already relative, so use relX/relY
		int_t mouseX = relX - 8;  // Beta: var1 - var4 - 8 (AbstractContainerScreen.java:64)
		int_t mouseY = relY - 8;  // Beta: var2 - var5 - 8 (AbstractContainerScreen.java:64)
		EntityRenderDispatcher::itemRenderer.renderGuiItem(font, minecraft.textures, *carried, mouseX, mouseY);
		EntityRenderDispatcher::itemRenderer.renderGuiItemDecorations(font, minecraft.textures, *carried, mouseX, mouseY);
	}
	
	glDisable(GL_RESCALE_NORMAL);  // Beta: GL11.glDisable(32826)
	Lighting::turnOff();
	glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896)
	glDisable(GL_DEPTH_TEST);  // Beta: GL11.glDisable(2929)
	renderLabels();  // Beta: this.renderLabels() (AbstractContainerScreen.java:72) - INSIDE glPushMatrix, BEFORE glPopMatrix
	// Alpha: No tooltips in Alpha 1.2.6 (tooltips were added in Beta 1.0)
	glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896) (AbstractContainerScreen.java:84)
	glEnable(GL_DEPTH_TEST);  // Beta: GL11.glEnable(2929) (AbstractContainerScreen.java:85)
	glPopMatrix();  // Beta: GL11.glPopMatrix() (AbstractContainerScreen.java:86) - AFTER re-enabling GL_LIGHTING and GL_DEPTH_TEST
}

// Beta: InventoryScreen.renderBg() - renders inventory background texture (InventoryScreen.java:30-64)
void InventoryScreen::renderBg(float a)
{
	int_t tex = minecraft.textures.loadTexture(u"/gui/inventory.png");
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	minecraft.textures.bind(tex);
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	blit(xo, yo, 0, 0, imageWidth, imageHeight);
	
	// Beta: Render 3D player model (InventoryScreen.java:38-62)
	glEnable(GL_RESCALE_NORMAL);  // Beta: GL11.glEnable(32826)
	glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2903) - GL_LIGHTING
	glPushMatrix();
	glTranslatef((float)(xo + 51), (float)(yo + 75), 50.0f);
	float ss = 30.0f;  // Beta: float var5 = 30.0F
	glScalef(-ss, ss, ss);
	glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
	
	// Beta: Save player rotation (InventoryScreen.java:44-46)
	float oybr = minecraft.player->yBodyRot;  // Beta: float oybr = this.minecraft.player.yBodyRot
	float oyr = minecraft.player->yRot;
	float oxr = minecraft.player->xRot;
	
	float xd = (float)(xo + 51) - xMouse;  // Beta: float var9 = xo + 51 - this.xMouse
	float yd = (float)(yo + 75 - 50) - yMouse;  // Beta: float var10 = yo + 75 - 50 - this.yMouse
	
	glRotatef(135.0f, 0.0f, 1.0f, 0.0f);
	Lighting::turnOn();
	glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
	glRotatef(-((float)std::atan(yd / 40.0f)) * 20.0f, 1.0f, 0.0f, 0.0f);  // Beta: GL11.glRotatef(-((float)Math.atan(yd / 40.0F)) * 20.0F, ...)
	minecraft.player->yBodyRot = (float)std::atan(xd / 40.0f) * 20.0f;  // Beta: this.minecraft.player.yBodyRot = (float)Math.atan(xd / 40.0F) * 20.0F
	minecraft.player->yRot = (float)std::atan(xd / 40.0f) * 40.0f;  // Beta: this.minecraft.player.yRot = (float)Math.atan(xd / 40.0F) * 40.0F
	minecraft.player->xRot = -((float)std::atan(yd / 40.0f)) * 20.0f;  // Beta: this.minecraft.player.xRot = -((float)Math.atan(yd / 40.0F)) * 20.0F
	
	glTranslatef(0.0f, minecraft.player->heightOffset, 0.0f);  // Beta: GL11.glTranslatef(0.0F, this.minecraft.player.heightOffset, 0.0F)
	EntityRenderDispatcher::instance.render(*minecraft.player, 0.0, 0.0, 0.0, 0.0f, 1.0f);  // Beta: EntityRenderDispatcher.instance.render(...)
	
	// Beta: Restore player rotation (InventoryScreen.java:58-60)
	minecraft.player->yBodyRot = oybr;  // Beta: this.minecraft.player.yBodyRot = oybr
	minecraft.player->yRot = oyr;
	minecraft.player->xRot = oxr;
	
	glPopMatrix();
	Lighting::turnOff();
	glDisable(GL_RESCALE_NORMAL);  // Beta: GL11.glDisable(32826)
}

// Beta: InventoryScreen.renderLabels() - renders text labels (InventoryScreen.java:18-20)
void InventoryScreen::renderLabels()
{
	// Beta: Draw "Crafting" label (InventoryScreen.java:19 - uses draw() not drawShadow())
	// Beta: Coordinates are relative to glTranslatef(xo, yo, 0.0F) - still inside glPushMatrix, so use 86, 16 directly
	font.draw(u"Crafting", 86, 16, 0x00404040);  // Beta: this.font.draw("Crafting", 86, 16, 4210752) - converted Java integer to unsigned hex
}

// Beta: InventoryScreen.renderSlot() - renders a single inventory slot
void InventoryScreen::renderSlot(int_t slot, int_t x, int_t y, ItemStack &itemStack, float a)
{
	if (!itemStack.isEmpty())
	{
		// Beta: popTime animation (Gui.java:325-337)
		float popTime = static_cast<float>(itemStack.popTime) - a;  // Beta: float var6 = var5.popTime - var1 (Gui.java:325)
		if (popTime > 0.0f)  // Beta: if (var6 > 0.0F) (Gui.java:326)
		{
			// Beta: Animation scale calculation (Gui.java:327-330)
			float scale = 1.0f + popTime * 0.15f;  // Beta: float var7 = 1.0F + var6 * 0.15F (Gui.java:327)
			glPushMatrix();  // Beta: GL11.glPushMatrix() (Gui.java:328)
			glTranslatef(static_cast<float>(x + 8), static_cast<float>(y + 12), 0.0f);  // Beta: GL11.glTranslatef((float)(var2 + 8), (float)(var3 + 12), 0.0F) (Gui.java:329)
			glScalef(scale, scale, 1.0f);  // Beta: GL11.glScalef(var7, var7, 1.0F) (Gui.java:330)
			glTranslatef(static_cast<float>(-(x + 8)), static_cast<float>(-(y + 12)), 0.0f);  // Beta: GL11.glTranslatef((float)(-(var2 + 8)), (float)(-(var3 + 12)), 0.0F) (Gui.java:331)
		}
		
		// Beta: Render item icon (AbstractContainerScreen.java:109-110)
		EntityRenderDispatcher::itemRenderer.renderGuiItem(font, minecraft.textures, itemStack, x, y);
		EntityRenderDispatcher::itemRenderer.renderGuiItemDecorations(font, minecraft.textures, itemStack, x, y);
		
		// Beta: End popTime animation (Gui.java:337)
		if (popTime > 0.0f)
		{
			glPopMatrix();  // Beta: GL11.glPopMatrix() (Gui.java:337)
		}
	}
	else
	{
		// Alpha 1.2.6: Render armor slot outline icons when empty (SlotArmor.java:21-23)
		// Armor slots are at x=8, y=8+i*18 (i=0..3) = y positions 8, 26, 44, 62
		// Hotbar slots are at y=142, so we can distinguish by position
		// Alpha 1.2.6: SlotArmor.getBackgroundIconIndex() returns 15 + armorType * 16
		// armorType: 0=helmet, 1=chestplate, 2=leggings, 3=boots
		// So: helmet=15, chestplate=31, leggings=47, boots=63
		// Armor slot positions: y=8 (helmet, armorType 0), y=26 (chestplate, armorType 1),
		//                       y=44 (leggings, armorType 2), y=62 (boots, armorType 3)
		// Only render outlines for armor slots: x=8 AND y in [8, 62]
		if (x == 8 && y >= 8 && y <= 62)
		{
			// Alpha 1.2.6: iconIndex = 15 + armorType * 16
			// Calculate armorType from y position: (y - 8) / 18
			// This maps: y=8→0 (helmet), y=26→1 (chestplate), y=44→2 (leggings), y=62→3 (boots)
			int_t armorType = (y - 8) / 18;
			int_t iconIndex = 15 + armorType * 16;
			
			// Beta: Render icon from /gui/items.png (AbstractContainerScreen.java:102-104)
			glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896)
			int_t itemsTex = minecraft.textures.loadTexture(u"/gui/items.png");
			minecraft.textures.bind(itemsTex);
			// Beta: blit(var2, var3, var5 % 16 * 16, var5 / 16 * 16, 16, 16)
			int_t iconX = (iconIndex % 16) * 16;
			int_t iconY = (iconIndex / 16) * 16;
			blit(x, y, iconX, iconY, 16, 16);
			glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896)
		}
	}
}

// Beta: InventoryScreen.keyPressed() - handles key press (AbstractContainerScreen.java:161-167)
void InventoryScreen::keyPressed(char_t eventCharacter, int_t eventKey)
{
	// Beta: Close inventory on ESC or inventory key (I)
	if (eventKey == lwjgl::Keyboard::KEY_ESCAPE || eventKey == minecraft.options.keyInventory.key)
	{
		minecraft.setScreen(nullptr);  // Beta: this.minecraft.player.closeContainer()
		minecraft.grabMouse();
		return;
	}
	
	Screen::keyPressed(eventCharacter, eventKey);
}

// Alpha 1.2.6: Public methods for multiplayer inventory synchronization
void InventoryScreen::setCraftSlot(int_t slot, const ItemStack& stack)
{
	if (slot >= 0 && slot < 4)
	{
		craftSlots[slot] = stack;
		updateCraftingResult();  // Update result when crafting grid changes
	}
}

void InventoryScreen::setResultSlot(const ItemStack& stack)
{
	resultSlot = stack;
}

const ItemStack& InventoryScreen::getCraftSlot(int_t slot) const
{
	if (slot >= 0 && slot < 4)
		return craftSlots[slot];
	static const ItemStack empty;
	return empty;
}

// Beta: InventoryScreen.mouseClicked() - handles mouse clicks for slot interactions (AbstractContainerScreen.java:133-152)
// Beta: Uses Inventory.getCarried()/setCarried() system like Java (AbstractContainerMenu.java:74-184)
void InventoryScreen::mouseClicked(int_t x, int_t y, int_t buttonNum)
{
	if (buttonNum != 0 && buttonNum != 1)  // Beta: Only handle left (0) and right (1) clicks
		return;
	
	int_t xo = (width - imageWidth) / 2;
	int_t yo = (height - imageHeight) / 2;
	
	// Beta: Check if click is outside inventory area (AbstractContainerScreen.java:138)
	bool outside = x < xo || y < yo || x >= xo + imageWidth || y >= yo + imageHeight;
	
	ItemStack *carried = minecraft.player->inventory.getCarried();
	
	// Beta: Click outside = drop carried item (AbstractContainerMenu.java:78-91)
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
	
	// Beta: Check if clicking on crafting grid or result slot first
	// Crafting grid slots (2x2) at 88 + col*18, 26 + row*18
	for (int_t row = 0; row < 2; ++row)
	{
		for (int_t col = 0; col < 2; ++col)
		{
			int_t slotX = xo + 88 + col * 18;
			int_t slotY = yo + 26 + row * 18;
			if (x >= slotX - 1 && x < slotX + 17 && y >= slotY - 1 && y < slotY + 17)
			{
				int_t craftSlotIndex = col + row * 2;
				ItemStack &slotStack = craftSlots[craftSlotIndex];
				
				// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
				if (minecraft.isOnline())
				{
					// ContainerPlayer slot mapping: crafting grid slots 0-3 map to slots 1-4
					int_t containerSlot = 1 + craftSlotIndex;  // Map 0->1, 1->2, 2->3, 3->4
					
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
						mpMode->handleWindowClick(0, containerSlot, buttonNum, false, minecraft.player, std::move(itemBefore));
					}
				}
				else
				{
					// Single-player: process locally
					handleSlotClick(slotStack, carried, buttonNum, true);
					updateCraftingResult();
				}
				return;  // Handled crafting slot click
			}
		}
	}
	
	// Check result slot at 144, 36
	int_t resultX = xo + 144;
	int_t resultY = yo + 36;
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
					// ContainerPlayer slot 0 is the result slot
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
					for (int_t i = 0; i < 4; ++i)
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
						mpMode->handleWindowClick(0, containerSlot, buttonNum, false, minecraft.player, std::move(itemBefore));
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
					for (int_t i = 0; i < 4; ++i)
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
		return;  // Handled result slot click
	}
	
	// Beta: Check armor slots (InventoryMenu.java:30-46)
	// Armor slots at 8, 8 + i * 18 for slots 0-3
	for (int_t i = 0; i < 4; ++i)
	{
		int_t slotX = xo + 8;
		int_t slotY = yo + 8 + i * 18;
		if (x >= slotX - 1 && x < slotX + 17 && y >= slotY - 1 && y < slotY + 17)
		{
			int_t armorIndex = 3 - i;  // Container slot i maps to armorInventory[3-i]
			int_t containerSlot = 5 + i;  // Container slots 5-8
			ItemStack &armorStack = minecraft.player->inventory.armorInventory[armorIndex];
			
			// Beta: Validate armor slot - mayPlace() checks if item matches slot type (InventoryMenu.java:39-44)
			// Slot 0 = boots (armorIndex 3), Slot 1 = leggings (armorIndex 2), Slot 2 = chestplate (armorIndex 1), Slot 3 = helmet (armorIndex 0)
			if (carried != nullptr && !carried->isEmpty())
			{
				Item *item = carried->getItem();
				if (item != nullptr)
				{
					ItemArmor *armorItem = dynamic_cast<ItemArmor *>(item);
					// Beta: Check if armor item slot matches the armor slot index (InventoryMenu.java:40-41)
					// Also allow pumpkin in helmet slot (InventoryMenu.java:42-43)
					bool canPlace = false;
					if (armorItem != nullptr)
					{
						canPlace = (armorItem->slot == i);  // i is the slot type (0=boots, 1=leggings, 2=chestplate, 3=helmet)
					}
					// TODO: Check for pumpkin in helmet slot when pumpkin tile is implemented
					
					if (!canPlace)
					{
						return;  // Cannot place this item in this armor slot
					}
				}
			}
			
			// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
			if (minecraft.isOnline())
			{
				// Capture "before" state before processing locally
				std::unique_ptr<ItemStack> itemBefore = (!armorStack.isEmpty()) ? 
					std::make_unique<ItemStack>(armorStack) : nullptr;
				
				// Process locally first for immediate visual feedback
				handleSlotClick(armorStack, carried, buttonNum, true);  // Armor slots have maxStackSize = 1
				
				// Then send packet to server with "before" state
				MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
				if (mpMode != nullptr)
				{
					mpMode->handleWindowClick(0, containerSlot, buttonNum, false, minecraft.player, std::move(itemBefore));
				}
			}
			else
			{
				// Single-player: process locally
				handleSlotClick(armorStack, carried, buttonNum, true);  // Armor slots have maxStackSize = 1
			}
			return;  // Handled armor slot click
		}
	}
	
	// Beta: Find which inventory slot was clicked (if any)
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
	
	// Beta: Handle inventory slot click (AbstractContainerMenu.java:92-179)
	if (clickedSlot >= 0 && clickedSlot < 36)
	{
		ItemStack &slotStack = minecraft.player->inventory.mainInventory[clickedSlot];
		
		// Alpha 1.2.6: In multiplayer, process locally first (optimistic update), then send packet
		// Java: PlayerControllerMP.func_27174_a() calls super.func_27174_a() to process locally,
		// then sends Packet102WindowClick with the result
		if (minecraft.isOnline())
		{
			// ContainerPlayer slot mapping:
			// Hotbar slots 0-8 map to slots 36-44
			// Main inventory slots 9-35 map to slots 9-35
			int_t containerSlot;
			if (clickedSlot < 9)
			{
				containerSlot = 36 + clickedSlot;  // Hotbar: 0-8 -> 36-44
			}
			else
			{
				containerSlot = clickedSlot;  // Main inventory: 9-35 -> 9-35
			}
			
			// Capture "before" state BEFORE processing locally (Java's func_27280_a returns what was in slot before click)
			// Java: ItemStack var13 = var12.getStack(); ... var5 = var13.copy(); ... return var5;
			std::unique_ptr<ItemStack> itemBefore = (!slotStack.isEmpty()) ? 
				std::make_unique<ItemStack>(slotStack) : nullptr;
			
			// Process locally first for immediate visual feedback (optimistic update)
			handleSlotClick(slotStack, carried, buttonNum, false);
			
			// Then send packet to server with "before" state (server will validate and correct if needed)
			MultiplayerMode* mpMode = dynamic_cast<MultiplayerMode*>(minecraft.gameMode.get());
			if (mpMode != nullptr)
			{
				mpMode->handleWindowClick(0, containerSlot, buttonNum, false, minecraft.player, std::move(itemBefore));
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

// Beta: Helper method to handle slot clicks - matches AbstractContainerMenu.clicked() logic (AbstractContainerMenu.java:92-179)
void InventoryScreen::handleSlotClick(ItemStack &slotStack, ItemStack *carried, int_t buttonNum, bool isCraftingSlot)
{
	// Beta: AbstractContainerMenu.clicked() logic (AbstractContainerMenu.java:101-178)
	if (slotStack.isEmpty() && carried == nullptr)
		return;  // Both empty - nothing to do
	
	ItemStack *carriedPtr = carried;
	if (carriedPtr == nullptr)
		carriedPtr = minecraft.player->inventory.getCarried();
	
	// Beta: Slot has item, nothing carried - pick up (AbstractContainerMenu.java:102-109)
	if (!slotStack.isEmpty() && (carriedPtr == nullptr || carriedPtr->isEmpty()))
	{
		int_t toTake = (buttonNum == 0) ? slotStack.stackSize : (slotStack.stackSize + 1) / 2;  // Beta: var13 = var2 == 0 ? var7.count : (var7.count + 1) / 2
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
	
	// Beta: Slot empty, something carried - place item (AbstractContainerMenu.java:110-119)
	if (slotStack.isEmpty() && carriedPtr != nullptr && !carriedPtr->isEmpty())
	{
		int_t toPlace = (buttonNum == 0) ? carriedPtr->stackSize : 1;  // Beta: var12 = var2 == 0 ? var5.getCarried().count : 1
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
	
	// Beta: Both slot and carried have items - swap or merge (AbstractContainerMenu.java:120-177)
	if (!slotStack.isEmpty() && carriedPtr != nullptr && !carriedPtr->isEmpty())
	{
		// Beta: Different item types - swap if carried fits (AbstractContainerMenu.java:121-126)
		// Beta: Check if items can stack - same ID and same damage (or not damage-sensitive)
		// Simplified: if IDs match and damage matches, they can merge; otherwise swap
		bool canMerge = (slotStack.itemID == carriedPtr->itemID && 
		                 slotStack.itemDamage == carriedPtr->itemDamage &&
		                 slotStack.isStackable() && carriedPtr->isStackable());
		
		if (!canMerge)
		{
			// Beta: Swap items if carried fits in slot (AbstractContainerMenu.java:123-126)
			// Alpha 1.2.6: Always allow swapping when items are different types, regardless of stack sizes
			// The stack size check is only needed for placing, not for swapping different item types
				ItemStack temp = slotStack;
				slotStack = *carriedPtr;
				minecraft.player->inventory.setCarried(std::move(temp));
				if (isCraftingSlot)
					updateCraftingResult();
			return;
		}
		
		// Beta: Same item type - merge stacks (AbstractContainerMenu.java:127-161)
		if (canMerge)
		{
			if (buttonNum == 0)  // Left click - merge all possible
			{
				int_t toAdd = carriedPtr->stackSize;
				int_t maxStack = slotStack.getMaxStackSize();
				if (toAdd > maxStack - slotStack.stackSize)
					toAdd = maxStack - slotStack.stackSize;
				if (toAdd > carriedPtr->getMaxStackSize() - slotStack.stackSize)
					toAdd = carriedPtr->getMaxStackSize() - slotStack.stackSize;
				
				carriedPtr->stackSize -= toAdd;
				if (carriedPtr->stackSize <= 0)
					minecraft.player->inventory.setCarriedNull();
				slotStack.stackSize += toAdd;
			}
			else if (buttonNum == 1)  // Right click - merge one
			{
				int_t toAdd = 1;
				int_t maxStack = slotStack.getMaxStackSize();
				if (toAdd > maxStack - slotStack.stackSize)
					toAdd = maxStack - slotStack.stackSize;
				if (toAdd > carriedPtr->getMaxStackSize() - slotStack.stackSize)
					toAdd = carriedPtr->getMaxStackSize() - slotStack.stackSize;
				
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

// Beta: Update crafting result when grid changes (InventoryMenu.java:62-64)
void InventoryScreen::updateCraftingResult()
{
	// Beta: Use Recipes system to find matching recipe (InventoryMenu.java:63)
	resultSlot = Recipes::getInstance().getItemFor(craftingContainer);
}