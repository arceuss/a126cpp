#include "client/renderer/entity/ItemRenderer.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/ItemStack.h"
#include "world/level/tile/Tile.h"
#include "client/renderer/TileRenderer.h"
#include "client/renderer/Tesselator.h"
#include "client/renderer/Textures.h"
#include "client/gui/Font.h"
#include "util/Mth.h"
#include "pc/OpenGL.h"
#include "java/String.h"

// Beta: GL_RESCALE_NORMAL constant (GL_EXT_rescale_normal = 32826)
// Use numeric value to match Beta exactly
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 32826
#endif

// Beta: ItemRenderer constructor (ItemRenderer.java:18-21)
ItemRenderer::ItemRenderer(EntityRenderDispatcher &entityRenderDispatcher) : EntityRenderer(entityRenderDispatcher)
{
	shadowRadius = 0.15f;  // Beta: shadowRadius = 0.15F
	shadowStrength = 0.75f;  // Beta: shadowStrength = 0.75F
}

// Beta: ItemRenderer.render() - ported accurately from ItemRenderer.java:23-107
void ItemRenderer::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	EntityItem &itemEntity = static_cast<EntityItem &>(entity);
	ItemStack &item = itemEntity.item;
	
	random.setSeed(187L);  // Beta: this.random.setSeed(187L)
	
	glPushMatrix();  // Beta: GL11.glPushMatrix()
	
	// Beta: bob = Mth.sin((itemEntity.age + a) / 10.0F + itemEntity.bobOffs) * 0.1F + 0.1F
	float bob = Mth::sin((itemEntity.age + a) / 10.0f + itemEntity.bobOffs) * 0.1f + 0.1f;
	// Beta: spin = ((itemEntity.age + a) / 20.0F + itemEntity.bobOffs) * (180.0F / (float)Math.PI)
	float spin = ((itemEntity.age + a) / 20.0f + itemEntity.bobOffs) * (180.0f / Mth::PI);
	
	// Beta: Determine render count based on stack size (ItemRenderer.java:29-40)
	int_t count = 1;
	if (item.stackSize > 1)
		count = 2;
	if (item.stackSize > 5)
		count = 3;
	if (item.stackSize > 20)
		count = 4;
	
	// Beta: GL11.glTranslatef((float)x, (float)y + bob, (float)z)
	glTranslatef((float)x, (float)(y + bob), (float)z);
	// Beta: GL11.glEnable(32826) - GL_RESCALE_NORMAL
	glEnable(GL_RESCALE_NORMAL);
	
	// Beta: if (item.id < 256 && TileRenderer.canRender(Tile.tiles[item.id].getRenderShape()))
	if (item.itemID < 256 && TileRenderer::canRender(Tile::tiles[item.itemID]->getRenderShape()))
	{
		// Beta: Render as block (ItemRenderer.java:45-65)
		glRotatef(spin, 0.0f, 1.0f, 0.0f);  // Beta: GL11.glRotatef(spin, 0.0F, 1.0F, 0.0F)
		bindTexture(u"/terrain.png");  // Beta: this.bindTexture("/terrain.png")
		
		float s = 0.25f;  // Beta: float s = 0.25F
		// Beta: if (!Tile.tiles[item.id].isCubeShaped() && item.id != Tile.stoneSlabHalf.id)
		if (!Tile::tiles[item.itemID]->isCubeShaped() && item.itemID != 0)  // TODO: Check for stoneSlabHalf
			s = 0.5f;  // Beta: s = 0.5F
		
		glScalef(s, s, s);  // Beta: GL11.glScalef(s, s, s)
		
		// Beta: for (int i = 0; i < count; i++) - render multiple items for stacks
		for (int_t i = 0; i < count; i++)
		{
			glPushMatrix();  // Beta: GL11.glPushMatrix()
			if (i > 0)
			{
				// Beta: Random offset for stacked items (ItemRenderer.java:57-60)
				float xo = (random.nextFloat() * 2.0f - 1.0f) * 0.2f / s;
				float yo = (random.nextFloat() * 2.0f - 1.0f) * 0.2f / s;
				float zo = (random.nextFloat() * 2.0f - 1.0f) * 0.2f / s;
				glTranslatef(xo, yo, zo);  // Beta: GL11.glTranslatef(xo, yo, zo)
			}
			
			// Beta: this.tileRenderer.renderTile(Tile.tiles[item.id], item.getAuxValue())
			tileRenderer.renderTile(*Tile::tiles[item.itemID], item.getAuxValue());
			
			glPopMatrix();  // Beta: GL11.glPopMatrix()
		}
	}
	else
	{
		// Beta: Render as item icon (ItemRenderer.java:67-103)
		glScalef(0.5f, 0.5f, 0.5f);  // Beta: GL11.glScalef(0.5F, 0.5F, 0.5F)
		
		int_t icon = item.getIcon();  // Beta: int icon = item.getIcon()
		
		// Beta: Bind texture based on item type (ItemRenderer.java:69-73)
		if (item.itemID < 256)
			bindTexture(u"/terrain.png");  // Beta: this.bindTexture("/terrain.png")
		else
			bindTexture(u"/gui/items.png");  // Beta: this.bindTexture("/gui/items.png")
		
		// Enable alpha testing for transparent item sprites to prevent black backgrounds
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.1f);
		
		Tesselator &t = Tesselator::instance;  // Beta: Tesselator t = Tesselator.instance
		
		// Beta: Calculate UV coordinates (ItemRenderer.java:76-79)
		float u0 = (icon % 16 * 16 + 0) / 256.0f;
		float u1 = (icon % 16 * 16 + 16) / 256.0f;
		float v0 = (icon / 16 * 16 + 0) / 256.0f;
		float v1 = (icon / 16 * 16 + 16) / 256.0f;
		
		float r = 1.0f;  // Beta: float r = 1.0F
		float xo = 0.5f;  // Beta: float xo = 0.5F
		float yo = 0.25f;  // Beta: float yo = 0.25F
		
		// Beta: for (int i = 0; i < count; i++) - render multiple items for stacks
		for (int_t i = 0; i < count; i++)
		{
			glPushMatrix();  // Beta: GL11.glPushMatrix()
			if (i > 0)
			{
				// Beta: Random offset for stacked items (ItemRenderer.java:87-90)
				float _xo = (random.nextFloat() * 2.0f - 1.0f) * 0.3f;
				float _yo = (random.nextFloat() * 2.0f - 1.0f) * 0.3f;
				float _zo = (random.nextFloat() * 2.0f - 1.0f) * 0.3f;
				glTranslatef(_xo, _yo, _zo);  // Beta: GL11.glTranslatef(_xo, _yo, _zo)
			}
			
			// Beta: GL11.glRotatef(180.0F - this.entityRenderDispatcher.playerRotY, 0.0F, 1.0F, 0.0F)
			glRotatef(180.0f - entityRenderDispatcher.playerRotY, 0.0f, 1.0f, 0.0f);
			
			// Beta: Render billboard quad (ItemRenderer.java:94-100)
			t.begin();
			t.normal(0.0f, 1.0f, 0.0f);  // Beta: t.normal(0.0F, 1.0F, 0.0F)
			t.vertexUV(0.0f - xo, 0.0f - yo, 0.0, u0, v1);  // Beta: t.vertexUV(0.0F - xo, 0.0F - yo, 0.0, u0, v1)
			t.vertexUV(r - xo, 0.0f - yo, 0.0, u1, v1);  // Beta: t.vertexUV(r - xo, 0.0F - yo, 0.0, u1, v1)
			t.vertexUV(r - xo, 1.0f - yo, 0.0, u1, v0);  // Beta: t.vertexUV(r - xo, 1.0F - yo, 0.0, u1, v0)
			t.vertexUV(0.0f - xo, 1.0f - yo, 0.0, u0, v0);  // Beta: t.vertexUV(0.0F - xo, 1.0F - yo, 0.0, u0, v0)
			t.end();
			
			glPopMatrix();  // Beta: GL11.glPopMatrix()
		}
	}
	
	// Beta: GL11.glDisable(32826) - GL_RESCALE_NORMAL
	glDisable(GL_RESCALE_NORMAL);
	glPopMatrix();  // Beta: GL11.glPopMatrix()
}

// Beta: ItemRenderer.renderGuiItem() - renders item icon in GUI (ItemRenderer.java:109-139)
void ItemRenderer::renderGuiItem(Font &font, Textures &textures, ItemStack &item, int_t x, int_t y)
{
	if (item.isEmpty())
		return;
	
	// Beta: Render blocks as 3D models (ItemRenderer.java:111-124)
	if (item.itemID < 256 && TileRenderer::canRender(Tile::tiles[item.itemID]->getRenderShape()))
	{
		glEnable(GL_CULL_FACE);  // Beta: GL11.glEnable(2884) - GL_CULL_FACE (enable before rendering blocks)
		
		int_t paint = item.itemID;
		textures.bind(textures.loadTexture(u"/terrain.png"));
		Tile *tile = Tile::tiles[paint];
		
		glPushMatrix();
		glTranslatef((float)(x - 2), (float)(y + 3), 0.0f);
		glScalef(10.0f, 10.0f, 10.0f);
		glTranslatef(1.0f, 0.5f, 8.0f);
		glRotatef(210.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glScalef(1.0f, 1.0f, 1.0f);
		
		tileRenderer.renderTile(*tile, item.getAuxValue());
		
		glPopMatrix();
	}
	// Beta: Render items as 2D icons (ItemRenderer.java:125-135)
	else if (item.getIcon() >= 0)
	{
		glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896) - GL_LIGHTING
		glDisable(GL_DEPTH_TEST);  // Alpha 1.2.6: Disable depth test so 2D sprites render on top of 3D blocks
		
		if (item.itemID < 256)
			textures.bind(textures.loadTexture(u"/terrain.png"));
		else
			textures.bind(textures.loadTexture(u"/gui/items.png"));
		
		blit(x, y, item.getIcon() % 16 * 16, item.getIcon() / 16 * 16, 16, 16);
		
		glEnable(GL_DEPTH_TEST);  // Alpha 1.2.6: Re-enable depth test after rendering 2D sprite
		glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896)
	}
	
	glEnable(GL_CULL_FACE);  // Beta: GL11.glEnable(2884) - GL_CULL_FACE
}

// Beta: ItemRenderer.renderGuiItemDecorations() - renders count text and durability bar (ItemRenderer.java:141-170)
void ItemRenderer::renderGuiItemDecorations(Font &font, Textures &textures, ItemStack &item, int_t x, int_t y)
{
	if (item.isEmpty())
		return;
	
	// Beta: Render stack count (ItemRenderer.java:143-150)
	if (item.stackSize > 1)
	{
		jstring amount = String::toString(item.stackSize);
		glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896)
		glDisable(GL_DEPTH_TEST);  // Beta: GL11.glDisable(2929)
		
		font.drawShadow(amount, x + 19 - 2 - font.width(amount), y + 6 + 3, 0xFFFFFF);
		
		glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896)
		glEnable(GL_DEPTH_TEST);  // Beta: GL11.glEnable(2929)
	}
	
	// Beta: Render durability bar for damaged items (ItemRenderer.java:152-168)
	if (item.isItemDamaged())
	{
		int_t maxDamage = item.getMaxDamage();
		if (maxDamage > 0)
		{
			int_t p = (int_t)std::round(13.0 - item.itemDamage * 13.0 / maxDamage);
			int_t cc = (int_t)std::round(255.0 - item.itemDamage * 255.0 / maxDamage);
			
			glDisable(GL_LIGHTING);  // Beta: GL11.glDisable(2896)
			glDisable(GL_DEPTH_TEST);  // Beta: GL11.glDisable(2929)
			glDisable(GL_TEXTURE_2D);  // Beta: GL11.glDisable(3553)
			
			Tesselator &t = Tesselator::instance;
			int_t ca = (255 - cc) << 16 | cc << 8;
			int_t cb = ((255 - cc) / 4) << 16 | 16128;
			
			fillRect(x + 2, y + 13, 13, 2, 0);
			fillRect(x + 2, y + 13, 12, 1, cb);
			fillRect(x + 2, y + 13, p, 1, ca);
			
			glEnable(GL_TEXTURE_2D);  // Beta: GL11.glEnable(3553)
			glEnable(GL_LIGHTING);  // Beta: GL11.glEnable(2896)
			glEnable(GL_DEPTH_TEST);  // Beta: GL11.glEnable(2929)
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

// Beta: ItemRenderer.fillRect() - helper method for rendering filled rectangle (ItemRenderer.java:172-180)
void ItemRenderer::fillRect(int_t x, int_t y, int_t w, int_t h, int_t color)
{
	Tesselator &t = Tesselator::instance;
	t.begin();
	t.color(color);
	t.vertex((float)(x + 0), (float)(y + 0), 0.0);
	t.vertex((float)(x + 0), (float)(y + h), 0.0);
	t.vertex((float)(x + w), (float)(y + h), 0.0);
	t.vertex((float)(x + w), (float)(y + 0), 0.0);
	t.end();
}

// Beta: ItemRenderer.blit() - renders texture region (ItemRenderer.java:182-193)
void ItemRenderer::blit(int_t x, int_t y, int_t sx, int_t sy, int_t w, int_t h)
{
	const float blitOffset = 0.0f;  // Beta: float blitOffset = 0.0F
	const float us = 0.00390625f;  // Beta: 1/256 = 0.00390625F
	const float vs = 0.00390625f;  // Beta: 1/256 = 0.00390625F
	
	Tesselator &t = Tesselator::instance;
	t.begin();
	t.vertexUV((float)(x + 0), (float)(y + h), blitOffset, (sx + 0) * us, (sy + h) * vs);
	t.vertexUV((float)(x + w), (float)(y + h), blitOffset, (sx + w) * us, (sy + h) * vs);
	t.vertexUV((float)(x + w), (float)(y + 0), blitOffset, (sx + w) * us, (sy + 0) * vs);
	t.vertexUV((float)(x + 0), (float)(y + 0), blitOffset, (sx + 0) * us, (sy + 0) * vs);
	t.end();
}
