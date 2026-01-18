#include "world/level/tile/RedStoneOreTile.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/item/Item.h"
#include "world/entity/Entity.h"
#include "world/entity/player/Player.h"
#include "java/Random.h"

// Beta: RedStoneOreTile(int var1, int var2, boolean var3) (RedStoneOreTile.java:13-20)
RedStoneOreTile::RedStoneOreTile(int_t id, int_t tex, bool lit) : Tile(id, tex, Material::stone), lit(lit)
{
	// Beta: RedStoneOreTile uses Material.stone (RedStoneOreTile.java:14)
	// Beta: if (var3) { this.setTicking(true); } (RedStoneOreTile.java:15-17)
	if (lit)
	{
		setTicking(true);
	}
	// Beta: this.lit = var3 (RedStoneOreTile.java:19)
}

// Beta: RedStoneOreTile.getTickDelay() - returns 30 (RedStoneOreTile.java:22-25)
int_t RedStoneOreTile::getTickDelay()
{
	// Beta: return 30 (RedStoneOreTile.java:24)
	return 30;
}

// Beta: RedStoneOreTile.attack() - triggers glow (RedStoneOreTile.java:27-31)
void RedStoneOreTile::attack(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: this.interact(var1, var2, var3, var4) (RedStoneOreTile.java:29)
	interact(level, x, y, z);
	// Beta: super.attack(var1, var2, var3, var4, var5) (RedStoneOreTile.java:30)
	Tile::attack(level, x, y, z, player);
}

// Beta: RedStoneOreTile.stepOn() - triggers glow (RedStoneOreTile.java:33-37)
void RedStoneOreTile::stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{
	// Beta: this.interact(var1, var2, var3, var4) (RedStoneOreTile.java:35)
	interact(level, x, y, z);
	// Beta: super.stepOn(var1, var2, var3, var4, var5) (RedStoneOreTile.java:36)
	Tile::stepOn(level, x, y, z, entity);
}

// Beta: RedStoneOreTile.use() - triggers glow (RedStoneOreTile.java:39-43)
bool RedStoneOreTile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	// Beta: this.interact(var1, var2, var3, var4) (RedStoneOreTile.java:41)
	interact(level, x, y, z);
	// Beta: return super.use(var1, var2, var3, var4, var5) (RedStoneOreTile.java:42)
	return Tile::use(level, x, y, z, player);
}

// Beta: RedStoneOreTile.interact() - triggers glow (RedStoneOreTile.java:45-50)
void RedStoneOreTile::interact(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: this.poofParticles(var1, var2, var3, var4) (RedStoneOreTile.java:46)
	poofParticles(level, x, y, z);
	// Beta: if (this.id == Tile.redStoneOre.id) (RedStoneOreTile.java:47)
	// Use ID 73 directly to avoid circular dependency
	if (id == 73)
	{
		// Beta: var1.setTile(var2, var3, var4, Tile.redStoneOre_lit.id) (RedStoneOreTile.java:48)
		// Use ID 74 directly
		level.setTile(x, y, z, 74);
	}
}

// Beta: RedStoneOreTile.tick() - turns off lit version after 30 ticks (RedStoneOreTile.java:52-57)
void RedStoneOreTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: if (this.id == Tile.redStoneOre_lit.id) (RedStoneOreTile.java:54)
	// Use ID 74 directly to avoid circular dependency
	if (id == 74)
	{
		// Beta: var1.setTile(var2, var3, var4, Tile.redStoneOre.id) (RedStoneOreTile.java:55)
		// Use ID 73 directly
		level.setTile(x, y, z, 73);
	}
}

// Beta: RedStoneOreTile.getResource() - returns redstone item (RedStoneOreTile.java:59-62)
int_t RedStoneOreTile::getResource(int_t data, Random &random)
{
	// Beta: return Item.redStone.id (RedStoneOreTile.java:61)
	// TODO: Use proper Item::redStone.id when Item system is fully implemented
	// Alpha 1.2.6: redstone item is typically item 65, shiftedIndex = 256 + 65 = 321
	return 256 + 65;  // Placeholder: redstone item shiftedIndex
}

// Beta: RedStoneOreTile.getResourceCount() - returns 4-5 redstone (RedStoneOreTile.java:64-67)
int_t RedStoneOreTile::getResourceCount(Random &random)
{
	// Beta: return 4 + var1.nextInt(2) (RedStoneOreTile.java:66)
	return 4 + random.nextInt(2);
}

// Beta: RedStoneOreTile.animateTick() - adds particles if lit (RedStoneOreTile.java:69-74)
void RedStoneOreTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: if (this.lit) (RedStoneOreTile.java:71)
	if (lit)
	{
		// Beta: this.poofParticles(var1, var2, var3, var4) (RedStoneOreTile.java:72)
		poofParticles(level, x, y, z);
	}
}

// Beta: RedStoneOreTile.poofParticles() - adds reddust particles (RedStoneOreTile.java:76-112)
void RedStoneOreTile::poofParticles(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: Random var5 = var1.random (RedStoneOreTile.java:77)
	Random &rand = level.random;
	// Beta: double var6 = 0.0625 (RedStoneOreTile.java:78)
	double offset = 0.0625;

	// Beta: for (int var8 = 0; var8 < 6; var8++) (RedStoneOreTile.java:80)
	for (int_t face = 0; face < 6; face++)
	{
		// Beta: double var9 = var2 + var5.nextFloat() (RedStoneOreTile.java:81)
		double px = (double)x + rand.nextFloat();
		// Beta: double var11 = var3 + var5.nextFloat() (RedStoneOreTile.java:82)
		double py = (double)y + rand.nextFloat();
		// Beta: double var13 = var4 + var5.nextFloat() (RedStoneOreTile.java:83)
		double pz = (double)z + rand.nextFloat();
		
		// Beta: if (var8 == 0 && !var1.isSolidTile(var2, var3 + 1, var4)) { var11 = var3 + 1 + var6; } (RedStoneOreTile.java:84-86)
		if (face == 0 && !level.isSolidTile(x, y + 1, z))
			py = (double)y + 1 + offset;
		// Beta: if (var8 == 1 && !var1.isSolidTile(var2, var3 - 1, var4)) { var11 = var3 + 0 - var6; } (RedStoneOreTile.java:88-90)
		if (face == 1 && !level.isSolidTile(x, y - 1, z))
			py = (double)y + 0 - offset;
		// Beta: if (var8 == 2 && !var1.isSolidTile(var2, var3, var4 + 1)) { var13 = var4 + 1 + var6; } (RedStoneOreTile.java:92-94)
		if (face == 2 && !level.isSolidTile(x, y, z + 1))
			pz = (double)z + 1 + offset;
		// Beta: if (var8 == 3 && !var1.isSolidTile(var2, var3, var4 - 1)) { var13 = var4 + 0 - var6; } (RedStoneOreTile.java:96-98)
		if (face == 3 && !level.isSolidTile(x, y, z - 1))
			pz = (double)z + 0 - offset;
		// Beta: if (var8 == 4 && !var1.isSolidTile(var2 + 1, var3, var4)) { var9 = var2 + 1 + var6; } (RedStoneOreTile.java:100-102)
		if (face == 4 && !level.isSolidTile(x + 1, y, z))
			px = (double)x + 1 + offset;
		// Beta: if (var8 == 5 && !var1.isSolidTile(var2 - 1, var3, var4)) { var9 = var2 + 0 - var6; } (RedStoneOreTile.java:104-106)
		if (face == 5 && !level.isSolidTile(x - 1, y, z))
			px = (double)x + 0 - offset;

		// Beta: if (var9 < var2 || var9 > var2 + 1 || var11 < 0.0 || var11 > var3 + 1 || var13 < var4 || var13 > var4 + 1) (RedStoneOreTile.java:108)
		if (px < x || px > x + 1 || py < 0.0 || py > y + 1 || pz < z || pz > z + 1)
		{
		// Beta: var1.addParticle("reddust", var9, var11, var13, 0.0, 0.0, 0.0) (RedStoneOreTile.java:109)
		level.addParticle(u"reddust", px, py, pz, 0.0, 0.0, 0.0);
		}
	}
}
