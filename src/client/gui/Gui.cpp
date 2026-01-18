#include "client/gui/Gui.h"

#include "client/Lighting.h"
#include "client/gui/ScreenSizeCalculator.h"
#include "client/gui/GuiNewChat.h"
#include "client/gui/GuiMessage.h"
#include "client/gui/Font.h"

#include "client/Minecraft.h"
#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "client/renderer/entity/ItemRenderer.h"
#include "client/gui/Font.h"
#include "world/item/ItemStack.h"
#include "client/renderer/Tesselator.h"
#include "util/Mth.h"

#include "java/Runtime.h"
#include "pc/OpenGL.h"

// Beta: GL_RESCALE_NORMAL constant (GL_EXT_rescale_normal = 32826)
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 32826
#endif

Gui::Gui(Minecraft &minecraft) : minecraft(minecraft)
{
	// Alpha 1.2.6: Initialize chat GUI
	// Java: this.persistantChatGUI = new GuiNewChat(var1);
	chatGUI = std::make_unique<GuiNewChat>(minecraft);
}

void Gui::render(float a, bool inScreen, int_t xm, int_t ym)
{
	ScreenSizeCalculator ssc(minecraft.width, minecraft.height, minecraft.options);
	int_t width = ssc.getWidth();
	int_t height = ssc.getHeight();

	Font &font = *minecraft.font;

	minecraft.gameRenderer.setupGuiScreen();

	glEnable(GL_BLEND);

	// if (minecraft.options.fancyGraphics)
	// 	renderVignette(minecraft.player->getBrightness(a), width, height);

	// TODO: pumpkin

	// TODO: portal

	// Inventory bar
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, minecraft.textures.loadTexture(u"/gui/gui.png"));

	blit(width / 2 - 91, height - 22, 0, 0, 182, 22);
	blit(width / 2 - 91 - 1 + minecraft.player->inventory.currentItem * 20, height - 22 - 1, 0, 22, 24, 22);  // Beta: var11.selected * 20 (Gui.java:62)

	// Cross
	glBindTexture(GL_TEXTURE_2D, minecraft.textures.loadTexture(u"/gui/icons.png"));
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
	blit(width / 2 - 7, height / 2 - 7, 0, 0, 16, 16);
	glDisable(GL_BLEND);

	// Health and armor
	bool flicker = (minecraft.player->invulnerableTime / 3) % 2 == 1;
	if (minecraft.player->invulnerableTime < 10)
		flicker = false;

	int_t nowHealth = minecraft.player->health;
	int_t lastHealth = minecraft.player->lastHealth;

	random.setSeed(tickCount * 312871);

	if (minecraft.gameMode->canHurtPlayer())
	{
		// Alpha 1.2.6: Calculate armor value from equipped armor (GuiIngame.java:72-73)
		// Beta: Use Inventory.getArmorValue() (Inventory.java:320-338)
		int_t armor = minecraft.player->inventory.getArmorValue();

		for (int_t x = 0; x < 10; x++)
		{
			int_t y = height - 32;
			if (armor > 0)
			{
				int_t armorX = width / 2 + 91 - x * 8 - 9;
				if (x * 2 + 1 < armor) blit(armorX, y, 34, 9, 9, 9);
				if (x * 2 + 1 == armor) blit(armorX, y, 25, 9, 9, 9);
				if (x * 2 + 1 > armor) blit(armorX, y, 16, 9, 9, 9);
			}

			int_t healthX = width / 2 - 91 + x * 8;

			if (nowHealth <= 4)
				y += random.nextInt(2);

			blit(healthX, y, 16 + flicker * 9, 0, 9, 9);
			if (flicker)
			{
				if (x * 2 + 1 < lastHealth) blit(healthX, y, 70, 0, 9, 9);
				if (x * 2 + 1 == lastHealth) blit(healthX, y, 79, 0, 9, 9);
			}
			if (x * 2 + 1 < nowHealth) blit(healthX, y, 52, 0, 9, 9);
			if (x * 2 + 1 == nowHealth) blit(healthX, y, 61, 0, 9, 9);
		}
		
		// Beta: Render air supply bubbles when underwater (Gui.java:126-137)
		const Material &waterMat = Material::water;
		if (minecraft.player->isUnderLiquid(waterMat))
		{
			// Beta: Calculate air supply bubbles (Gui.java:127-128)
			int_t airSupply = minecraft.player->airSupply;  // Beta: Entity.airSupply (Entity.java:73)
			int_t bubbleCount1 = (int_t)Mth::ceil((airSupply - 2) * 10.0 / 300.0);
			int_t bubbleCount2 = (int_t)Mth::ceil(airSupply * 10.0 / 300.0) - bubbleCount1;
			
			// Beta: Render air bubbles (Gui.java:130-136)
			for (int_t i = 0; i < bubbleCount1 + bubbleCount2; i++)
			{
				int_t bubbleX = width / 2 - 91 + i * 8;
				int_t bubbleY = height - 32 - 9;
				if (i < bubbleCount1)
				{
					blit(bubbleX, bubbleY, 16, 18, 9, 9);  // Beta: Full bubble (Gui.java:132)
				}
				else
				{
					blit(bubbleX, bubbleY, 25, 18, 9, 9);  // Beta: Empty bubble (Gui.java:134)
				}
			}
		}
	}

	// Beta: Setup for hotbar rendering (Gui.java:140-145)
	glDisable(GL_BLEND);  // Beta: GL11.glDisable(3042) (Gui.java:140)
	glEnable(GL_RESCALE_NORMAL);  // Beta: GL11.glEnable(32826) (Gui.java:141)
	glPushMatrix();  // Beta: GL11.glPushMatrix() (Gui.java:142)
	glRotatef(180.0f, 1.0f, 0.0f, 0.0f);  // Beta: GL11.glRotatef(180.0F, 1.0F, 0.0F, 0.0F) (Gui.java:143)
	Lighting::turnOn();  // Beta: Lighting.turnOn() (Gui.java:144)
	glPopMatrix();  // Beta: GL11.glPopMatrix() (Gui.java:145)
	
	// Beta: Render hotbar slots (Gui.java:147-151)
	for (int_t i = 0; i < 9; i++)
	{
		int_t x = width / 2 - 90 + i * 20 + 2;  // Beta: var29 = var6 / 2 - 90 + var24 * 20 + 2 (Gui.java:148)
		int_t y = height - 16 - 3;  // Beta: var33 = var7 - 16 - 3 (Gui.java:149)
		renderSlot(i, x, y, a);  // Beta: this.renderSlot(var24, var29, var33, var1) (Gui.java:150)
	}
	
	Lighting::turnOff();  // Beta: Lighting.turnOff() (Gui.java:153)
	glDisable(GL_RESCALE_NORMAL);  // Beta: GL11.glDisable(32826) (Gui.java:154)

	// Debug text
	if (minecraft.options.showDebugInfo)
	{
		font.drawShadow(Minecraft::VERSION_STRING + u" (" + minecraft.fpsString + u")", 2, 2, 0xFFFFFF);
		font.drawShadow(minecraft.gatherStats1(), 2, 12, 0xFFFFFF);
		font.drawShadow(minecraft.gatherStats2(), 2, 22, 0xFFFFFF);
		font.drawShadow(minecraft.gatherStats3(), 2, 32, 0xFFFFFF);
		font.drawShadow(minecraft.gatherStats4(), 2, 42, 0xFFFFFF);

		long_t maxMemory = Runtime::getRuntime().maxMemory();
		long_t totalMemory = Runtime::getRuntime().totalMemory();
		long_t freeMemory = Runtime::getRuntime().freeMemory();
		long_t usedMemory = totalMemory - freeMemory;

		jstring str = u"Used memory: " + String::toString(usedMemory * 100 / maxMemory) + u"% (" + String::toString(usedMemory / 1024 / 1024) + u"MB) of " + String::toString(maxMemory / 1024 / 1024) + u"MB";
		drawString(font, str, width - font.width(str) - 2, 2, 0xE0E0E0);
		str = u"Allocated memory: " + String::toString(totalMemory * 100 / maxMemory) + u"% (" + String::toString(totalMemory / 1024 / 1024) + u"MB)";
		drawString(font, str, width - font.width(str) - 2, 12, 0xE0E0E0);

		drawString(font, u"x: " + String::toString(minecraft.player->x), 2, 64, 0xE0E0E0);
		drawString(font, u"y: " + String::toString(minecraft.player->y), 2, 72, 0xE0E0E0);
		drawString(font, u"z: " + String::toString(minecraft.player->z), 2, 80, 0xE0E0E0);
		// drawString(font, u"xRot: " + String::toString(minecraft.player->xRot), 2, 88, 0xE0E0E0);
		// drawString(font, u"yRot: " + String::toString(minecraft.player->yRot), 2, 96, 0xE0E0E0);
		// drawString(font, u"tilt: " + String::toString(minecraft.player->tilt), 2, 104, 0xE0E0E0);
	}
	else
	{
		font.drawShadow(Minecraft::VERSION_STRING, 2, 2, 0xFFFFFF);
	}
	
	// Alpha 1.2.6: Render chat overlay
	// Java: GuiIngame.java lines 198-207
	// GL11.glEnable(GL11.GL_BLEND);
	// GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
	// GL11.glDisable(GL11.GL_ALPHA_TEST);
	// GL11.glPushMatrix();
	// GL11.glTranslatef(0.0F, (float)(var7 - 48), 0.0F);
	// this.persistantChatGUI.func_73762_a(this.updateCounter);
	// GL11.glPopMatrix();
	// GL11.glEnable(GL11.GL_ALPHA_TEST);
	// GL11.glDisable(GL11.GL_BLEND);
	if (chatGUI)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_ALPHA_TEST);
		glPushMatrix();
		glTranslatef(0.0f, static_cast<float>(height - 48), 0.0f);
		chatGUI->render(tickCount);
		glPopMatrix();
		glEnable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
	}
}

void Gui::tick()
{
	if (nowPlayingTime > 0) nowPlayingTime--;

	tickCount++;
	
	// Beta 1.2/newb12: Increment ticks for all messages
	// Java: for (int var1 = 0; var1 < this.guiMessages.size(); var1++) {
	//         this.guiMessages.get(var1).ticks++;
	//       }
	for (auto &message : guiMessages)
	{
		message.ticks++;
	}
}

// Beta 1.2/newb12: addMessage() - adds a chat message
// Java: public void addMessage(String var1)
void Gui::addMessage(const jstring &message)
{
	// newb12: while (this.minecraft.font.width(var1) > 320) {
	//         int var2 = 1;
	//         while (var2 < var1.length() && this.minecraft.font.width(var1.substring(0, var2 + 1)) <= 320) {
	//             var2++;
	//         }
	//         this.addMessage(var1.substring(0, var2));
	//         var1 = var1.substring(var2);
	//       }
	Font &font = *minecraft.font;
	jstring remaining = message;
	
	while (font.width(remaining) > 320)
	{
		int_t splitPos = 1;
		
		// Find the longest substring that fits within 320 pixels
		while (splitPos < remaining.length() && font.width(remaining.substr(0, splitPos + 1)) <= 320)
		{
			splitPos++;
		}
		
		// Recursively add the first part
		addMessage(remaining.substr(0, splitPos));
		
		// Continue with the remainder
		remaining = remaining.substr(splitPos);
	}
	
	// newb12: this.guiMessages.add(0, new GuiMessage(var1));
	guiMessages.insert(guiMessages.begin(), GuiMessage(remaining));
	
	// newb12: while (this.guiMessages.size() > 50) {
	//         this.guiMessages.remove(this.guiMessages.size() - 1);
	//       }
	while (guiMessages.size() > 50)
	{
		guiMessages.pop_back();
	}
}

// Alpha 1.2.6: getChatGUI() - returns the chat GUI
// Java: public GuiNewChat getChatGUI() {
//     return this.persistantChatGUI;
// }
GuiNewChat &Gui::getChatGUI()
{
	return *chatGUI;
}

// Beta: Gui.renderSlot() - renders a single hotbar slot (Gui.java:322-341)
void Gui::renderSlot(int_t slot, int_t x, int_t y, float a)
{
	// Beta: Get item from inventory slot (Gui.java:323)
	ItemStack &stack = minecraft.player->inventory.mainInventory[slot];
	if (!stack.isEmpty())
	{
		// Beta: popTime animation (Gui.java:325-337) - skip for now since ItemStack doesn't have popTime yet
		// float popTime = stack.popTime - a;
		// if (popTime > 0.0f) { ... }
		
		Font &font = *minecraft.font;  // Beta: Use minecraft.font (Gui.java:334)
		
		// Beta: Render item icon (Gui.java:334)
		EntityRenderDispatcher::itemRenderer.renderGuiItem(font, minecraft.textures, stack, x, y);
		
		// Beta: Render item decorations (count text, durability bar) (Gui.java:339)
		EntityRenderDispatcher::itemRenderer.renderGuiItemDecorations(font, minecraft.textures, stack, x, y);
	}
}
