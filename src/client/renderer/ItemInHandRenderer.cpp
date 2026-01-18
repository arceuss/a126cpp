#include "client/renderer/ItemInHandRenderer.h"

#include "client/Minecraft.h"
#include "client/Lighting.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "client/renderer/entity/PlayerRenderer.h"
#include "client/renderer/Tesselator.h"
#include "world/item/ItemStack.h"
#include "world/item/Item.h"
#include "world/item/Items.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FireTile.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"

#include "util/Mth.h"
#include "OpenGL.h"

ItemInHandRenderer::ItemInHandRenderer(Minecraft &mc) : mc(mc)
{

}

// Beta: renderItem() - renders item in hand (ItemInHandRenderer.java:28-130)
void ItemInHandRenderer::renderItem(ItemStack &item)
{
	glPushMatrix();
	
	// Beta: if (item.id < 256 && TileRenderer.canRender(Tile.tiles[item.id].getRenderShape()))
	bool renderedAsBlock = false;
	if (!item.isEmpty() && item.itemID < 256)
	{
		Tile *tile = Tile::tiles[item.itemID];
		if (tile != nullptr && TileRenderer::canRender(tile->getRenderShape()))
		{
			// Beta: Render as block
			glBindTexture(GL_TEXTURE_2D, mc.textures.loadTexture(u"/terrain.png"));
			tileRenderer.renderTile(*tile, item.getAuxValue());
			renderedAsBlock = true;
		}
	}
	
	if (!renderedAsBlock && !item.isEmpty())
	{
		// Beta: Render as item icon
		if (item.itemID < 256)
		{
			glBindTexture(GL_TEXTURE_2D, mc.textures.loadTexture(u"/terrain.png"));
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, mc.textures.loadTexture(u"/gui/items.png"));
		}
		
		Tesselator &t = Tesselator::instance;
		int_t icon = item.getIcon();
		
		// Beta: Calculate UV coordinates (ItemInHandRenderer.java:41-44)
		float u1 = (icon % 16 * 16 + 0.0f) / 256.0f;
		float u0 = (icon % 16 * 16 + 15.99f) / 256.0f;
		float v0 = (icon / 16 * 16 + 0.0f) / 256.0f;
		float v1 = (icon / 16 * 16 + 15.99f) / 256.0f;
		
		float r = 1.0f;
		float xo = 0.0f;
		float yo = 0.3f;
		glEnable(GL_RESCALE_NORMAL_EXT);  // Beta: GL11.glEnable(32826)
		glTranslatef(-xo, -yo, 0.0f);
		float s = 1.5f;
		glScalef(s, s, s);
		glRotatef(50.0f, 0.0f, 1.0f, 0.0f);
		glRotatef(335.0f, 0.0f, 0.0f, 1.0f);
		glTranslatef(-0.9375f, -0.0625f, 0.0f);
		float dd = 0.0625f;
		
		// Beta: Front face (ItemInHandRenderer.java:56-62)
		t.begin();
		t.normal(0.0f, 0.0f, 1.0f);
		t.vertexUV(0.0, 0.0, 0.0, u0, v1);
		t.vertexUV(r, 0.0, 0.0, u1, v1);
		t.vertexUV(r, 1.0, 0.0, u1, v0);
		t.vertexUV(0.0, 1.0, 0.0, u0, v0);
		t.end();
		
		// Beta: Back face (ItemInHandRenderer.java:63-69)
		t.begin();
		t.normal(0.0f, 0.0f, -1.0f);
		t.vertexUV(0.0, 1.0, 0.0f - dd, u0, v0);
		t.vertexUV(r, 1.0, 0.0f - dd, u1, v0);
		t.vertexUV(r, 0.0, 0.0f - dd, u1, v1);
		t.vertexUV(0.0, 0.0, 0.0f - dd, u0, v1);
		t.end();
		
		// Beta: Left side (ItemInHandRenderer.java:70-83)
		t.begin();
		t.normal(-1.0f, 0.0f, 0.0f);
		for (int_t i = 0; i < 16; i++)
		{
			float p = i / 16.0f;
			float uu = u0 + (u1 - u0) * p - 0.001953125f;
			float xx = r * p;
			t.vertexUV(xx, 0.0, 0.0f - dd, uu, v1);
			t.vertexUV(xx, 0.0, 0.0, uu, v1);
			t.vertexUV(xx, 1.0, 0.0, uu, v0);
			t.vertexUV(xx, 1.0, 0.0f - dd, uu, v0);
		}
		t.end();
		
		// Beta: Right side (ItemInHandRenderer.java:84-97)
		t.begin();
		t.normal(1.0f, 0.0f, 0.0f);
		for (int_t i = 0; i < 16; i++)
		{
			float p = i / 16.0f;
			float uu = u0 + (u1 - u0) * p - 0.001953125f;
			float xx = r * p + 0.0625f;
			t.vertexUV(xx, 1.0, 0.0f - dd, uu, v0);
			t.vertexUV(xx, 1.0, 0.0, uu, v0);
			t.vertexUV(xx, 0.0, 0.0, uu, v1);
			t.vertexUV(xx, 0.0, 0.0f - dd, uu, v1);
		}
		t.end();
		
		// Beta: Top side (ItemInHandRenderer.java:98-111)
		t.begin();
		t.normal(0.0f, 1.0f, 0.0f);
		for (int_t i = 0; i < 16; i++)
		{
			float p = i / 16.0f;
			float vv = v1 + (v0 - v1) * p - 0.001953125f;
			float yy = r * p + 0.0625f;
			t.vertexUV(0.0, yy, 0.0, u0, vv);
			t.vertexUV(r, yy, 0.0, u1, vv);
			t.vertexUV(r, yy, 0.0f - dd, u1, vv);
			t.vertexUV(0.0, yy, 0.0f - dd, u0, vv);
		}
		t.end();
		
		// Beta: Bottom side (ItemInHandRenderer.java:112-125)
		t.begin();
		t.normal(0.0f, -1.0f, 0.0f);
		for (int_t i = 0; i < 16; i++)
		{
			float p = i / 16.0f;
			float vv = v1 + (v0 - v1) * p - 0.001953125f;
			float yy = r * p;
			t.vertexUV(r, yy, 0.0, u1, vv);
			t.vertexUV(0.0, yy, 0.0, u0, vv);
			t.vertexUV(0.0, yy, 0.0f - dd, u0, vv);
			t.vertexUV(r, yy, 0.0f - dd, u1, vv);
		}
		t.end();
		
		glDisable(GL_RESCALE_NORMAL_EXT);  // Beta: GL11.glDisable(32826)
	}
	
	glPopMatrix();
}

void ItemInHandRenderer::render(float a)
{
	float h = oHeight + (height - oHeight) * a;
	auto &localPlayer = *mc.player;

	glPushMatrix();
	glRotatef(localPlayer.xRotO + (localPlayer.xRot - localPlayer.xRotO) * a, 1.0f, 0.0f, 0.0f);
	glRotatef(localPlayer.yRotO + (localPlayer.yRot - localPlayer.yRotO) * a, 0.0f, 1.0f, 0.0f);
	Lighting::turnOn();
	glPopMatrix();

	float br = mc.level->getBrightness(Mth::floor(localPlayer.x), Mth::floor(localPlayer.y), Mth::floor(localPlayer.z));
	glColor4f(br, br, br, 1.0f);
	
	ItemStack item = selectedItem;  // Beta: ItemInstance item = this.selectedItem (ItemInHandRenderer.java:142)
	// Beta: If fishing, render stick instead of fishing rod (ItemInHandRenderer.java:143-144)
	if (localPlayer.fishing != nullptr && Items::stick != nullptr)
	{
		item = ItemStack(static_cast<int_t>(Items::stick->getShiftedIndex()), 1, 0);
	}
	
	// Beta: Only render if item is not empty (Java ItemInstance is null when empty, but we store as value so check isEmpty)
	if (!item.isEmpty())
	{
		// Beta: Render item in hand (ItemInHandRenderer.java:147-170)
		glPushMatrix();
		float d = 0.8f;
		float swing = localPlayer.getAttackAnim(a);
		float swing1 = Mth::sin(swing * Mth::PI);
		float swing2 = Mth::sin(Mth::sqrt(swing) * Mth::PI);
		glTranslatef(-swing2 * 0.4f, Mth::sin(Mth::sqrt(swing) * Mth::PI * 2.0f) * 0.2f, -swing1 * 0.2f);
		glTranslatef(0.7f * d, -0.65f * d - (1.0f - h) * 0.6f, -0.9f * d);
		glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
		glEnable(GL_RESCALE_NORMAL_EXT);
		swing = localPlayer.getAttackAnim(a);
		swing1 = Mth::sin(swing * swing * Mth::PI);
		swing2 = Mth::sin(Mth::sqrt(swing) * Mth::PI);
		glRotatef(-swing1 * 20.0f, 0.0f, 1.0f, 0.0f);
		glRotatef(-swing2 * 20.0f, 0.0f, 0.0f, 1.0f);
		glRotatef(-swing2 * 80.0f, 1.0f, 0.0f, 0.0f);
		float scale = 0.4f;
		glScalef(scale, scale, scale);
		// Beta: If item is mirrored art (like fishing rod), rotate 180 degrees (ItemInHandRenderer.java:165-167)
		Item *itemObj = item.getItem();
		if (itemObj != nullptr && itemObj->isMirroredArt())
		{
			glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
		}
		renderItem(item);
		glDisable(GL_RESCALE_NORMAL_EXT);
		glPopMatrix();
	}
	else
	{
		// Beta: Render hand when no item (ItemInHandRenderer.java:171-199)
		glPushMatrix();
		float d = 0.8f;

		float swing = localPlayer.getAttackAnim(a);

		float swing1 = Mth::sin(swing * Mth::PI);
		float swing2 = Mth::sin(Mth::sqrt(swing) * Mth::PI);
		glTranslatef(-swing2 * 0.3f, Mth::sin(Mth::sqrt(swing) * Mth::PI * 2.0f) * 0.4f, -swing1 * 0.4f);

		glTranslatef(0.8f * d, -0.75f * d - (1.0f - h) * 0.6f, -0.9f * d);

		glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
		glEnable(GL_RESCALE_NORMAL_EXT);

		swing = localPlayer.getAttackAnim(a);
		float swing3 = Mth::sin(swing * swing * Mth::PI);
		swing2 = Mth::sin(Mth::sqrt(swing) * Mth::PI);
		glRotatef(swing2 * 70.0f, 0.0f, 1.0f, 0.0f);
		glRotatef(-swing3 * 20.0f, 0.0f, 0.0f, 1.0f);

		jstring backupTexture = mc.player->getTexture();  // Beta: backup texture = getTexture() (ItemInHandRenderer.java:186)
		glBindTexture(GL_TEXTURE_2D, mc.textures.loadHttpTexture(mc.player->customTextureUrl, &backupTexture));  // Beta: loadHttpTexture(customTextureUrl, getTexture()) (ItemInHandRenderer.java:186)
		glTranslatef(-1.0f, 3.6f, 3.5f);
		glRotatef(120.0f, 0.0f, 0.0f, 1.0f);
		glRotatef(200.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
		glScalef(1.0f, 1.0f, 1.0f);
		glTranslatef(5.6f, 0.0f, 0.0f);

		auto &playerRenderer = EntityRenderDispatcher::playerRenderer;
		float ss = 1.0f;
		glScalef(ss, ss, ss);
		playerRenderer.renderHand();
		glPopMatrix();
	}

	glDisable(GL_RESCALE_NORMAL_EXT);
	Lighting::turnOff();
}

void ItemInHandRenderer::renderScreenEffect(float a)
{
	glDisable(GL_ALPHA_TEST);

	if (mc.player->isOnFire())
	{
		int_t id = mc.textures.loadTexture(u"/terrain.png");
		glBindTexture(GL_TEXTURE_2D, id);
		renderFire(a);
	}

	if (mc.player->isInWall())
	{
		int_t x = Mth::floor(mc.player->x);
		int_t y = Mth::floor(mc.player->y);
		int_t z = Mth::floor(mc.player->z);

		int_t id = mc.textures.loadTexture(u"/terrain.png");
		glBindTexture(GL_TEXTURE_2D, id);
		int_t tile = mc.level->getTile(x, y, z);
		if (Tile::tiles[tile] != nullptr)
			renderTex(a, Tile::tiles[tile]->getTexture(Facing::NORTH));
	}

	// Beta: Render underwater overlay (ItemInHandRenderer.java:225-229)
	const Material &waterMat = Material::water;
	if (mc.player->isUnderLiquid(waterMat))
	{
		int_t id = mc.textures.loadTexture(u"/misc/water.png");
		glBindTexture(GL_TEXTURE_2D, id);
		renderWater(a);
	}

	glEnable(GL_ALPHA_TEST);
}

// Beta: renderTex() - renders texture overlay when player is inside a block (ItemInHandRenderer.java:234-258)
void ItemInHandRenderer::renderTex(float a, int_t tex)
{
	Tesselator &t = Tesselator::instance;
	float br = mc.player->getBrightness(a);  // Beta: float br = this.mc.player.getBrightness(a) (ItemInHandRenderer.java:236)
	br = 0.1f;  // Beta: br = 0.1F - override brightness for darkness effect (ItemInHandRenderer.java:237)
	glColor4f(br, br, br, 0.5f);  // Beta: GL11.glColor4f(br, br, br, 0.5F) (ItemInHandRenderer.java:238)
	glPushMatrix();  // Beta: GL11.glPushMatrix() (ItemInHandRenderer.java:239)
	
	float x0 = -1.0f;  // Beta: float x0 = -1.0F (ItemInHandRenderer.java:240)
	float x1 = 1.0f;   // Beta: float x1 = 1.0F (ItemInHandRenderer.java:241)
	float y0 = -1.0f;  // Beta: float y0 = -1.0F (ItemInHandRenderer.java:242)
	float y1 = 1.0f;   // Beta: float y1 = 1.0F (ItemInHandRenderer.java:243)
	float z0 = -0.5f;  // Beta: float z0 = -0.5F (ItemInHandRenderer.java:244)
	float r = 0.0078125f;  // Beta: float r = 0.0078125F (ItemInHandRenderer.java:245)
	
	// Beta: Calculate texture UV coordinates with small offset (ItemInHandRenderer.java:246-249)
	float u0 = (tex % 16) / 256.0f - r;  // Beta: float u0 = tex % 16 / 256.0F - r (ItemInHandRenderer.java:246)
	float u1 = ((tex % 16) + 15.99f) / 256.0f + r;  // Beta: float u1 = (tex % 16 + 15.99F) / 256.0F + r (ItemInHandRenderer.java:247)
	float v0 = (tex / 16) / 256.0f - r;  // Beta: float v0 = tex / 16 / 256.0F - r (ItemInHandRenderer.java:248)
	float v1 = ((tex / 16) + 15.99f) / 256.0f + r;  // Beta: float v1 = (tex / 16 + 15.99F) / 256.0F + r (ItemInHandRenderer.java:249)
	
	t.begin();
	t.vertexUV(x0, y0, z0, u1, v1);  // Beta: t.vertexUV(x0, y0, z0, u1, v1) (ItemInHandRenderer.java:251)
	t.vertexUV(x1, y0, z0, u0, v1);  // Beta: t.vertexUV(x1, y0, z0, u0, v1) (ItemInHandRenderer.java:252)
	t.vertexUV(x1, y1, z0, u0, v0);  // Beta: t.vertexUV(x1, y1, z0, u0, v0) (ItemInHandRenderer.java:253)
	t.vertexUV(x0, y1, z0, u1, v0);  // Beta: t.vertexUV(x0, y1, z0, u1, v0) (ItemInHandRenderer.java:254)
	t.end();
	
	glPopMatrix();  // Beta: GL11.glPopMatrix() (ItemInHandRenderer.java:256)
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Beta: GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F) (ItemInHandRenderer.java:257)
}

// Beta: renderWater() - renders underwater overlay (ItemInHandRenderer.java:260-284)
void ItemInHandRenderer::renderWater(float a)
{
	Tesselator &t = Tesselator::instance;
	float br = mc.player->getBrightness(a);  // Beta: float br = this.mc.player.getBrightness(a) (ItemInHandRenderer.java:262)
	glColor4f(br, br, br, 0.5f);  // Beta: GL11.glColor4f(br, br, br, 0.5F) (ItemInHandRenderer.java:263)
	glEnable(GL_BLEND);  // Beta: GL11.glEnable(3042) (ItemInHandRenderer.java:264)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Beta: GL11.glBlendFunc(770, 771) (ItemInHandRenderer.java:265)
	glPushMatrix();  // Beta: GL11.glPushMatrix() (ItemInHandRenderer.java:266)
	
	float size = 4.0f;  // Beta: float size = 4.0F (ItemInHandRenderer.java:267)
	float x0 = -1.0f;  // Beta: float x0 = -1.0F (ItemInHandRenderer.java:268)
	float x1 = 1.0f;   // Beta: float x1 = 1.0F (ItemInHandRenderer.java:269)
	float y0 = -1.0f;  // Beta: float y0 = -1.0F (ItemInHandRenderer.java:270)
	float y1 = 1.0f;   // Beta: float y1 = 1.0F (ItemInHandRenderer.java:271)
	float z0 = -0.5f;  // Beta: float z0 = -0.5F (ItemInHandRenderer.java:272)
	
	float uo = -mc.player->yRot / 64.0f;  // Beta: float uo = -this.mc.player.yRot / 64.0F (ItemInHandRenderer.java:273)
	float vo = mc.player->xRot / 64.0f;  // Beta: float vo = this.mc.player.xRot / 64.0F (ItemInHandRenderer.java:274)
	
	t.begin();
	t.vertexUV(x0, y0, z0, size + uo, size + vo);  // Beta: t.vertexUV(x0, y0, z0, size + uo, size + vo) (ItemInHandRenderer.java:276)
	t.vertexUV(x1, y0, z0, 0.0f + uo, size + vo);  // Beta: t.vertexUV(x1, y0, z0, 0.0F + uo, size + vo) (ItemInHandRenderer.java:277)
	t.vertexUV(x1, y1, z0, 0.0f + uo, 0.0f + vo);  // Beta: t.vertexUV(x1, y1, z0, 0.0F + uo, 0.0F + vo) (ItemInHandRenderer.java:278)
	t.vertexUV(x0, y1, z0, size + uo, 0.0f + vo);  // Beta: t.vertexUV(x0, y1, z0, size + uo, 0.0F + vo) (ItemInHandRenderer.java:279)
	t.end();
	
	glPopMatrix();  // Beta: GL11.glPopMatrix() (ItemInHandRenderer.java:281)
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Beta: GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F) (ItemInHandRenderer.java:282)
	glDisable(GL_BLEND);  // Beta: GL11.glDisable(3042) (ItemInHandRenderer.java:283)
}

// Beta: renderFire() - renders fire overlay when player is burning (ItemInHandRenderer.java:286-320)
void ItemInHandRenderer::renderFire(float a)
{
	Tesselator &t = Tesselator::instance;
	glColor4f(1.0f, 1.0f, 1.0f, 0.9f);  // Beta: GL11.glColor4f(1.0F, 1.0F, 1.0F, 0.9F) (line 288)
	glEnable(GL_BLEND);  // Beta: GL11.glEnable(3042) (line 289)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Beta: GL11.glBlendFunc(770, 771) (line 290)
	float size = 1.0f;  // Beta: float size = 1.0F (line 291)
	
	// Beta: Render two fire textures (ItemInHandRenderer.java:293-316)
	for (int_t i = 0; i < 2; i++)
	{
		glPushMatrix();  // Beta: GL11.glPushMatrix() (line 294)
		int_t tex = Tile::fire.tex + i * 16;  // Beta: int tex = Tile.fire.tex + i * 16 (line 295)
		int_t xt = (tex & 15) << 4;  // Beta: int xt = (tex & 15) << 4 (line 296)
		int_t yt = tex & 240;  // Beta: int yt = tex & 240 (line 297)
		float u0 = (float)xt / 256.0f;  // Beta: float u0 = xt / 256.0F (line 298)
		float u1 = (float)(xt + 15.99f) / 256.0f;  // Beta: float u1 = (xt + 15.99F) / 256.0F (line 299)
		float v0 = (float)yt / 256.0f;  // Beta: float v0 = yt / 256.0F (line 300)
		float v1 = (float)(yt + 15.99f) / 256.0f;  // Beta: float v1 = (yt + 15.99F) / 256.0F (line 301)
		float x0 = (0.0f - size) / 2.0f;  // Beta: float x0 = (0.0F - size) / 2.0F (line 302)
		float x1 = x0 + size;  // Beta: float x1 = x0 + size (line 303)
		float y0 = 0.0f - size / 2.0f;  // Beta: float y0 = 0.0F - size / 2.0F (line 304)
		float y1 = y0 + size;  // Beta: float y1 = y0 + size (line 305)
		float z0 = -0.5f;  // Beta: float z0 = -0.5F (line 306)
		glTranslatef(-(float)(i * 2 - 1) * 0.24f, -0.3f, 0.0f);  // Beta: GL11.glTranslatef(-(i * 2 - 1) * 0.24F, -0.3F, 0.0F) (line 307)
		glRotatef((float)(i * 2 - 1) * 10.0f, 0.0f, 1.0f, 0.0f);  // Beta: GL11.glRotatef((i * 2 - 1) * 10.0F, 0.0F, 1.0F, 0.0F) (line 308)
		t.begin();
		t.vertexUV(x0, y0, z0, u1, v1);  // Beta: t.vertexUV(x0, y0, z0, u1, v1) (line 310)
		t.vertexUV(x1, y0, z0, u0, v1);  // Beta: t.vertexUV(x1, y0, z0, u0, v1) (line 311)
		t.vertexUV(x1, y1, z0, u0, v0);  // Beta: t.vertexUV(x1, y1, z0, u0, v0) (line 312)
		t.vertexUV(x0, y1, z0, u1, v0);  // Beta: t.vertexUV(x0, y1, z0, u1, v0) (line 313)
		t.end();
		glPopMatrix();  // Beta: GL11.glPopMatrix() (line 315)
	}
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Beta: GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F) (line 318)
	glDisable(GL_BLEND);  // Beta: GL11.glDisable(3042) (line 319)
}

// Beta: tick() - tracks selected item and updates height (ItemInHandRenderer.java:322-352)
void ItemInHandRenderer::tick()
{
	oHeight = height;

	auto &localPlayer = *mc.player;
	ItemStack *selectedPtr = localPlayer.inventory.getSelected();  // Beta: ItemInstance selected = player.inventory.getSelected() (ItemInHandRenderer.java:325)
	
	// Beta: Check if slot/item matches (ItemInHandRenderer.java:326)
	// In Java, selectedItem and selected are ItemInstance objects, and == compares references
	// We store selectedItem as a value, so we compare by checking if both are empty or both have the same itemID
	bool selectedEmpty = (selectedPtr == nullptr);
	bool selectedItemEmpty = selectedItem.isEmpty();
	bool matches = lastSlot == localPlayer.inventory.currentItem;
	
	// Beta: Compare items - in Java this compares references, we compare by value
	if (selectedEmpty && selectedItemEmpty)
	{
		matches = matches && true;  // Both empty - match
	}
	else if (!selectedEmpty && !selectedItemEmpty)
	{
		matches = matches && (selectedPtr->itemID == selectedItem.itemID);  // Same item ID - match
	}
	else
	{
		matches = false;  // One empty, one not - no match
	}
	
	// Beta: If both are null/empty, matches = true (ItemInHandRenderer.java:327-329)
	if (selectedItemEmpty && selectedEmpty)
	{
		matches = true;
	}
	
	// Beta: If same item ID but different instances, update selectedItem (ItemInHandRenderer.java:331-334)
	if (!selectedEmpty && !selectedItemEmpty && selectedPtr->itemID == selectedItem.itemID)
	{
		selectedItem = *selectedPtr;  // Copy the value
		matches = true;
	}
	
	// Beta: Calculate height animation (ItemInHandRenderer.java:336-347)
	float max = 0.4f;
	float tHeight = matches ? 1.0f : 0.0f;
	float dd = tHeight - height;
	if (dd < -max)
	{
		dd = -max;
	}
	if (dd > max)
	{
		dd = max;
	}
	
	height += dd;
	
	// Beta: Update selectedItem and lastSlot when height < 0.1F (ItemInHandRenderer.java:348-351)
	if (height < 0.1f)
	{
		if (selectedPtr != nullptr)
			selectedItem = *selectedPtr;  // Copy the value
		else
			selectedItem = ItemStack();  // Empty stack
		lastSlot = localPlayer.inventory.currentItem;
	}
}

void ItemInHandRenderer::itemPlaced()
{
	height = 0.0f;
}

void ItemInHandRenderer::itemUsed()
{
	height = 0.0f;
}
