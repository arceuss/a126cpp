#include "client/renderer/entity/PlayerRenderer.h"

#include "world/entity/player/Player.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/ItemStack.h"
#include "world/item/Item.h"
#include "world/item/ItemArmor.h"
#include "world/item/Items.h"
#include "world/level/tile/Tile.h"
#include "client/renderer/TileRenderer.h"
#include "client/renderer/Tesselator.h"
#include "client/gui/Font.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "util/Mth.h"
#include <iostream>
#include <typeinfo>
#include <cmath>

#include "OpenGL.h"

PlayerRenderer::PlayerRenderer(EntityRenderDispatcher &entityRenderDispatcher) : MobRenderer(entityRenderDispatcher, Util::make_shared<HumanoidModel>(0.0f), 0.5f)
{
	humanoidModel = std::static_pointer_cast<HumanoidModel>(model);

	armorParts1 = Util::make_shared<HumanoidModel>(1.0f);
	armorParts2 = Util::make_shared<HumanoidModel>(0.5f);
}

void PlayerRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	Player &mob = reinterpret_cast<Player &>(entity);

	// newb12: Set holdingRightHand and sneaking on all models (PlayerRenderer.java:50-51)
	ItemStack *item = mob.inventory.getSelected();
	bool holdingItem = item != nullptr;
	bool sneaking = mob.isSneaking();
	humanoidModel->holdingRightHand = holdingItem;
	armorParts1->holdingRightHand = holdingItem;
	armorParts2->holdingRightHand = holdingItem;
	humanoidModel->sneaking = sneaking;
	armorParts1->sneaking = sneaking;
	armorParts2->sneaking = sneaking;

	double yp = y - mob.heightOffset;
	if (mob.isSneaking())
		yp -= 0.125;

	MobRenderer::render(mob, x, yp, z, rot, a);
	
	// newb12: Reset after rendering (PlayerRenderer.java:58-59)
	humanoidModel->holdingRightHand = false;
	armorParts1->holdingRightHand = false;
	armorParts2->holdingRightHand = false;
	humanoidModel->sneaking = false;
	armorParts1->sneaking = false;
	armorParts2->sneaking = false;
	
	// Alpha 1.2.6: Render nametag above player (PlayerRenderer.java:60-103)
	// Only render for other players (not local player in first person)
	if (entityRenderDispatcher.player.get() != &mob || entityRenderDispatcher.options->thirdPersonView > 0)
	{
		renderNameTag(mob, x, yp, z, a);
	}
}

void PlayerRenderer::scale(Mob &mob, float a)
{
	float s = 0.9375f;
	glScalef(s, s, s);
}

// Alpha 1.2.6: RenderPlayer.a() - prepares armor for rendering (RenderPlayer.java:15-36)
// newb12: PlayerRenderer.prepareArmor() (PlayerRenderer.java:25-46)
bool PlayerRenderer::prepareArmor(Mob &mob, int_t layer, float a)
{
	Player &player = static_cast<Player &>(mob);
	
	// Alpha: ItemStack var3 = var1.inventory.armorItemInSlot(3 - var2) (RenderPlayer.java:16)
	// newb12: ItemInstance itemInstance = player.inventory.getArmor(3 - layer) (PlayerRenderer.java:26)
	ItemStack *stack = player.inventory.armorItemInSlot(3 - layer);
	if (stack == nullptr || stack->isEmpty())
		return false;
	
	Item *item = stack->getItem();
	if (item == nullptr)
		return false;
	
	// Alpha: if(var4 instanceof ItemArmor) (RenderPlayer.java:19)
	ItemArmor *armorItem = dynamic_cast<ItemArmor *>(item);
	if (armorItem == nullptr)
		return false;
	
	// Alpha: armorFilenamePrefix = {"cloth", "chain", "iron", "diamond", "gold"} (RenderPlayer.java:9)
	// newb12: MATERIAL_NAMES = {"cloth", "chain", "iron", "diamond", "gold"} (PlayerRenderer.java:19)
	static const jstring MATERIAL_NAMES[] = {u"cloth", u"chain", u"iron", u"diamond", u"gold"};
	
	// Alpha: this.loadTexture("/armor/" + armorFilenamePrefix[var5.renderIndex] + "_" + (var2 == 2 ? 2 : 1) + ".png") (RenderPlayer.java:21)
	// newb12: this.bindTexture("/armor/" + MATERIAL_NAMES[armorItem.icon] + "_" + (layer == 2 ? 2 : 1) + ".png") (PlayerRenderer.java:31)
	int_t iconIndex = armorItem->icon;
	if (iconIndex < 0 || iconIndex > 4)
		iconIndex = 0;  // Fallback to cloth
	int_t layerSuffix = (layer == 2) ? 2 : 1;
	jstring layerStr = (layerSuffix == 2) ? u"2" : u"1";
	jstring texturePath = u"/armor/" + MATERIAL_NAMES[iconIndex] + u"_" + layerStr + u".png";
	bindTexture(texturePath);
	
	// Alpha: ModelBiped var6 = var2 == 2 ? this.field_207_h : this.field_208_g (RenderPlayer.java:22)
	// newb12: HumanoidModel armor = layer == 2 ? this.armorParts2 : this.armorParts1 (PlayerRenderer.java:32)
	std::shared_ptr<HumanoidModel> armorModel = (layer == 2) ? armorParts2 : armorParts1;
	
	// Alpha: Set visibility of model parts based on layer (RenderPlayer.java:23-29)
	// newb12: Set visibility of model parts based on layer (PlayerRenderer.java:33-39)
	armorModel->head.visible = (layer == 0);  // Alpha: var6.bipedHead.field_1403_h = var2 == 0 (RenderPlayer.java:23)
	armorModel->hair.visible = (layer == 0);  // Alpha: var6.field_1285_b.field_1403_h = var2 == 0 (RenderPlayer.java:24)
	armorModel->body.visible = (layer == 1 || layer == 2);  // Alpha: var6.field_1284_c.field_1403_h = var2 == 1 || var2 == 2 (RenderPlayer.java:25)
	armorModel->arm0.visible = (layer == 1);  // Alpha: var6.bipedRightArm.field_1403_h = var2 == 1 (RenderPlayer.java:26)
	armorModel->arm1.visible = (layer == 1);  // Alpha: var6.bipedLeftArm.field_1403_h = var2 == 1 (RenderPlayer.java:27)
	armorModel->leg0.visible = (layer == 2 || layer == 3);  // Alpha: var6.bipedRightLeg.field_1403_h = var2 == 2 || var2 == 3 (RenderPlayer.java:28)
	armorModel->leg1.visible = (layer == 2 || layer == 3);  // Alpha: var6.bipedLeftLeg.field_1403_h = var2 == 2 || var2 == 3 (RenderPlayer.java:29)
	
	// Alpha: this.func_4013_a(var6) (RenderPlayer.java:30)
	// newb12: this.setArmor(armor) (PlayerRenderer.java:40)
	setArmor(armorModel);
	
	return true;
}

void PlayerRenderer::renderHand()
{
	humanoidModel->attackTime = 0.0f;
	humanoidModel->setupAnim(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / 16.0f);
	humanoidModel->arm0.render(1.0f / 16.0f);
}

// Beta: PlayerRenderer.additionalRendering() - renders item in hand (PlayerRenderer.java:198-236)
void PlayerRenderer::additionalRendering(Mob &mob, float a)
{
	Player &player = static_cast<Player &>(mob);
	
	// Beta: ItemInstance item = mob.inventory.getSelected() (PlayerRenderer.java:198)
	ItemStack *itemPtr = player.inventory.getSelected();
	if (itemPtr == nullptr)
		return;
	
	ItemStack item = *itemPtr;  // Beta: Copy item so we can modify it for fishing (PlayerRenderer.java:198)
	
	// Beta: Render item in hand (PlayerRenderer.java:200-235)
	glPushMatrix();
	humanoidModel->arm0.translateTo(0.0625f);  // Beta: this.humanoidModel.arm0.translateTo(0.0625F) (PlayerRenderer.java:201)
	glTranslatef(-0.0625f, 0.4375f, 0.0625f);  // Beta: GL11.glTranslatef(-0.0625F, 0.4375F, 0.0625F) (PlayerRenderer.java:202)
	
	// Beta: If fishing, render stick instead of fishing rod (PlayerRenderer.java:203-204)
	if (player.fishing != nullptr && Items::stick != nullptr)
	{
		item = ItemStack(static_cast<int_t>(Items::stick->getShiftedIndex()), 1, 0);
	}
	
	// Beta: Apply transformations based on item type (PlayerRenderer.java:207-232)
	Item *itemObj = item.getItem();
	// Bounds check: itemID must be >= 0 and < 256 to access Tile::tiles array
	if (item.itemID >= 0 && item.itemID < 256 && Tile::tiles[item.itemID] != nullptr && TileRenderer::canRender(Tile::tiles[item.itemID]->getRenderShape()))
	{
		// Beta: Block rendering (PlayerRenderer.java:207-213)
		float s = 0.5f;
		glTranslatef(0.0f, 0.1875f, -0.3125f);
		s *= 0.75f;
		glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
		glScalef(s, -s, s);
	}
	else if (itemObj != nullptr && itemObj->isHandEquipped())
	{
		// Beta: Hand-equipped items (tools/swords) rendering (PlayerRenderer.java:214-225)
		float s = 0.625f;
		if (itemObj->isMirroredArt())
		{
			glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
			glTranslatef(0.0f, -0.125f, 0.0f);
		}
		
		glTranslatef(0.0f, 0.1875f, 0.0f);
		glScalef(s, -s, s);
		glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		// Beta: Default item rendering (PlayerRenderer.java:226-232)
		float s = 0.375f;
		glTranslatef(0.25f, 0.1875f, -0.1875f);
		glScalef(s, s, s);
		glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
		glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
	}
	
	// Beta: Render the item (PlayerRenderer.java:234)
	// Use tileRenderer from MobRenderer (via EntityRenderer) for blocks, or duplicate item rendering logic
	// Bounds check: itemID must be >= 0 and < 256 to access Tile::tiles array
	if (item.itemID >= 0 && item.itemID < 256 && Tile::tiles[item.itemID] != nullptr && TileRenderer::canRender(Tile::tiles[item.itemID]->getRenderShape()))
	{
		// Beta: Render as block
		bindTexture(u"/terrain.png");
		tileRenderer.renderTile(*Tile::tiles[item.itemID], item.getAuxValue());
	}
	else
	{
		// Beta: Render as item icon (simplified version for third-person)
		if (item.itemID < 256)
		{
			bindTexture(u"/terrain.png");
		}
		else
		{
			bindTexture(u"/gui/items.png");
		}
		
		Tesselator &t = Tesselator::instance;
		int_t icon = item.getIcon();
		
		// Beta: Calculate UV coordinates
		float u1 = (icon % 16 * 16 + 0.0f) / 256.0f;
		float u0 = (icon % 16 * 16 + 15.99f) / 256.0f;
		float v0 = (icon / 16 * 16 + 0.0f) / 256.0f;
		float v1 = (icon / 16 * 16 + 15.99f) / 256.0f;
		
		float r = 1.0f;
		float xo = 0.0f;
		float yo = 0.3f;
		glEnable(GL_RESCALE_NORMAL_EXT);
		glTranslatef(-xo, -yo, 0.0f);
		float s = 1.5f;
		glScalef(s, s, s);
		glRotatef(50.0f, 0.0f, 1.0f, 0.0f);
		glRotatef(335.0f, 0.0f, 0.0f, 1.0f);
		glTranslatef(-0.9375f, -0.0625f, 0.0f);
		float dd = 0.0625f;
		
		// Beta: Front face
		t.begin();
		t.normal(0.0f, 0.0f, 1.0f);
		t.vertexUV(0.0, 0.0, 0.0, u0, v1);
		t.vertexUV(r, 0.0, 0.0, u1, v1);
		t.vertexUV(r, 1.0, 0.0, u1, v0);
		t.vertexUV(0.0, 1.0, 0.0, u0, v0);
		t.end();
		
		// Beta: Back face
		t.begin();
		t.normal(0.0f, 0.0f, -1.0f);
		t.vertexUV(0.0, 1.0, 0.0f - dd, u0, v0);
		t.vertexUV(r, 1.0, 0.0f - dd, u1, v0);
		t.vertexUV(r, 0.0, 0.0f - dd, u1, v1);
		t.vertexUV(0.0, 0.0, 0.0f - dd, u0, v1);
		t.end();
		
		// Beta: Left side
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
		
		// Beta: Right side
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
		
		// Beta: Top side
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
		
		// Beta: Bottom side
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
		
		glDisable(GL_RESCALE_NORMAL_EXT);
	}
	
	glPopMatrix();
}

// Beta: PlayerRenderer.render() - renders nametag (PlayerRenderer.java:60-127)
// Beta: Nametag rendering is inline in render() method, not a separate method
// Note: We keep it as a separate method for code organization, but match Java exactly
void PlayerRenderer::renderNameTag(Player &player, double x, double y, double z, float a)
{
	// Beta: Calculate size and scale (PlayerRenderer.java:60-65)
	float size = 1.6f;  // Beta: float size = 1.6F (PlayerRenderer.java:60)
	float s = 0.016666668f * size;  // Beta: float s = 0.016666668F * size (PlayerRenderer.java:61)
	float dist = static_cast<float>(player.distanceTo(*entityRenderDispatcher.player.get()));  // Beta: float dist = mob.distanceTo(this.entityRenderDispatcher.player) (PlayerRenderer.java:62)
	float maxDist = player.isSneaking() ? 32.0f : 64.0f;  // Beta: float maxDist = mob.isSneaking() ? 32 : 64 (PlayerRenderer.java:63)
	
	// Beta: Only render if within max distance (PlayerRenderer.java:64)
	if (dist >= maxDist)
		return;  // Beta: if (dist < maxDist) { ... } (PlayerRenderer.java:64)
	
	// Beta: Scale text based on distance (PlayerRenderer.java:65)
	// Beta: s = (float)(s * (Math.sqrt(dist) / 2.0)); (PlayerRenderer.java:65)
	s = static_cast<float>(s * (std::sqrt(dist) / 2.0));
	
	// Beta: Get font and message (PlayerRenderer.java:66-67)
	Font &font = getFont();  // Beta: Font font = this.getFont() (PlayerRenderer.java:66)
	jstring msg = player.name;  // Beta: String msg = mob.name (PlayerRenderer.java:67)
	
	// Beta: Push matrix and set up transformations (PlayerRenderer.java:68-72)
	glPushMatrix();  // Beta: GL11.glPushMatrix() (PlayerRenderer.java:68)
	glTranslatef(static_cast<float>(x), static_cast<float>(y) + 2.3f, static_cast<float>(z));  // Beta: GL11.glTranslatef((float)x + 0.0F, (float)y + 2.3F, (float)z) (PlayerRenderer.java:68)
	glNormal3f(0.0f, 1.0f, 0.0f);  // Beta: GL11.glNormal3f(0.0F, 1.0F, 0.0F) (PlayerRenderer.java:69)
	glRotatef(-entityRenderDispatcher.playerRotY, 0.0f, 1.0f, 0.0f);  // Beta: GL11.glRotatef(-this.entityRenderDispatcher.playerRotY, 0.0F, 1.0F, 0.0F) (PlayerRenderer.java:70)
	glRotatef(entityRenderDispatcher.playerRotX, 1.0f, 0.0f, 0.0f);  // Beta: GL11.glRotatef(this.entityRenderDispatcher.playerRotX, 1.0F, 0.0F, 0.0F) (PlayerRenderer.java:71)
	glScalef(-s, -s, s);  // Beta: GL11.glScalef(-s, -s, s) (PlayerRenderer.java:72)
	
	// Beta: Disable lighting (PlayerRenderer.java:74)
	glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896) (PlayerRenderer.java:74)
	
	if (!player.isSneaking())  // Beta: if (!mob.isSneaking()) (PlayerRenderer.java:75)
	{
		// Beta: Render nametag when not sneaking (PlayerRenderer.java:76-102)
		glDepthMask(GL_FALSE);  // Beta: GL11.glDepthMask(false) (PlayerRenderer.java:76)
		glDisable(GL_DEPTH_TEST);  // Beta: GL11.glDisable(2929) (PlayerRenderer.java:77)
		glEnable(GL_BLEND);  // Beta: GL11.glEnable(3042) (PlayerRenderer.java:78)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Beta: GL11.glBlendFunc(770, 771) (PlayerRenderer.java:79)
		
		Tesselator &t = Tesselator::instance;  // Beta: Tesselator t = Tesselator.instance (PlayerRenderer.java:80)
		int offs = 0;  // Beta: int offs = 0 (PlayerRenderer.java:81)
		
		// Beta: Special case for "deadmau5" (PlayerRenderer.java:82-84)
		if (msg == u"deadmau5")  // Beta: if (mob.name.equals("deadmau5")) (PlayerRenderer.java:82)
		{
			offs = -10;  // Beta: offs = -10 (PlayerRenderer.java:83)
		}
		
		// Beta: Render shadow background (PlayerRenderer.java:86-94)
		glDisable(GL_TEXTURE_2D);  // Beta: GL11.glDisable(3553) (PlayerRenderer.java:86)
		t.begin();  // Beta: t.begin() (PlayerRenderer.java:87)
		int w = font.width(msg) / 2;  // Beta: int w = font.width(msg) / 2 (PlayerRenderer.java:88)
		t.color(0.0f, 0.0f, 0.0f, 0.25f);  // Beta: t.color(0.0F, 0.0F, 0.0F, 0.25F) (PlayerRenderer.java:89)
		t.vertex(-w - 1, -1 + offs, 0.0);  // Beta: t.vertex(-w - 1, -1 + offs, 0.0) (PlayerRenderer.java:90)
		t.vertex(-w - 1, 8 + offs, 0.0);  // Beta: t.vertex(-w - 1, 8 + offs, 0.0) (PlayerRenderer.java:91)
		t.vertex(w + 1, 8 + offs, 0.0);  // Beta: t.vertex(w + 1, 8 + offs, 0.0) (PlayerRenderer.java:92)
		t.vertex(w + 1, -1 + offs, 0.0);  // Beta: t.vertex(w + 1, -1 + offs, 0.0) (PlayerRenderer.java:93)
		t.end();  // Beta: t.end() (PlayerRenderer.java:94)
		glEnable(GL_TEXTURE_2D);  // Beta: GL11.glEnable(3553) (PlayerRenderer.java:95)
		
		// Beta: Render text shadow (PlayerRenderer.java:96)
		font.draw(msg, -font.width(msg) / 2, offs, 553648127);  // Beta: font.draw(msg, -font.width(msg) / 2, offs, 553648127) (PlayerRenderer.java:96)
		
		// Beta: Re-enable depth test and depth mask (PlayerRenderer.java:97-98)
		glEnable(GL_DEPTH_TEST);  // Beta: GL11.glEnable(2929) (PlayerRenderer.java:97)
		glDepthMask(GL_TRUE);  // Beta: GL11.glDepthMask(true) (PlayerRenderer.java:98)
		
		// Beta: Render text (PlayerRenderer.java:99)
		font.draw(msg, -font.width(msg) / 2, offs, -1);  // Beta: font.draw(msg, -font.width(msg) / 2, offs, -1) (PlayerRenderer.java:99)
		
		// Beta: Re-enable lighting and disable blend (PlayerRenderer.java:100-101)
		glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896) (PlayerRenderer.java:100)
		glDisable(GL_BLEND);  // Beta: GL11.glDisable(3042) (PlayerRenderer.java:101)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Beta: GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F) (PlayerRenderer.java:102)
		
		// Beta: Pop matrix (PlayerRenderer.java:103)
		glPopMatrix();  // Beta: GL11.glPopMatrix() (PlayerRenderer.java:103)
	}
	else  // Beta: else (PlayerRenderer.java:104)
	{
		// Beta: Render nametag when sneaking (PlayerRenderer.java:105-125)
		glTranslatef(0.0f, 0.25f / s, 0.0f);  // Beta: GL11.glTranslatef(0.0F, 0.25F / s, 0.0F) (PlayerRenderer.java:105)
		glDepthMask(GL_FALSE);  // Beta: GL11.glDepthMask(false) (PlayerRenderer.java:106)
		glEnable(GL_BLEND);  // Beta: GL11.glEnable(3042) (PlayerRenderer.java:107)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Beta: GL11.glBlendFunc(770, 771) (PlayerRenderer.java:108)
		
		Tesselator &t = Tesselator::instance;  // Beta: Tesselator t = Tesselator.instance (PlayerRenderer.java:109)
		
		// Beta: Render shadow background when sneaking (PlayerRenderer.java:110-118)
		glDisable(GL_TEXTURE_2D);  // Beta: GL11.glDisable(3553) (PlayerRenderer.java:110)
		t.begin();  // Beta: t.begin() (PlayerRenderer.java:111)
		int w = font.width(msg) / 2;  // Beta: int w = font.width(msg) / 2 (PlayerRenderer.java:112)
		t.color(0.0f, 0.0f, 0.0f, 0.25f);  // Beta: t.color(0.0F, 0.0F, 0.0F, 0.25F) (PlayerRenderer.java:113)
		t.vertex(-w - 1, -1.0, 0.0);  // Beta: t.vertex(-w - 1, -1.0, 0.0) (PlayerRenderer.java:114)
		t.vertex(-w - 1, 8.0, 0.0);  // Beta: t.vertex(-w - 1, 8.0, 0.0) (PlayerRenderer.java:115)
		t.vertex(w + 1, 8.0, 0.0);  // Beta: t.vertex(w + 1, 8.0, 0.0) (PlayerRenderer.java:116)
		t.vertex(w + 1, -1.0, 0.0);  // Beta: t.vertex(w + 1, -1.0, 0.0) (PlayerRenderer.java:117)
		t.end();  // Beta: t.end() (PlayerRenderer.java:118)
		glEnable(GL_TEXTURE_2D);  // Beta: GL11.glEnable(3553) (PlayerRenderer.java:119)
		
		// Beta: Set depth mask to true BEFORE drawing text (PlayerRenderer.java:120)
		glDepthMask(GL_TRUE);  // Beta: GL11.glDepthMask(true) (PlayerRenderer.java:120)
		
		// Beta: Render text shadow only (no white text when sneaking) (PlayerRenderer.java:121)
		font.draw(msg, -font.width(msg) / 2, 0, 553648127);  // Beta: font.draw(msg, -font.width(msg) / 2, 0, 553648127) (PlayerRenderer.java:121)
		
		// Beta: Re-enable lighting and disable blend (PlayerRenderer.java:122-123)
		glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896) (PlayerRenderer.java:122)
		glDisable(GL_BLEND);  // Beta: GL11.glDisable(3042) (PlayerRenderer.java:123)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Beta: GL11.glColor4f(1.0F, 1.0F, 1.0F, 1.0F) (PlayerRenderer.java:124)
		
		// Beta: Pop matrix (PlayerRenderer.java:125)
		glPopMatrix();  // Beta: GL11.glPopMatrix() (PlayerRenderer.java:125)
	}
}
