#include "world/level/tile/FireTile.h"
#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/GasMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/WoodTile.h"
#include "world/level/tile/TreeTile.h"
#include "world/level/tile/LeafTile.h"
#include "world/level/tile/BookshelfTile.h"
#include "world/level/tile/TntTile.h"
#include "world/level/tile/ClothTile.h"
#include "world/level/tile/PortalTile.h"
#include "world/level/tile/ObsidianTile.h"
#include "world/phys/AABB.h"
#include "java/Random.h"

// Beta: Tile.fire = new FireTile(51, 31).setDestroyTime(0.0F).setLightEmission(1.0F).setSoundType(SOUND_WOOD)
// Beta: setLightEmission(1.0F) in Java converts to int: (int)(15.0F * 1.0F) = 15
FireTile::FireTile(int_t id, int_t tex) : Tile(id, tex, Material::fire)
{
	setDestroyTime(0.0f);
	setLightEmission(15);  // Beta: setLightEmission(1.0F) = 15 (full brightness)
	setTicking(true);
	
	// Initialize arrays to 0
	flameOdds.fill(0);
	burnOdds.fill(0);
	
	// Beta: Set flammable blocks (FireTile.java:24-29)
	setFlammable(Tile::wood.id, 5, 20);        // Beta: setFlammable(Tile.wood.id, 5, 20)
	setFlammable(Tile::treeTrunk.id, 5, 5);    // Beta: setFlammable(Tile.treeTrunk.id, 5, 5)
	setFlammable(Tile::leaves.id, 30, 60);     // Beta: setFlammable(Tile.leaves.id, 30, 60)
	setFlammable(Tile::bookshelf.id, 30, 20);  // Beta: setFlammable(Tile.bookshelf.id, 30, 20)
	setFlammable(Tile::tnt.id, 15, 100);       // Beta: setFlammable(Tile.tnt.id, 15, 100)
	setFlammable(Tile::cloth.id, 30, 60);      // Beta: setFlammable(Tile.cloth.id, 30, 60)
}

void FireTile::setFlammable(int_t tileId, int_t flameOddsValue, int_t burnOddsValue)
{
	// Beta: FireTile.setFlammable() (FireTile.java:33-36)
	flameOdds[tileId] = flameOddsValue;
	burnOdds[tileId] = burnOddsValue;
}

AABB *FireTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.getAABB() returns null (no collision) (FireTile.java:39-41)
	return nullptr;
}

bool FireTile::isSolidRender()
{
	// Beta: FireTile.isSolidRender() returns false (FireTile.java:48-50)
	return false;
}

bool FireTile::isCubeShaped()
{
	// Beta: FireTile.isCubeShaped() returns false (FireTile.java:53-55)
	return false;
}

Tile::Shape FireTile::getRenderShape()
{
	// Beta: FireTile.getRenderShape() returns 3 (FireTile.java:58-60)
	return SHAPE_FIRE;
}

int_t FireTile::getResourceCount(Random &random)
{
	// Beta: FireTile.getResourceCount() returns 0 (FireTile.java:63-65)
	return 0;
}

int_t FireTile::getTickDelay()
{
	// Beta: FireTile.getTickDelay() returns 10 (FireTile.java:68-70)
	return 10;
}

void FireTile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: FireTile.tick() - complex fire spread logic (FireTile.java:73-115)
	// Check if on hellRock (netherrack) - fire burns forever on hellRock
	// TODO: Check hellRock ID when it's ported (ID 87 in Beta 1.2, bloodStone in Alpha 1.2.6)
	bool onHellRock = false;  // Beta: var6 = var1.getTile(var2, var3 - 1, var4) == Tile.hellRock.id
	// if (Tile::hellRock exists) onHellRock = level.getTile(x, y - 1, z) == Tile::hellRock.id;
	int_t data = level.getData(x, y, z);  // Beta: var7 = var1.getData(var2, var3, var4)
	
	// Increment fire age
	if (data < 15)
	{
		level.setData(x, y, z, data + 1);  // Beta: var1.setData(var2, var3, var4, var7 + 1)
		// Beta: var1.addToTickNextTick(var2, var3, var4, this.id)
		// TODO: Use scheduleBlockUpdate or addToTickNextTick when available
		level.scheduleBlockUpdate(x, y, z, id);
	}
	
	// Check if fire should extinguish
	if (!onHellRock && !isValidFireLocation(level, x, y, z))
	{
		// Beta: If no solid block below or fire age > 3, extinguish (FireTile.java:82-84)
		if (!level.isSolidTile(x, y - 1, z) || data > 3)
		{
			level.setTile(x, y, z, 0);
		}
	}
	else if (!onHellRock && !canBurn(level, x, y - 1, z) && data == 15 && random.nextInt(4) == 0)
	{
		// Beta: Random chance to extinguish if nothing to burn below (FireTile.java:85-86)
		level.setTile(x, y, z, 0);
	}
	else
	{
		// Beta: Fire spread logic (FireTile.java:88-113)
		if (data % 2 == 0 && data > 2)
		{
			// Check adjacent blocks for burning
			checkBurn(level, x + 1, y, z, 300, random);
			checkBurn(level, x - 1, y, z, 300, random);
			checkBurn(level, x, y - 1, z, 250, random);
			checkBurn(level, x, y + 1, z, 250, random);
			checkBurn(level, x, y, z - 1, 300, random);
			checkBurn(level, x, y, z + 1, 300, random);
			
			// Beta: Check 3x3x5 area for fire spread (FireTile.java:96-112)
			for (int_t dx = x - 1; dx <= x + 1; dx++)
			{
				for (int_t dz = z - 1; dz <= z + 1; dz++)
				{
					for (int_t dy = y - 1; dy <= y + 4; dy++)
					{
						if (dx != x || dy != y || dz != z)
						{
							int_t chance = 100;  // Beta: var11 = 100
							if (dy > y + 1)
							{
								chance += (dy - (y + 1)) * 100;  // Beta: var11 += (var10 - (var3 + 1)) * 100
							}
							
							int_t fireOdds = getFireOdds(level, dx, dy, dz);  // Beta: var12 = this.getFireOdds(var1, var8, var10, var9)
							if (fireOdds > 0 && random.nextInt(chance) <= fireOdds)
							{
								level.setTile(dx, dy, dz, id);  // Beta: var1.setTile(var8, var10, var9, this.id)
							}
						}
					}
				}
			}
		}
	}
}

void FireTile::checkBurn(Level &level, int_t x, int_t y, int_t z, int_t chance, Random &random)
{
	// Beta: FireTile.checkBurn() - checks if block should burn (FireTile.java:117-131)
	int_t tileId = level.getTile(x, y, z);
	if (tileId < 0 || tileId >= 256)
		return;
	
	int_t burnOddsValue = burnOdds[tileId];  // Beta: var7 = this.burnOdds[var1.getTile(var2, var3, var4)]
	if (random.nextInt(chance) < burnOddsValue)
	{
		bool isTnt = tileId == Tile::tnt.id;  // Beta: var8 = var1.getTile(var2, var3, var4) == Tile.tnt.id
		if (random.nextInt(2) == 0)
		{
			level.setTile(x, y, z, id);  // Beta: var1.setTile(var2, var3, var4, this.id)
		}
		else
		{
			level.setTile(x, y, z, 0);  // Beta: var1.setTile(var2, var3, var4, 0)
		}
		
		if (isTnt)
		{
			Tile::tnt.destroy(level, x, y, z, 0);  // Beta: Tile.tnt.destroy(var1, var2, var3, var4, 0)
		}
	}
}

bool FireTile::isValidFireLocation(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.isValidFireLocation() - checks if fire can exist here (FireTile.java:133-145)
	if (canBurn(level, x + 1, y, z))
		return true;
	if (canBurn(level, x - 1, y, z))
		return true;
	if (canBurn(level, x, y - 1, z))
		return true;
	if (canBurn(level, x, y + 1, z))
		return true;
	if (canBurn(level, x, y, z - 1))
		return true;
	return canBurn(level, x, y, z + 1);
}

int_t FireTile::getFireOdds(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.getFireOdds() - calculates fire spread odds (FireTile.java:147-159)
	int_t odds = 0;
	
	// Beta: Check if tile is empty (FireTile.java:149-151)
	if (level.getTile(x, y, z) != 0)
		return 0;
	
	// Beta: Check adjacent blocks for flammability (FireTile.java:152-157)
	odds = getFlammability(level, x + 1, y, z, odds);
	odds = getFlammability(level, x - 1, y, z, odds);
	odds = getFlammability(level, x, y - 1, z, odds);
	odds = getFlammability(level, x, y + 1, z, odds);
	odds = getFlammability(level, x, y, z - 1, odds);
	odds = getFlammability(level, x, y, z + 1, odds);
	
	return odds;
}

bool FireTile::mayPick()
{
	// Beta: FireTile.mayPick() returns false (FireTile.java:162-164)
	return false;
}

bool FireTile::canBurn(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.canBurn() - checks if block can burn (FireTile.java:166-168)
	int_t tileId = level.getTile(x, y, z);
	if (tileId < 0 || tileId >= 256)
		return false;
	return flameOdds[tileId] > 0;
}

bool FireTile::canBlockCatchFire(LevelSource &level, int_t x, int_t y, int_t z)
{
	// Beta: Static method for rendering - same as canBurn() but static
	// RenderBlocks.java uses Tile.fire.canBlockCatchFire() (static call)
	// Beta: canBlockCatchFire checks if flameOdds[tileId] > 0, same as canBurn()
	return Tile::fire.canBurn(level, x, y, z);
}

int_t FireTile::getFlammability(Level &level, int_t x, int_t y, int_t z, int_t current)
{
	// Beta: FireTile.getFlammability() - gets flammability value (FireTile.java:170-173)
	int_t tileId = level.getTile(x, y, z);
	if (tileId < 0 || tileId >= 256)
		return current;
	int_t odds = flameOdds[tileId];  // Beta: var6 = this.flameOdds[var1.getTile(var2, var3, var4)]
	return (odds > current) ? odds : current;  // Beta: return var6 > var5 ? var6 : var5
}

bool FireTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.mayPlace() - checks if fire can be placed (FireTile.java:176-178)
	return level.isSolidTile(x, y - 1, z) || isValidFireLocation(level, x, y, z);
}

void FireTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: FireTile.neighborChanged() - checks if fire can survive (FireTile.java:181-185)
	if (!level.isSolidTile(x, y - 1, z) && !isValidFireLocation(level, x, y, z))
	{
		level.setTile(x, y, z, 0);
	}
}

void FireTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.onPlace() - validates fire placement and creates portal (FireTile.java:188-196)
	// Beta: if (var1.getTile(var2, var3 - 1, var4) != Tile.obsidian.id || !Tile.portalTile.trySpawnPortal(var1, var2, var3, var4)) (FireTile.java:189)
	// If block below is obsidian AND portal can be created, portal is created and fire is not placed
	// Otherwise, do normal fire placement validation
	if (level.getTile(x, y - 1, z) != Tile::obsidian.id || !Tile::portalTile.trySpawnPortal(level, x, y, z))
	{
		// Beta: Normal fire placement validation (FireTile.java:190-194)
		if (!level.isSolidTile(x, y - 1, z) && !isValidFireLocation(level, x, y, z))
		{
			level.setTile(x, y, z, 0);
		}
		else
		{
			// Beta: var1.addToTickNextTick(var2, var3, var4, this.id) (FireTile.java:193)
			level.scheduleBlockUpdate(x, y, z, id);
		}
	}
	// Beta: If portal was created, fire is not placed (the condition above is false, so we return without placing fire)
}

bool FireTile::isFlammable(int_t tileId)
{
	// Beta: FireTile.isFlammable() - checks if block ID is flammable (FireTile.java:198-200)
	if (tileId < 0 || tileId >= 256)
		return false;
	return flameOdds[tileId] > 0;
}

void FireTile::ignite(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.ignite() - tries to ignite nearby blocks (FireTile.java:202-231)
	bool ignited = false;
	
	if (!ignited)
		ignited = tryIgnite(level, x, y + 1, z);
	if (!ignited)
		ignited = tryIgnite(level, x - 1, y, z);
	if (!ignited)
		ignited = tryIgnite(level, x + 1, y, z);
	if (!ignited)
		ignited = tryIgnite(level, x, y, z - 1);
	if (!ignited)
		ignited = tryIgnite(level, x, y, z + 1);
	if (!ignited)
		ignited = tryIgnite(level, x, y - 1, z);
	
	if (!ignited)
	{
		level.setTile(x, y, z, Tile::fire.id);  // Beta: var1.setTile(var2, var3, var4, Tile.fire.id)
	}
}

void FireTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: FireTile.animateTick() - adds smoke particles and sounds (FireTile.java:234-292)
	if (random.nextInt(24) == 0)  // Beta: if (var5.nextInt(24) == 0) (FireTile.java:235)
	{
		// Beta: Play fire sound (FireTile.java:236)
		level.playSound((double)x + 0.5, (double)y + 0.5, (double)z + 0.5, u"fire.fire", 1.0f + random.nextFloat(), random.nextFloat() * 0.7f + 0.3f);
	}
	
	// Beta: Check if block below is solid and can catch fire (FireTile.java:239)
	bool solidBelow = level.isSolidTile(x, y - 1, z);
	bool canCatchFireBelow = canBurn(level, x, y - 1, z);
	
	if (!solidBelow && !canCatchFireBelow)  // Beta: if (!var1.isSolidTile(var2, var3 - 1, var4) && !Tile.fire.canBurn(var1, var2, var3 - 1, var4)) (FireTile.java:239)
	{
		// Beta: Add smoke particles on sides where blocks can catch fire (FireTile.java:240-284)
		if (canBurn(level, x - 1, y, z))  // Beta: if (Tile.fire.canBurn(var1, var2 - 1, var3, var4)) (FireTile.java:240)
		{
			for (int_t i = 0; i < 2; i++)  // Beta: for (int var10 = 0; var10 < 2; var10++) (FireTile.java:241)
			{
				float px = (float)x + random.nextFloat() * 0.1f;  // Beta: float var15 = var2 + var5.nextFloat() * 0.1F (FireTile.java:242)
				float py = (float)y + random.nextFloat();  // Beta: float var20 = var3 + var5.nextFloat() (FireTile.java:243)
				float pz = (float)z + random.nextFloat();  // Beta: float var25 = var4 + var5.nextFloat() (FireTile.java:244)
				level.addParticle(u"largesmoke", (double)px, (double)py, (double)pz, 0.0, 0.0, 0.0);  // Beta: var1.addParticle("largesmoke", var15, var20, var25, 0.0, 0.0, 0.0) (FireTile.java:245)
			}
		}
		
		if (canBurn(level, x + 1, y, z))  // Beta: if (Tile.fire.canBurn(var1, var2 + 1, var3, var4)) (FireTile.java:249)
		{
			for (int_t i = 0; i < 2; i++)  // Beta: for (int var11 = 0; var11 < 2; var11++) (FireTile.java:250)
			{
				float px = (float)x + 1.0f - random.nextFloat() * 0.1f;  // Beta: float var16 = var2 + 1 - var5.nextFloat() * 0.1F (FireTile.java:251)
				float py = (float)y + random.nextFloat();  // Beta: float var21 = var3 + var5.nextFloat() (FireTile.java:252)
				float pz = (float)z + random.nextFloat();  // Beta: float var26 = var4 + var5.nextFloat() (FireTile.java:253)
				level.addParticle(u"largesmoke", (double)px, (double)py, (double)pz, 0.0, 0.0, 0.0);  // Beta: var1.addParticle("largesmoke", var16, var21, var26, 0.0, 0.0, 0.0) (FireTile.java:254)
			}
		}
		
		if (canBurn(level, x, y, z - 1))  // Beta: if (Tile.fire.canBurn(var1, var2, var3, var4 - 1)) (FireTile.java:258)
		{
			for (int_t i = 0; i < 2; i++)  // Beta: for (int var12 = 0; var12 < 2; var12++) (FireTile.java:259)
			{
				float px = (float)x + random.nextFloat();  // Beta: float var17 = var2 + var5.nextFloat() (FireTile.java:260)
				float py = (float)y + random.nextFloat();  // Beta: float var22 = var3 + var5.nextFloat() (FireTile.java:261)
				float pz = (float)z + random.nextFloat() * 0.1f;  // Beta: float var27 = var4 + var5.nextFloat() * 0.1F (FireTile.java:262)
				level.addParticle(u"largesmoke", (double)px, (double)py, (double)pz, 0.0, 0.0, 0.0);  // Beta: var1.addParticle("largesmoke", var17, var22, var27, 0.0, 0.0, 0.0) (FireTile.java:263)
			}
		}
		
		if (canBurn(level, x, y, z + 1))  // Beta: if (Tile.fire.canBurn(var1, var2, var3, var4 + 1)) (FireTile.java:267)
		{
			for (int_t i = 0; i < 2; i++)  // Beta: for (int var13 = 0; var13 < 2; var13++) (FireTile.java:268)
			{
				float px = (float)x + random.nextFloat();  // Beta: float var18 = var2 + var5.nextFloat() (FireTile.java:269)
				float py = (float)y + random.nextFloat();  // Beta: float var23 = var3 + var5.nextFloat() (FireTile.java:270)
				float pz = (float)z + 1.0f - random.nextFloat() * 0.1f;  // Beta: float var28 = var4 + 1 - var5.nextFloat() * 0.1F (FireTile.java:271)
				level.addParticle(u"largesmoke", (double)px, (double)py, (double)pz, 0.0, 0.0, 0.0);  // Beta: var1.addParticle("largesmoke", var18, var23, var28, 0.0, 0.0, 0.0) (FireTile.java:272)
			}
		}
		
		if (canBurn(level, x, y + 1, z))  // Beta: if (Tile.fire.canBurn(var1, var2, var3 + 1, var4)) (FireTile.java:276)
		{
			for (int_t i = 0; i < 2; i++)  // Beta: for (int var14 = 0; var14 < 2; var14++) (FireTile.java:277)
			{
				float px = (float)x + random.nextFloat();  // Beta: float var19 = var2 + var5.nextFloat() (FireTile.java:278)
				float py = (float)y + 1.0f - random.nextFloat() * 0.1f;  // Beta: float var24 = var3 + 1 - var5.nextFloat() * 0.1F (FireTile.java:279)
				float pz = (float)z + random.nextFloat();  // Beta: float var29 = var4 + var5.nextFloat() (FireTile.java:280)
				level.addParticle(u"largesmoke", (double)px, (double)py, (double)pz, 0.0, 0.0, 0.0);  // Beta: var1.addParticle("largesmoke", var19, var24, var29, 0.0, 0.0, 0.0) (FireTile.java:281)
			}
		}
	}
	else  // Beta: else (FireTile.java:284)
	{
		// Beta: Add smoke particles above fire when on solid block (FireTile.java:285-290)
		for (int_t i = 0; i < 3; i++)  // Beta: for (int var6 = 0; var6 < 3; var6++) (FireTile.java:285)
		{
			float px = (float)x + random.nextFloat();  // Beta: float var7 = var2 + var5.nextFloat() (FireTile.java:286)
			float py = (float)y + random.nextFloat() * 0.5f + 0.5f;  // Beta: float var8 = var3 + var5.nextFloat() * 0.5F + 0.5F (FireTile.java:287)
			float pz = (float)z + random.nextFloat();  // Beta: float var9 = var4 + var5.nextFloat() (FireTile.java:288)
			level.addParticle(u"largesmoke", (double)px, (double)py, (double)pz, 0.0, 0.0, 0.0);  // Beta: var1.addParticle("largesmoke", var7, var8, var9, 0.0, 0.0, 0.0) (FireTile.java:289)
		}
	}
}

bool FireTile::tryIgnite(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: FireTile.tryIgnite() - tries to place fire at location (FireTile.java:294-304)
	int_t tileId = level.getTile(x, y, z);
	if (tileId == Tile::fire.id)
	{
		return true;  // Beta: Already fire
	}
	else if (tileId == 0)
	{
		level.setTile(x, y, z, Tile::fire.id);  // Beta: var1.setTile(var2, var3, var4, Tile.fire.id)
		return true;
	}
	return false;
}
