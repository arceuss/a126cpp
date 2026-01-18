#include "world/level/tile/FluidTile.h"
#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/entity/Entity.h"
#include "world/phys/Vec3.h"
#include "world/level/material/GasMaterial.h"
#include "util/Mth.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Beta 1.2: LiquidTile.java - ported 1:1
FluidTile::FluidTile(int_t id, const LiquidMaterial &material)
	: Tile(id, (&material == &Material::lava ? (14 * 16 + 13) : (12 * 16 + 13)), material), fluidMaterial(material)
{
	// Beta: LiquidTile constructor (LiquidTile.java:12-18)
	// Beta: super(var1, (var2 == Material.lava ? 14 : 12) * 16 + 13, var2)
	float var3 = 0.0F;
	float var4 = 0.0F;
	setShape(0.0F + var4, 0.0F + var3, 0.0F + var4, 1.0F + var4, 1.0F + var3, 1.0F + var4);
	setTicking(true);
	
	// Beta: fluids are non-solid, translucent
	solid[id] = false;
	// Alpha/Beta: water blocks 3 light levels, lava blocks 255 (opaque) (Block.java:30-33, Tile.java:81-86)
	lightBlock[id] = (&material == &Material::water) ? 3 : 255;
	translucent[id] = true;
}

// Beta: LiquidTile.getHeight() - static helper (LiquidTile.java:20-26)
float FluidTile::getHeight(int_t metadata)
{
	if (metadata >= 8)
	{
		metadata = 0;
	}
	return static_cast<float>(metadata + 1) / 9.0F;
}

// Beta: LiquidTile.getTexture() - side textures (LiquidTile.java:28-31)
int_t FluidTile::getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	return (face != Facing::DOWN && face != Facing::UP) ? (tex + 1) : tex;
}

// Beta: LiquidTile.isCubeShaped() returns false (LiquidTile.java:50-53)
bool FluidTile::isCubeShaped()
{
	return false;
}

// Beta: LiquidTile.isSolidRender() returns false (LiquidTile.java:55-58)
bool FluidTile::isSolidRender() const
{
	return false;
}

// Beta: LiquidTile.mayPick() - only with bucket when level 0 (LiquidTile.java:60-63)
bool FluidTile::mayPick(int_t data, bool withTool)
{
	return withTool && data == 0;
}

// Beta: LiquidTile.shouldRenderFace() - hides faces between same fluid (LiquidTile.java:65-75)
bool FluidTile::shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: x, y, z are already the neighbor's position (passed from TileRenderer)
	// Beta 1.2: LiquidTile.shouldRenderFace() checks the neighbor material directly
	const Material &neighborMat = level.getMaterial(x, y, z);
	
	// Alpha/Beta: Hide face if neighbor is same fluid material (BlockFluids.java:58, LiquidTile.java:68-69)
	if (&neighborMat == &fluidMaterial)
		return false;
	
	// Alpha/Beta: Hide face if neighbor is ice (BlockFluids.java:58, LiquidTile.java:70-71)
	if (&neighborMat == &Material::ice)
		return false;
	
	// Alpha/Beta: Always show top face (face == 1) (BlockFluids.java:58, LiquidTile.java:73)
	if (face == Facing::UP)
		return true;
	
	// Alpha/Beta: Otherwise use base class logic (which checks isSolidTile on the neighbor position)
	return Tile::shouldRenderFace(level, x, y, z, face);
}

// Beta: LiquidTile.getAABB() returns null (no collision) (LiquidTile.java:77-80)
AABB *FluidTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	return nullptr;
}

// Beta: LiquidTile.getRenderShape() returns 4 (LiquidTile.java:82-85)
Tile::Shape FluidTile::getRenderShape()
{
	return SHAPE_WATER;
}

// Beta: LiquidTile.getResource() returns 0 (LiquidTile.java:87-90)
int_t FluidTile::getResource(int_t data, Random &random)
{
	return 0;
}

// Beta: LiquidTile.getResourceCount() returns 0 (LiquidTile.java:92-95)
int_t FluidTile::getResourceCount(Random &random)
{
	return 0;
}

// Beta: LiquidTile.getTickDelay() - water=5, lava=30 (LiquidTile.java:185-192)
int_t FluidTile::getTickDelay()
{
	if (&fluidMaterial == &Material::water)
	{
		return 5;
	}
	else
	{
		return (&fluidMaterial == &Material::lava) ? 30 : 0;
	}
}

// Beta: LiquidTile.getBrightness() - max of current and above (LiquidTile.java:194-199)
float FluidTile::getBrightness(LevelSource &level, int_t x, int_t y, int_t z)
{
	float current = level.getBrightness(x, y, z);
	float above = level.getBrightness(x, y + 1, z);
	return (current > above) ? current : above;
}

// Beta: LiquidTile.getRenderLayer() - 1 for water, 0 for lava (LiquidTile.java:206-209)
int_t FluidTile::getRenderLayer()
{
	return (&fluidMaterial == &Material::water) ? 1 : 0;
}

// Beta: LiquidTile.onPlace() calls updateLiquid (LiquidTile.java:244-247)
void FluidTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	updateLiquid(level, x, y, z);
}

// Beta: LiquidTile.neighborChanged() calls updateLiquid (LiquidTile.java:249-252)
void FluidTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	updateLiquid(level, x, y, z);
}

// Beta: LiquidTile.handleEntityInside() - applies flow vector (LiquidTile.java:177-183)
void FluidTile::handleEntityInside(Level &level, int_t x, int_t y, int_t z, Entity *entity, Vec3 &motion)
{
	Vec3 *flow = getFlow(level, x, y, z);
	motion.x = motion.x + flow->x;
	motion.y = motion.y + flow->y;
	motion.z = motion.z + flow->z;
}

// Beta: LiquidTile.animateTick() - water sounds, lava particles (LiquidTile.java:211-229)
void FluidTile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{
	// Beta: Water sounds (LiquidTile.java:212-218)
	if (&fluidMaterial == &Material::water && random.nextInt(64) == 0)
	{
		int_t data = level.getData(x, y, z);
		if (data > 0 && data < 8)
		{
			// TODO: Play sound "liquid.water" when sound system is implemented
			// level.playSound(x + 0.5F, y + 0.5F, z + 0.5F, "liquid.water", random.nextFloat() * 0.25F + 0.75F, random.nextFloat() * 1.0F + 0.5F);
		}
	}
	
	// newb12: Lava particles (LiquidTile.java:220-228)
	// Ported 1:1 from newb12/net/minecraft/world/level/tile/LiquidTile.java
	if (&fluidMaterial == &Material::lava)  // newb12: if (this.material == Material.lava)
	{
		const Material &aboveMat = level.getMaterial(x, y + 1, z);  // newb12: var1.getMaterial(var2, var3 + 1, var4) == Material.air
		const Material &airMat = Material::air;
		if (&aboveMat == &airMat &&  // newb12: var1.getMaterial(var2, var3 + 1, var4) == Material.air
		    !level.isSolidTile(x, y + 1, z) &&  // newb12: !var1.isSolidTile(var2, var3 + 1, var4)
		    random.nextInt(100) == 0)  // newb12: var5.nextInt(100) == 0
		{
			double px = x + random.nextDouble();  // newb12: double var12 = var2 + var5.nextFloat()
			double py = y + yy1;  // newb12: double var8 = var3 + this.yy1
			double pz = z + random.nextDouble();  // newb12: double var10 = var4 + var5.nextFloat()
			level.addParticle(u"lava", px, py, pz, 0.0, 0.0, 0.0);  // newb12: var1.addParticle("lava", var12, var8, var10, 0.0, 0.0, 0.0)
		}
	}
}

// Beta: LiquidTile.getSlopeAngle() - static helper for flow direction (LiquidTile.java:230-242)
double FluidTile::getSlopeAngle(LevelSource &level, int_t x, int_t y, int_t z, const Material &material)
{
	Vec3 *flow = nullptr;
	if (&material == &Material::water)
	{
		// Beta: Cast to LiquidTile and call getFlow
		// We need to get the water tile instance
		if (Tile::tiles[8] != nullptr)  // Tile.water.id = 8
		{
			FluidTile *waterTile = dynamic_cast<FluidTile*>(Tile::tiles[8]);
			if (waterTile != nullptr)
				flow = waterTile->getFlow(level, x, y, z);
		}
	}
	
	if (&material == &Material::lava)
	{
		// Beta: Cast to LiquidTile and call getFlow
		if (Tile::tiles[10] != nullptr)  // Tile.lava.id = 10
		{
			FluidTile *lavaTile = dynamic_cast<FluidTile*>(Tile::tiles[10]);
			if (lavaTile != nullptr)
				flow = lavaTile->getFlow(level, x, y, z);
		}
	}
	
	if (flow == nullptr)
		return -1000.0;
	
	// Beta: return var5.x == 0.0 && var5.z == 0.0 ? -1000.0 : Math.atan2(var5.z, var5.x) - Math.PI * 0.5
	if (flow->x == 0.0 && flow->z == 0.0)
		return -1000.0;
	return std::atan2(flow->z, flow->x) - (M_PI * 0.5);
}

// Beta: LiquidTile.getDepth() - gets fluid level at position (LiquidTile.java:33-35)
int_t FluidTile::getDepth(Level &level, int_t x, int_t y, int_t z)
{
	return (&level.getMaterial(x, y, z) != &fluidMaterial) ? -1 : level.getData(x, y, z);
}

// Beta: LiquidTile.getRenderedDepth() - gets rendered depth (normalized) (LiquidTile.java:37-48)
int_t FluidTile::getRenderedDepth(LevelSource &level, int_t x, int_t y, int_t z)
{
	if (&level.getMaterial(x, y, z) != &fluidMaterial)
	{
		return -1;
	}
	else
	{
		int_t data = level.getData(x, y, z);
		if (data >= 8)
		{
			data = 0;
		}
		return data;
	}
}

// Beta: LiquidTile.getFlow() - calculates flow vector (LiquidTile.java:97-175)
Vec3 *FluidTile::getFlow(LevelSource &level, int_t x, int_t y, int_t z)
{
	Vec3 *flow = Vec3::newTemp(0.0, 0.0, 0.0);
	int_t currentDepth = getRenderedDepth(level, x, y, z);
	
	// Beta: Check 4 horizontal neighbors (LiquidTile.java:101-133)
	for (int_t i = 0; i < 4; ++i)
	{
		int_t nx = x;
		int_t nz = z;
		if (i == 0)
			nx = x - 1;
		if (i == 1)
			nz = z - 1;
		if (i == 2)
			nx++;
		if (i == 3)
			nz++;
		
		int_t neighborDepth = getRenderedDepth(level, nx, y, nz);
		if (neighborDepth < 0)
		{
			if (!level.getMaterial(nx, y, nz).blocksMotion())
			{
				neighborDepth = getRenderedDepth(level, nx, y - 1, nz);
				if (neighborDepth >= 0)
				{
					int_t diff = neighborDepth - (currentDepth - 8);
					flow = flow->add((nx - x) * diff, (y - y) * diff, (nz - z) * diff);
				}
			}
		}
		else if (neighborDepth >= 0)
		{
			int_t diff = neighborDepth - currentDepth;
			flow = flow->add((nx - x) * diff, (y - y) * diff, (nz - z) * diff);
		}
	}
	
	// Beta: If source block (metadata >= 8), check for downward flow (LiquidTile.java:135-172)
	if (level.getData(x, y, z) >= 8)
	{
		bool hasSide = false;
		if (hasSide || shouldRenderFace(level, x, y, z - 1, Facing::NORTH))
			hasSide = true;
		if (hasSide || shouldRenderFace(level, x, y, z + 1, Facing::SOUTH))
			hasSide = true;
		if (hasSide || shouldRenderFace(level, x - 1, y, z, Facing::WEST))
			hasSide = true;
		if (hasSide || shouldRenderFace(level, x + 1, y, z, Facing::EAST))
			hasSide = true;
		if (hasSide || shouldRenderFace(level, x, y + 1, z - 1, Facing::NORTH))
			hasSide = true;
		if (hasSide || shouldRenderFace(level, x, y + 1, z + 1, Facing::SOUTH))
			hasSide = true;
		if (hasSide || shouldRenderFace(level, x - 1, y + 1, z, Facing::WEST))
			hasSide = true;
		if (hasSide || shouldRenderFace(level, x + 1, y + 1, z, Facing::EAST))
			hasSide = true;
		
		if (hasSide)
		{
			Vec3 *normalized = flow->normalize();
			flow = normalized->add(0.0, -6.0, 0.0);
		}
	}
	
	return flow->normalize();
}

// Beta: LiquidTile.updateLiquid() - checks for water+lava hardening (LiquidTile.java:254-290)
void FluidTile::updateLiquid(Level &level, int_t x, int_t y, int_t z)
{
	if (level.getTile(x, y, z) == id)
	{
		if (&fluidMaterial == &Material::lava)
		{
			// Beta: Check all 6 adjacent blocks for water (LiquidTile.java:256-277)
			bool hasWater = false;
			const Material &waterMat = Material::water;
			if (hasWater || &level.getMaterial(x, y, z - 1) == &waterMat)
				hasWater = true;
			if (hasWater || &level.getMaterial(x, y, z + 1) == &waterMat)
				hasWater = true;
			if (hasWater || &level.getMaterial(x - 1, y, z) == &waterMat)
				hasWater = true;
			if (hasWater || &level.getMaterial(x + 1, y, z) == &waterMat)
				hasWater = true;
			if (hasWater || &level.getMaterial(x, y + 1, z) == &waterMat)
				hasWater = true;
			if (hasWater || &level.getMaterial(x, y - 1, z) == &waterMat)
				hasWater = true;
			
			if (hasWater)
			{
				int_t data = level.getData(x, y, z);
				// Beta: Lava source (metadata 0) → obsidian, flowing (1-4) → stoneBrick (LiquidTile.java:279-284)
				if (data == 0)
				{
					// Beta: Tile.obsidian.id (ID 49)
					level.setTile(x, y, z, 49);
				}
				else if (data <= 4)
				{
					// Beta: Tile.stoneBrick.id (ID 4)
					level.setTile(x, y, z, 4);
				}
				
				fizz(level, x, y, z);
			}
		}
	}
}

// Beta: LiquidTile.fizz() - plays fizz sound and particles (LiquidTile.java:292-298)
void FluidTile::fizz(Level &level, int_t x, int_t y, int_t z)
{
	// TODO: Play sound "random.fizz" when sound system is implemented
	// level.playSound(x + 0.5F, y + 0.5F, z + 0.5F, "random.fizz", 0.5F, 2.6F + (level.random.nextFloat() - level.random.nextFloat()) * 0.8F);
	
	// Beta: Spawn 8 smoke particles (LiquidTile.java:295-297)
	for (int_t i = 0; i < 8; ++i)
	{
		double px = x + (static_cast<double>(rand()) / RAND_MAX);
		double py = y + 1.2;
		double pz = z + (static_cast<double>(rand()) / RAND_MAX);
		// TODO: Add particle "largesmoke" when particle system is implemented
		// level.addParticle("largesmoke", px, py, pz, 0.0, 0.0, 0.0);
	}
}
