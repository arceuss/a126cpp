#pragma once

#include <string>
#include <array>
#include <vector>

#include "Facing.h"
#include "locale/Descriptive.h"

#include "world/level/material/Material.h"
#include "world/phys/AABB.h"

#include "java/Type.h"
#include "java/Random.h"
#include "java/String.h"

#include <memory>

class Level;
class LevelSource;
class Player;
class Entity;
class Mob;
class Player;
class TileEntity;

class StoneTile;
class GrassTile;
class DirtTile;
class CobblestoneTile;
class ObsidianTile;
class SandTile;
class GravelTile;
class TreeTile;
class LeafTile;
class FlowerTile;
class SaplingTile;
class MushroomTile;
class ReedTile;
class CactusTile;
class OreTile;
class FluidFlowingTile;
class FluidStationaryTile;
class IceTile;
class MossyCobblestoneTile;
class ChestTile;
class MobSpawnerTile;
class ClayTile;
class SnowTile;
class SnowBlockTile;
class TorchTile;
class FireTile;
class WorkbenchTile;
class StoneSlabTile;
class WoodTile;
class UnbreakableTile;
class RedBrickTile;
class MetalTile;
class GlassTile;
class ClothTile;
class SpongeTile;
class BookshelfTile;
class TntTile;
class StairTile;
class FarmTile;
class CropTile;
class FurnaceTile;
class LadderTile;
class RailTile;
class SignTile;
class FenceTile;
class ButtonTile;
class LeverTile;
class DoorTile;
class RedStoneDustTile;
class RedStoneOreTile;
class NotGateTile;
class PressurePlateTile;
class RecordPlayerTile;
class PumpkinTile;
class HellStoneTile;
class HellSandTile;
class LightGemTile;
class PortalTile;

class Tile //: public Descriptive<Tile>
{
private:
	static const jstring TILE_DESCRIPTION_PREFIX;

public:
	enum Shape
	{
		SHAPE_INVISIBLE = -1,
		SHAPE_BLOCK,
		SHAPE_CROSS_TEXTURE,
		SHAPE_TORCH,
		SHAPE_FIRE,
		SHAPE_WATER,
		SHAPE_RED_DUST,
		SHAPE_ROWS,
		SHAPE_DOOR,
		SHAPE_LADDER,
		SHAPE_RAIL,
		SHAPE_STAIRS,
		SHAPE_FENCE,
		SHAPE_LEVER,
		SHAPE_CACTUS,
	};

	// Tile properties
	static std::array<Tile *, 256> tiles;

	static std::array<bool, 256> shouldTick;
	static std::array<bool, 256> solid;
	static std::array<bool, 256> isEntityTile;
	static std::array<int_t, 256> lightBlock;
	static std::array<bool, 256> translucent;
	static std::array<int_t, 256> lightEmission;

	// Tiles
	static StoneTile rock;
	static GrassTile grass;
	static DirtTile dirt;
	
	static CobblestoneTile cobblestone;  // Block ID 4 (Alpha Block.java:26, Beta Tile.java:69 - stoneBrick)
	static WoodTile wood;                 // Block ID 5 (Beta Tile.java:74)
	static UnbreakableTile unbreakable;  // Block ID 7 (Beta Tile.java:76 - bedrock)
	static ObsidianTile obsidian;        // Block ID 49 (Alpha Block.java:71)

	static SandTile sand;
	static GravelTile gravel;

	static TreeTile treeTrunk;
	static LeafTile leaves;
	
	// Plant blocks
	static FlowerTile plantYellow;   // Block ID 37 (Alpha Block.java:59)
	static FlowerTile plantRed;      // Block ID 38 (Alpha Block.java:60)
	static SaplingTile sapling;      // Block ID 6 (Beta Tile.java:75)
	
	static MushroomTile mushroomBrown;  // Block ID 39 (Alpha Block.java:61)
	static MushroomTile mushroomRed;    // Block ID 40 (Alpha Block.java:62)
	
	static ReedTile reed;  // Block ID 83 (Alpha Block.java:105)
	
	static CactusTile cactus;  // Block ID 81 (Alpha Block.java:103)
	
	// Ore blocks
	static OreTile oreCoal;      // Block ID 16 (Alpha Block.java:38)
	static OreTile oreIron;      // Block ID 15 (Alpha Block.java:37)
	static OreTile oreGold;      // Block ID 14 (Alpha Block.java:36)
	static OreTile oreDiamond;   // Block ID 56 (Alpha Block.java:78)
	// Note: Redstone ore (ID 73) is now RedStoneOreTile redStoneOre (see Redstone components section)
	
	// Fluid blocks (Beta 1.2 Tile.java:81-92)
	// Beta 1.2 naming: water = dynamic/flowing (source), calmWater = static/stationary (flowing)
	static FluidFlowingTile water;        // Block ID 8 (LiquidTileDynamic - source/flowing water)
	static FluidStationaryTile calmWater; // Block ID 9 (LiquidTileStatic - stationary/flowing water)
	static FluidFlowingTile lava;         // Block ID 10 (LiquidTileDynamic - source/flowing lava)
	static FluidStationaryTile calmLava;  // Block ID 11 (LiquidTileStatic - stationary/flowing lava)
	
	// Legacy Alpha naming (references for compatibility)
	static FluidFlowingTile &waterStill;   // Reference to water (ID 8)
	static FluidStationaryTile &waterMoving; // Reference to calmWater (ID 9)
	static FluidFlowingTile &lavaStill;    // Reference to lava (ID 10)
	static FluidStationaryTile &lavaMoving;  // Reference to calmLava (ID 11)
	
	// Special blocks
	static IceTile ice;  // Block ID 79 (Alpha Block.java:101)
	static SnowBlockTile snowBlock;  // Block ID 80 (Beta Tile.java:226) - full snow block (different from topSnow ID 78)
	static TorchTile torch;  // Block ID 50 (Beta Tile.java:171) - light-emitting decoration block
	static FireTile fire;  // Block ID 51 (Beta Tile.java:172) - fire block with spread mechanics
	static StoneSlabTile stoneSlab;  // Block ID 43 (Beta Tile.java:144) - double stone slab
	static StoneSlabTile stoneSlabHalf;  // Block ID 44 (Beta Tile.java:149) - half stone slab
	static WorkbenchTile workBench;  // Block ID 58 (Beta Tile.java:187) - crafting table
	static CropTile crops;  // Block ID 59 (Beta Tile.java:188) - wheat crops
	static FarmTile farmland;  // Block ID 60 (Beta Tile.java:189) - tilled farmland
	static FurnaceTile furnace;  // Block ID 61 (Beta Tile.java:190) - furnace (unlit)
	static FurnaceTile furnace_lit;  // Block ID 62 (Beta Tile.java:191) - furnace (lit)
	static LadderTile ladder;  // Block ID 65 (Beta Tile.java:196) - ladder
	static RailTile rail;  // Block ID 66 (Beta Tile.java:199 - rail)
	static SignTile sign;  // Block ID 63 (Beta Tile.java:196 - sign post)
	static SignTile wallSign;  // Block ID 68 (Beta Tile.java:201 - wall sign)
	static FenceTile fence;  // Block ID 85 (Beta Tile.java:229 - fence)
	static ButtonTile button;  // Block ID 77 (Beta Tile.java:708 - button)
	static LeverTile lever;  // Block ID 69 (Beta Tile.java:202 - lever)
	static DoorTile door_wood;  // Block ID 64 (Beta Tile.java:197 - door_wood)
	static DoorTile door_iron;  // Block ID 71 (Beta Tile.java:204 - door_iron)
	static StairTile stairs_wood;  // Block ID 53 (Beta Tile.java:178 - stairs_wood, Alpha Block.stairCompactPlanks)
	static StairTile stairs_stone;  // Block ID 67 (Beta Tile.java:200 - stairs_stone, Alpha Block.stairCompactCobblestone)
	
	// Redstone components (Phase 4)
	static RedStoneDustTile redStoneDust;  // Block ID 55 (Beta Tile.java:183 - redStoneDust)
	static RedStoneOreTile redStoneOre;  // Block ID 73 (Beta Tile.java:206 - redStoneOre, unlit)
	static RedStoneOreTile redStoneOre_lit;  // Block ID 74 (Beta Tile.java:211 - redStoneOre_lit)
	static NotGateTile notGate_off;  // Block ID 75 (Beta Tile.java:217 - notGate_off)
	static NotGateTile notGate_on;  // Block ID 76 (Beta Tile.java:218 - notGate_on)
	static PressurePlateTile pressurePlate_stone;  // Block ID 70 (Beta Tile.java:700 - pressurePlate_stone)
	static PressurePlateTile pressurePlate_wood;  // Block ID 72 (Beta Tile.java:704 - pressurePlate_wood)
	
	// Dungeon blocks (Beta 1.2)
	static MossyCobblestoneTile mossStone;  // Block ID 48 (Beta Tile.java:161)
	static MobSpawnerTile mobSpawner;       // Block ID 52 (Beta Tile.java:177)
	static ChestTile chest;                 // Block ID 54 (Beta Tile.java:179)
	
	// Simple blocks
	static RedBrickTile redBrick;  // Block ID 45 (Beta Tile.java:154)
	static MetalTile goldBlock;    // Block ID 41 (Beta Tile.java:164 - blockGold)
	static MetalTile ironBlock;    // Block ID 42 (Beta Tile.java:169 - blockSteel)
	static MetalTile emeraldBlock; // Block ID 57 (Beta Tile.java:182 - blockDiamond)
	static GlassTile glass;        // Block ID 20 (Beta Tile.java:105 - GlassTile)
	static ClothTile cloth;        // Block ID 35 (Alpha Block.java:57 - cloth, Beta ClothTile.java)
	static SpongeTile sponge;      // Block ID 19 (Beta Tile.java:104 - Sponge)
	static BookshelfTile bookshelf; // Block ID 47 (Beta Tile.java:160 - BookshelfTile)
	static TntTile tnt;             // Block ID 46 (Beta Tile.java:106 - TntTile)
	
	// Special blocks
	static SnowTile snow;  // Block ID 78 (Alpha Block.java:100, Beta Tile.java:224 - topSnow)
	static ClayTile clay;  // Block ID 82 (Beta Tile.java:228, Alpha Block.java:104)
	
	// Phase 5: Advanced blocks
	static RecordPlayerTile recordPlayer;  // Block ID 84 (Beta Tile.java:230 - recordPlayer)
	static PumpkinTile pumpkin;            // Block ID 86 (Beta Tile.java:236 - pumpkin)
	static PumpkinTile litPumpkin;         // Block ID 91 (Beta Tile.java:249 - litPumpkin)
	static HellStoneTile hellRock;         // Block ID 87 (Beta Tile.java:237 - hellRock)
	static HellSandTile hellSand;          // Block ID 88 (Beta Tile.java:238 - hellSand)
	static LightGemTile lightGem;          // Block ID 89 (Beta Tile.java:239 - lightGem)
	static PortalTile portalTile;          // Block ID 90 (Beta Tile.java:244 - portalTile)
	
	// Helper functions to get block IDs without requiring full type definitions
	static int_t getPlantYellowId() { return 37; }
	static int_t getPlantRedId() { return 38; }
	static int_t getMushroomBrownId() { return 39; }
	static int_t getMushroomRedId() { return 40; }
	static int_t getReedId() { return 83; }
	static int_t getCactusId() { return 81; }
	static int_t getOreCoalId() { return 16; }
	static int_t getOreIronId() { return 15; }
	static int_t getOreGoldId() { return 14; }
	static int_t getOreDiamondId() { return 56; }
	static int_t getOreRedstoneId() { return 73; }

	static void initTiles();

public:
	int_t tex = 0;

	int_t id = 0;

protected:
	float destroySpeed = 0.0f;
	float explosionResistance = 0.0f;

public:
	double xx0 = 0.0;
	double yy0 = 0.0;
	double zz0 = 0.0;
	
	double xx1 = 0.0;
	double yy1 = 0.0;
	double zz1 = 0.0;

	float gravity = 1.0f;

	const Material &material;

	float friction = 0.6f;

protected:
	Tile(int_t id, const Material &material);
	Tile(int_t id, int_t tex, const Material &material);

	virtual ~Tile() {}

	Tile &setLightBlock(int_t lightBlock);
	Tile &setLightEmission(int_t lightEmission);
	Tile &setExplodeable(float resistance);
	Tile &setExplosionResistance(float resistance);  // Directly set explosion resistance

private:
	virtual bool isTranslucent();

public:
	virtual bool isCubeShaped();
	virtual Shape getRenderShape();

protected:
	Tile &setDestroyTime(float time);
	void setTicking(bool ticking);
	
public:
	// Public getters for properties (needed by StairTile and other delegating tiles)
	float getDestroySpeed() const { return destroySpeed; }
	float getExplosionResistance() const { return explosionResistance; }

public:
	void setShape(float x0, float y0, float z0, float x1, float y1, float z1);

	virtual float getBrightness(LevelSource &level, int_t x, int_t y, int_t z);

	static bool isFaceVisible(LevelSource &level, int_t x, int_t y, int_t z, Facing face);
	virtual bool shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face);

	virtual int_t getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face);
	virtual int_t getTexture(Facing face, int_t data);
	virtual int_t getTexture(Facing face);

	virtual AABB *getTileAABB(Level &level, int_t x, int_t y, int_t z);

	virtual void addAABBs(Level &level, int_t x, int_t y, int_t z, AABB &bb, std::vector<AABB *> &aabbList);
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z);

	virtual bool isSolidRender();

	virtual bool mayPick(int_t data, bool canPickLiquid);
	virtual bool mayPick();

	// Beta: Tile.mayPlace() - checks if block can be placed (Tile.java:618-621)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z);

	virtual void tick(Level &level, int_t x, int_t y, int_t z, Random &random);
	virtual void animateTick(Level &level, int_t x, int_t y, int_t z, Random &random);
	virtual void destroy(Level &level, int_t x, int_t y, int_t z, int_t data);
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile);
	virtual void addLights(Level &level, int_t x, int_t y, int_t z);
	
	virtual int_t getTickDelay();

	virtual void onPlace(Level &level, int_t x, int_t y, int_t z);
	virtual void onRemove(Level &level, int_t x, int_t y, int_t z);
	
	// Beta: EntityTile.newTileEntity() - creates tile entity for this block
	virtual std::shared_ptr<TileEntity> newTileEntity() { return nullptr; }

	virtual int_t getResourceCount(Random &random);
	virtual int_t getResource(int_t data, Random &random);

	float getDestroyProgress(Player &player);

	virtual void spawnResources(Level &level, int_t x, int_t y, int_t z, int_t data);
	virtual void spawnResources(Level &level, int_t x, int_t y, int_t z, int_t data, float chance);

	virtual int_t getSpawnResourcesAuxValue(int_t data);

	virtual HitResult clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to);

protected:
	virtual bool containsX(Vec3 *vec);
	virtual bool containsY(Vec3 *vec);
	virtual bool containsZ(Vec3 *vec);

public:
	virtual int_t getRenderLayer();

	virtual void stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity);
	virtual void setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face);
	virtual void setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob);  // Beta: Tile.setPlacedBy() - called when block is placed by mob
	virtual void prepareRender(Level &level, int_t x, int_t y, int_t z);
	virtual void attack(Level &level, int_t x, int_t y, int_t z, Player &player);
	
	// Beta: Tile.use() - called when player right-clicks block (Tile.java:623-625)
	virtual bool use(Level &level, int_t x, int_t y, int_t z, Player &player);

	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z);
	virtual int_t getColor(LevelSource &level, int_t x, int_t y, int_t z);

	virtual void entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity);

	virtual void updateDefaultShape();

	virtual void playerDestroy(Level &level, int_t x, int_t y, int_t z, int_t data);

	// Beta: Tile.getSignal() - returns signal strength (Tile.java:657-659)
	virtual bool getSignal(LevelSource &level, int_t x, int_t y, int_t z, int_t facing) { return false; }

	// Beta: Tile.getDirectSignal() - returns direct signal (Tile.java:668-670)
	virtual bool getDirectSignal(Level &level, int_t x, int_t y, int_t z, int_t facing) { return false; }

	// Beta: Tile.isSignalSource() - returns if block is signal source (Tile.java:661-663)
	virtual bool isSignalSource() { return false; }

	// Beta: Tile.SoundType class - defines sound properties for tiles (Tile.java:719-745)
public:
	class SoundType
	{
	public:
		jstring name;
		float volume;
		float pitch;

		// Beta: SoundType(String var1, float var2, float var3) (Tile.java:724-728)
		SoundType(const jstring &name, float volume, float pitch) 
			: name(name), volume(volume), pitch(pitch) {}

		// Beta: getVolume() (Tile.java:730-732)
		float getVolume() const { return volume; }

		// Beta: getPitch() (Tile.java:734-736)
		float getPitch() const { return pitch; }

		// Beta: getBreakSound() - returns "step." + name (Tile.java:738-740)
		virtual jstring getBreakSound() const { return u"step." + name; }

		// Beta: getStepSound() - returns "step." + name (Tile.java:742-744)
		virtual jstring getStepSound() const { return u"step." + name; }
	};

	// Beta: GlassSoundType with custom getBreakSound (Tile.java:31-36)
	class GlassSoundType : public SoundType
	{
	public:
		GlassSoundType() : SoundType(u"stone", 1.0f, 1.0f) {}
		
		virtual jstring getBreakSound() const override { return u"random.glass"; }
	};

	// Beta: SoundType constants (Tile.java:25-43)
	static const SoundType SOUND_NORMAL;
	static const SoundType SOUND_WOOD;
	static const SoundType SOUND_GRAVEL;
	static const SoundType SOUND_GRASS;
	static const SoundType SOUND_STONE;
	static const SoundType SOUND_METAL;
	static const SoundType SOUND_CLOTH;
	static const SoundType SOUND_SAND;
	static const GlassSoundType SOUND_GLASS;  // Has custom getBreakSound returning "random.glass"

protected:
	const SoundType *soundType = &SOUND_STONE;  // Default to stone sound

public:
	// Beta: setSoundType() - returns *this for chaining
	Tile &setSoundType(const SoundType &type) { soundType = &type; return *this; }
	const SoundType *getSoundType() const { return soundType; }
};
