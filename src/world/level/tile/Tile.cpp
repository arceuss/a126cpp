#include "world/level/tile/Tile.h"

#include <stdexcept>
#include <memory>
#include <iostream>

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/entity/Entity.h"
#include "world/entity/player/Player.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/ItemStack.h"

// Debug flag for break speed - disable in release builds
// #define DEBUG_BREAK_SPEED

// Tile properties
std::array<Tile *, 256> Tile::tiles;

std::array<bool, 256> Tile::shouldTick = {};
std::array<bool, 256> Tile::solid = {};
std::array<bool, 256> Tile::isEntityTile = {};
std::array<int_t, 256> Tile::lightBlock = {};
std::array<bool, 256> Tile::translucent = {};
std::array<int_t, 256> Tile::lightEmission = {};

// Beta: SoundType constants (Tile.java:25-43)
const Tile::SoundType Tile::SOUND_NORMAL(u"stone", 1.0f, 1.0f);
const Tile::SoundType Tile::SOUND_WOOD(u"wood", 1.0f, 1.0f);
const Tile::SoundType Tile::SOUND_GRAVEL(u"gravel", 1.0f, 1.0f);
const Tile::SoundType Tile::SOUND_GRASS(u"grass", 1.0f, 1.0f);
const Tile::SoundType Tile::SOUND_STONE(u"stone", 1.0f, 1.0f);
const Tile::SoundType Tile::SOUND_METAL(u"stone", 1.0f, 1.5f);
const Tile::SoundType Tile::SOUND_CLOTH(u"cloth", 1.0f, 1.0f);
const Tile::SoundType Tile::SOUND_SAND(u"sand", 1.0f, 1.0f);
const Tile::GlassSoundType Tile::SOUND_GLASS;

// Tiles
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/CobblestoneTile.h"
#include "world/level/tile/WoodTile.h"
#include "world/level/tile/UnbreakableTile.h"
#include "world/level/tile/ObsidianTile.h"
#include "world/level/tile/RedBrickTile.h"
#include "world/level/tile/MetalTile.h"
#include "world/level/tile/GlassTile.h"
#include "world/level/tile/ClothTile.h"
#include "world/level/tile/SpongeTile.h"
#include "world/level/tile/BookshelfTile.h"
#include "world/level/tile/TntTile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/GravelTile.h"
#include "world/level/tile/TreeTile.h"
#include "world/level/tile/LeafTile.h"
#include "world/level/tile/FlowerTile.h"
#include "world/level/tile/MushroomTile.h"
#include "world/level/tile/SaplingTile.h"
#include "world/level/tile/ReedTile.h"
#include "world/level/tile/CactusTile.h"
#include "world/level/tile/IceTile.h"
#include "world/level/tile/OreTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/tile/MossyCobblestoneTile.h"
#include "world/level/tile/ChestTile.h"
#include "world/level/tile/MobSpawnerTile.h"
#include "world/level/tile/ClayTile.h"
#include "world/level/tile/SnowTile.h"
#include "world/level/tile/SnowBlockTile.h"
#include "world/level/tile/TorchTile.h"
#include "world/level/tile/FireTile.h"
#include "world/level/tile/WorkbenchTile.h"
#include "world/level/tile/StoneSlabTile.h"
#include "world/level/tile/StairTile.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/tile/CropTile.h"
#include "world/level/tile/FurnaceTile.h"
#include "world/level/tile/LadderTile.h"
#include "world/level/tile/RailTile.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/FenceTile.h"
#include "world/level/tile/ButtonTile.h"
#include "world/level/tile/LeverTile.h"
#include "world/level/tile/DoorTile.h"
#include "world/level/tile/RedStoneDustTile.h"
#include "world/level/tile/RedStoneOreTile.h"
#include "world/level/tile/NotGateTile.h"
#include "world/level/tile/PressurePlateTile.h"
#include "world/level/tile/RecordPlayerTile.h"
#include "world/level/tile/PumpkinTile.h"
#include "world/level/tile/HellStoneTile.h"
#include "world/level/tile/HellSandTile.h"
#include "world/level/tile/LightGemTile.h"
#include "world/level/tile/PortalTile.h"

StoneTile Tile::rock = StoneTile(1, 1);
GrassTile Tile::grass = GrassTile(2);
DirtTile Tile::dirt = DirtTile(3, 2);

CobblestoneTile Tile::cobblestone = CobblestoneTile(4, 16);  // Alpha Block.java:26, Beta Tile.java:69 - stoneBrick
WoodTile Tile::wood = WoodTile(5, 4);                        // Beta Tile.java:74
UnbreakableTile Tile::unbreakable = UnbreakableTile(7, 17);  // Beta Tile.java:76 - bedrock
ObsidianTile Tile::obsidian = ObsidianTile(49, 37);          // Alpha Block.java:71

SandTile Tile::sand = SandTile(12, 18);
GravelTile Tile::gravel = GravelTile(13, 19);

TreeTile Tile::treeTrunk = TreeTile(17);
LeafTile Tile::leaves = LeafTile(18, 52);

// Plant blocks (Alpha Block.java:59-62, 103, 105)
// Beta: Sapling exists in Alpha 1.2.6 (Tile.java:75) - ID 6, texture 15
SaplingTile Tile::sapling = SaplingTile(6, 15);  // Beta: Tile.sapling (ID 6, texture 15)

// Fluid blocks (Beta 1.2 Tile.java:81-92)
// Beta 1.2 naming: water = dynamic/flowing (source), calmWater = static/stationary (flowing)
FluidFlowingTile Tile::water = FluidFlowingTile(8, Material::water);        // LiquidTileDynamic (ID 8)
FluidStationaryTile Tile::calmWater = FluidStationaryTile(9, Material::water);  // LiquidTileStatic (ID 9)
FluidFlowingTile Tile::lava = FluidFlowingTile(10, Material::lava);         // LiquidTileDynamic (ID 10)
FluidStationaryTile Tile::calmLava = FluidStationaryTile(11, Material::lava);    // LiquidTileStatic (ID 11)

// Legacy Alpha naming (aliases for compatibility)
FluidFlowingTile &Tile::waterStill = Tile::water;      // Alias for water (ID 8)
FluidStationaryTile &Tile::waterMoving = Tile::calmWater;  // Alias for calmWater (ID 9)
FluidFlowingTile &Tile::lavaStill = Tile::lava;        // Alias for lava (ID 10)
FluidStationaryTile &Tile::lavaMoving = Tile::calmLava;     // Alias for calmLava (ID 11)

// Plant blocks (Alpha Block.java:59-62, 103, 105)
// Note: Sapling is defined above with TreeTile/LeafTile
FlowerTile Tile::plantYellow = FlowerTile(37, 13);      // Alpha Block.java:59
FlowerTile Tile::plantRed = FlowerTile(38, 12);         // Alpha Block.java:60
MushroomTile Tile::mushroomBrown = MushroomTile(39, 29); // Alpha Block.java:61 (lightValue 2/16)
MushroomTile Tile::mushroomRed = MushroomTile(40, 28);   // Alpha Block.java:62
ReedTile Tile::reed = ReedTile(83, 73);                 // Alpha Block.java:105
CactusTile Tile::cactus = CactusTile(81, 70);           // Alpha Block.java:103

// Special blocks
IceTile Tile::ice = IceTile(79, 67);  // Alpha: Block.blockIce (Block.java:101) - ID 79, texture 67

// Dungeon blocks (Beta 1.2)
MossyCobblestoneTile Tile::mossStone = MossyCobblestoneTile(48, 36);  // Beta: Tile.mossStone (ID 48, texture 36)
MobSpawnerTile Tile::mobSpawner = MobSpawnerTile(52, 65);            // Beta: Tile.mobSpawner (ID 52, texture 65)
ChestTile Tile::chest = ChestTile(54);                                // Beta: Tile.chest (ID 54, texture 26)

// Simple blocks
RedBrickTile Tile::redBrick = RedBrickTile(45, 7);  // Beta: Tile.redBrick (ID 45, texture 7)
MetalTile Tile::goldBlock = MetalTile(41, 23);      // Beta: Tile.blockGold (ID 41, texture 23)
MetalTile Tile::ironBlock = MetalTile(42, 22);      // Beta: Tile.ironBlock (ID 42, texture 22)
StoneSlabTile Tile::stoneSlab = StoneSlabTile(43, true);  // Beta: Tile.stoneSlab (ID 43, double slab)
StoneSlabTile Tile::stoneSlabHalf = StoneSlabTile(44, false);  // Beta: Tile.stoneSlabHalf (ID 44, half slab)
MetalTile Tile::emeraldBlock = MetalTile(57, 24);   // Beta: Tile.blockDiamond (ID 57, texture 24)
GlassTile Tile::glass = GlassTile(20, 49, Material::glass, false);  // Beta: Tile.glass (ID 20, texture 49)
ClothTile Tile::cloth = ClothTile();                // Beta: Tile.cloth (ID 35, texture 64)
SpongeTile Tile::sponge = SpongeTile(19);           // Beta: Tile.sponge (ID 19, texture 48)
BookshelfTile Tile::bookshelf = BookshelfTile(47, 35);  // Beta: Tile.bookshelf (ID 47, texture 35)
TntTile Tile::tnt = TntTile(46, 8);                 // Beta: Tile.tnt (ID 46, texture 8)

// Special blocks
SnowTile Tile::snow = SnowTile(78, 66);  // Alpha: Block.blockSnow (ID 78, texture 66), Beta: Tile.topSnow
SnowBlockTile Tile::snowBlock = SnowBlockTile(80, 66);  // Beta: Tile.snow (ID 80, texture 66) - full snow block
TorchTile Tile::torch = TorchTile(50, 80);  // Beta: Tile.torch (ID 50, texture 80) - light-emitting decoration block
FireTile Tile::fire = FireTile(51, 31);  // Beta: Tile.fire (ID 51, texture 31) - fire block with spread mechanics
WorkbenchTile Tile::workBench = WorkbenchTile(58);  // Beta: Tile.workBench (ID 58, texture 59) - crafting table
CropTile Tile::crops = CropTile(59, 88);  // Beta: Tile.crops (ID 59, texture 88), Alpha: Block.crops
FarmTile Tile::farmland = FarmTile(60);  // Beta: Tile.farmland (ID 60, texture 87), Alpha: Block.tilledField
FurnaceTile Tile::furnace = FurnaceTile(61, false);  // Beta: Tile.furnace (ID 61, unlit), Alpha: Block.furnaceIdle
FurnaceTile Tile::furnace_lit = FurnaceTile(62, true);  // Beta: Tile.furnace_lit (ID 62, lit), Alpha: Block.furnaceActive
LadderTile Tile::ladder = LadderTile(65, 83);  // Beta: Tile.ladder (ID 65, texture 83), Alpha: Block.ladder
RailTile Tile::rail = RailTile(66, 128);  // Beta: Tile.rail (ID 66, texture 128), Alpha: Block.rail
SignTile Tile::sign = SignTile(63, true);  // Beta: Tile.sign (ID 63, sign post), Alpha: Block.signPost
SignTile Tile::wallSign = SignTile(68, false);  // Beta: Tile.wallSign (ID 68, wall sign), Alpha: Block.signWall
FenceTile Tile::fence = FenceTile(85, 4);  // Beta: Tile.fence (ID 85, texture 4), Alpha: Block.fence
ButtonTile Tile::button = ButtonTile(77, Tile::rock.tex);  // Beta: Tile.button (ID 77, texture rock.tex), Alpha: Block.button
LeverTile Tile::lever = LeverTile(69, 96);  // Beta: Tile.lever (ID 69, texture 96), Alpha: Block.lever
DoorTile Tile::door_wood = DoorTile(64, Material::wood);  // Beta: Tile.door_wood (ID 64, Material.wood), Alpha: Block.doorWood
DoorTile Tile::door_iron = DoorTile(71, Material::metal);  // Beta: Tile.door_iron (ID 71, Material.metal), Alpha: Block.doorIron
StairTile Tile::stairs_wood = StairTile(53, Tile::wood);  // Beta: Tile.stairs_wood (ID 53), Alpha: Block.stairCompactPlanks
StairTile Tile::stairs_stone = StairTile(67, Tile::cobblestone);  // Beta: Tile.stairs_stone (ID 67, uses stoneBrick), Alpha: Block.stairCompactCobblestone
ClayTile Tile::clay = ClayTile(82, 72);  // Beta: Tile.clay (ID 82, texture 72), Alpha: Block.blockClay

// Redstone components (Phase 4)
RedStoneDustTile Tile::redStoneDust = RedStoneDustTile(55, 84);  // Beta: Tile.redStoneDust (ID 55, texture 84), Alpha: Block.redstoneWire
RedStoneOreTile Tile::redStoneOre = RedStoneOreTile(73, 51, false);  // Beta: Tile.redStoneOre (ID 73, texture 51, lit=false), Alpha: Block.oreRedstone
RedStoneOreTile Tile::redStoneOre_lit = RedStoneOreTile(74, 51, true);  // Beta: Tile.redStoneOre_lit (ID 74, texture 51, lit=true), Alpha: Block.oreRedstoneGlowing
NotGateTile Tile::notGate_off = NotGateTile(75, 115, false);  // Beta: Tile.notGate_off (ID 75, texture 115, on=false), Alpha: Block.torchRedstoneIdle
NotGateTile Tile::notGate_on = NotGateTile(76, 99, true);  // Beta: Tile.notGate_on (ID 76, texture 99, on=true), Alpha: Block.torchRedstoneActive
PressurePlateTile Tile::pressurePlate_stone = PressurePlateTile(70, Tile::rock.tex, PressurePlateTile::Sensitivity::mobs);  // Beta: Tile.pressurePlate_stone (ID 70, texture rock.tex, Sensitivity.mobs), Alpha: Block.pressurePlateStone
PressurePlateTile Tile::pressurePlate_wood = PressurePlateTile(72, Tile::wood.tex, PressurePlateTile::Sensitivity::everything);  // Beta: Tile.pressurePlate_wood (ID 72, texture wood.tex, Sensitivity.everything), Alpha: Block.pressurePlatePlanks

// Phase 5: Advanced blocks
RecordPlayerTile Tile::recordPlayer = RecordPlayerTile(84, 74);  // Beta: Tile.recordPlayer (ID 84, texture 74)
PumpkinTile Tile::pumpkin = PumpkinTile(86, 102, false);  // Beta: Tile.pumpkin (ID 86, texture 102, lit=false)
PumpkinTile Tile::litPumpkin = PumpkinTile(91, 102, true);  // Beta: Tile.litPumpkin (ID 91, texture 102, lit=true)
HellStoneTile Tile::hellRock = HellStoneTile(87, 103);  // Beta: Tile.hellRock (ID 87, texture 103)
HellSandTile Tile::hellSand = HellSandTile(88, 104);  // Beta: Tile.hellSand (ID 88, texture 104)
LightGemTile Tile::lightGem = LightGemTile(89, 105, Material::glass);  // Beta: Tile.lightGem (ID 89, texture 105, Material.glass)
PortalTile Tile::portalTile = PortalTile(90, 14);  // Beta: Tile.portalTile (ID 90, texture 14)

// Ore blocks (Alpha Block.java:36-38, 78, 95)
OreTile Tile::oreCoal = OreTile(16, 34, 263);      // Alpha: drops Item.coal (shiftedIndex 263 = 256 + 7)
OreTile Tile::oreIron = OreTile(15, 33, 0);        // Alpha: drops blockID (Block.java:37, BlockOre.java:11)
OreTile Tile::oreGold = OreTile(14, 32, 0);        // Alpha: drops blockID (Block.java:36, BlockOre.java:11)
OreTile Tile::oreDiamond = OreTile(56, 50, 264);   // Alpha: drops Item.diamond (shiftedIndex 264 = 256 + 8)
// Note: redStoneOre (ID 73) is defined above in Redstone components section

void Tile::initTiles()
{
	// Beta: Set sound types (Tile.java:66-238)
	// Alpha block properties from Block.java:23-40
	rock.setDestroyTime(1.5f).setExplodeable(10.0f).setSoundType(SOUND_STONE);  // Beta: Tile.java:66
	grass.setDestroyTime(0.6f).setSoundType(SOUND_GRASS);  // Beta: Tile.java:67
	dirt.setDestroyTime(0.5f).setSoundType(SOUND_GRAVEL);  // Beta: Tile.java:68
	
	cobblestone.setDestroyTime(2.0f).setExplodeable(10.0f).setSoundType(SOUND_STONE);  // Beta: Tile.java:72
	wood.setDestroyTime(2.0f).setExplodeable(5.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:74
	unbreakable.setDestroyTime(-1.0f).setExplodeable(6000000.0f).setSoundType(SOUND_STONE);  // Beta: Tile.java:78
	obsidian.setDestroyTime(10.0f).setExplosionResistance(2000.0f).setSoundType(SOUND_STONE);  // Beta: Tile.java:169

	sand.setDestroyTime(0.5f).setSoundType(SOUND_SAND);  // Beta: Tile.java:93
	gravel.setDestroyTime(0.6f).setSoundType(SOUND_GRAVEL);  // Beta: Tile.java:94

	treeTrunk.setDestroyTime(2.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:98

	leaves.setDestroyTime(0.2f).setLightBlock(1).setSoundType(SOUND_GRASS);  // Beta: Tile.java:102
	
	// Beta: Lava light emission (Tile.java:83-87, 88-92)
	// Beta: lava.setLightEmission(1.0F) = 15 (full brightness)
	lava.setDestroyTime(0.0f).setLightEmission(15).setLightBlock(255);  // Beta: Tile.java:83-87
	calmLava.setDestroyTime(100.0f).setLightEmission(15).setLightBlock(255);  // Beta: Tile.java:88-92
	
	// Plant blocks - properties set in constructors
	// plantYellow: hardness 0.0F (Block.java:59), Beta: Tile.java:126
	// plantRed: hardness 0.0F (Block.java:60), Beta: Tile.java:127
	// mushroomBrown: hardness 0.0F, lightValue 2/16 (Block.java:61), Beta: Tile.java:130
	// mushroomRed: hardness 0.0F (Block.java:62), Beta: Tile.java:133
	// reed: hardness 0.0F (Block.java:105), Beta: Tile.java:229
	// cactus: hardness 0.4F (Block.java:103), Beta: Tile.java:227
	// Alpha: mushroomBrown has lightValue 2.0F / 16.0F = 0.125F, which rounds to 2/15 light level
	// setLightEmission takes value 0-15, so 2.0F/16.0F * 15 = 1.875 ≈ 2
	// Sapling properties (Beta: Tile.java:75)
	sapling.setDestroyTime(0.0f).setSoundType(SOUND_GRASS);  // Beta: Tile.java:75 (setDestroyTime(0.0F).setSoundType(SOUND_GRASS))
	
	plantYellow.setSoundType(SOUND_GRASS);  // Beta: Tile.java:126
	plantRed.setSoundType(SOUND_GRASS);  // Beta: Tile.java:127
	mushroomBrown.setLightEmission(2).setSoundType(SOUND_GRASS);  // Beta: Tile.java:130
	mushroomRed.setSoundType(SOUND_GRASS);  // Beta: Tile.java:133
	reed.setSoundType(SOUND_GRASS);  // Beta: Tile.java:229
	cactus.setSoundType(SOUND_CLOTH);  // Beta: Tile.java:227
	
	// Ensure plants are marked as non-solid and translucent (TransparentTile should handle this, but double-check)
	// Plants use cross-texture rendering and should not block light or be solid
	// Using direct IDs to avoid forward declaration issues
	solid[6] = false; // sapling
	solid[37] = false; // plantYellow
	solid[38] = false; // plantRed
	solid[39] = false; // mushroomBrown
	solid[40] = false; // mushroomRed
	solid[83] = false; // reed
	lightBlock[6] = 0; // sapling
	lightBlock[37] = 0; // plantYellow
	lightBlock[38] = 0; // plantRed
	lightBlock[39] = 0; // mushroomBrown
	lightBlock[40] = 0; // mushroomRed
	lightBlock[83] = 0; // reed
	translucent[6] = true; // sapling - let light through for proper lighting
	translucent[37] = true; // plantYellow - let light through for proper lighting
	translucent[38] = true; // plantRed
	translucent[39] = true; // mushroomBrown
	translucent[40] = true; // mushroomRed
	translucent[83] = true; // reed
	
	// Cactus is non-solid for rendering (isSolidRender() returns false) but uses custom render shape
	// Beta: lightBlock is set based on isSolidRender() - if false, lightBlock = 0 (Tile.java:280)
	solid[81] = false; // Cactus.isSolidRender() returns false (CactusTile.java:63-64)
	lightBlock[81] = 0; // Cactus doesn't block light since isSolidRender() is false
	translucent[81] = true; // Cactus is translucent for proper lighting
	
	// Ice block - properties set in IceTile constructor
	// solid[79] = false; // Already set by TransparentTile (ice.isSolidRender() returns false)
	ice.setSoundType(SOUND_GLASS);  // Beta: Tile.java:225 (ice uses SOUND_GLASS)
	lightBlock[79] = 3;  // Alpha: lightOpacity 3 (Block.java:101), Beta: setLightBlock(3)
	translucent[79] = true; // Ice is transparent
	
	// Ore blocks - properties set in OreTile constructor (Beta: Tile.java:95-97, 106, 181)
	oreCoal.setSoundType(SOUND_STONE);  // Beta: Tile.java:97
	oreIron.setSoundType(SOUND_STONE);  // Beta: Tile.java:96
	oreGold.setSoundType(SOUND_STONE);  // Beta: Tile.java:95
	oreDiamond.setSoundType(SOUND_STONE);  // Beta: Tile.java:181
	
	// Dungeon blocks - properties set in constructors (Beta: Tile.java:164, 177, 179)
	mossStone.setSoundType(SOUND_STONE);  // Beta: Tile.java:164
	mobSpawner.setSoundType(SOUND_METAL);  // Beta: Tile.java:177
	chest.setSoundType(SOUND_WOOD);  // Beta: Tile.java:179
	
	// Simple blocks
	redBrick.setDestroyTime(2.0f).setExplodeable(10.0f).setSoundType(SOUND_STONE);  // Beta: Tile.java:157
	goldBlock.setDestroyTime(3.0f).setExplodeable(10.0f).setSoundType(SOUND_METAL);  // Beta: Tile.java:164
	ironBlock.setDestroyTime(5.0f).setExplodeable(10.0f).setSoundType(SOUND_METAL);  // Beta: Tile.java:169
	emeraldBlock.setDestroyTime(5.0f).setExplodeable(10.0f).setSoundType(SOUND_METAL);  // Beta: Tile.java:182-185
	glass.setDestroyTime(0.3f).setSoundType(SOUND_GLASS);  // Beta: Tile.java:105 (GlassTile uses SOUND_GLASS)
	// Beta: Glass doesn't block light (GlassTile extends TransparentTile which has blocksLight() = false)
	lightBlock[20] = 0;  // Glass ID 20 - doesn't block light
	translucent[20] = true;  // Glass is translucent
	cloth.setDestroyTime(0.8f).setSoundType(SOUND_CLOTH);  // Beta: Tile.java:124 (hardness 0.8F, SOUND_CLOTH)
	sponge.setDestroyTime(0.6f).setSoundType(SOUND_GRASS);  // Beta: Tile.java:104
	bookshelf.setDestroyTime(1.5f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:160
	
	// Snow block - properties set in constructor (Beta: Tile.java:224)
	snow.setSoundType(SOUND_CLOTH);  // Beta: Tile.java:224
	
	// Clay block - properties set in constructor (Beta: Tile.java:228)
	clay.setSoundType(SOUND_GRAVEL);  // Beta: Tile.java:228
	
	// Crop block - properties set in constructor (Beta: Tile.java:188)
	crops.setDestroyTime(0.0f).setSoundType(SOUND_GRASS);  // Beta: Tile.java:188 (setDestroyTime(0.0F).setSoundType(SOUND_GRASS))
	// Alpha 1.2.6: Crops should not block light (they need light to grow)
	lightBlock[59] = 0;  // Crops ID 59 - doesn't block light
	translucent[59] = true;  // Crops are translucent for proper lighting
	
	// Farmland block - properties set in constructor (Beta: Tile.java:189)
	farmland.setDestroyTime(0.6f).setSoundType(SOUND_GRAVEL);  // Beta: Tile.java:189 (setDestroyTime(0.6F).setSoundType(SOUND_GRAVEL))
	// Alpha 1.2.6: Farmland should not block light (crops need light to grow)
	lightBlock[60] = 0;  // Farmland ID 60 - doesn't block light
	translucent[60] = true;  // Farmland is translucent for proper lighting
	
	// Furnace blocks - properties (Beta: Tile.java:190-194)
	furnace.setDestroyTime(3.5f).setSoundType(SOUND_STONE);  // Beta: Tile.java:190 (setDestroyTime(3.5F).setSoundType(SOUND_STONE))
	furnace_lit.setDestroyTime(3.5f).setSoundType(SOUND_STONE).setLightEmission(13);  // Beta: Tile.java:191-194 (setDestroyTime(3.5F).setSoundType(SOUND_STONE).setLightEmission(0.875F) = 13)
	
	// Workbench block - properties (Beta: Tile.java:187)
	workBench.setDestroyTime(2.5f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:187 (setDestroyTime(2.5F).setSoundType(SOUND_WOOD)), Alpha: Block.java:80 (hardness 2.5F, soundWoodFootstep)
	
	// Ladder block - properties (Beta: Tile.java:196)
	ladder.setDestroyTime(0.4f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:196 (setDestroyTime(0.4F).setSoundType(SOUND_WOOD))
	
	// Rail block - properties (Beta: Tile.java:199)
	rail.setDestroyTime(0.7f).setSoundType(SOUND_METAL);  // Beta: Tile.java:199 (setDestroyTime(0.7F).setSoundType(SOUND_METAL))
	// Beta: Rails don't block light (RailTile.blocksLight() returns false)
	lightBlock[66] = 0;  // Rail ID 66 - doesn't block light
	translucent[66] = true;  // Rails are translucent for proper lighting
	
	// Stairs blocks - properties (Beta: StairTile.java:17-23)
	// Stairs copy properties from base tile, but base tiles must be initialized first
	// Alpha: BlockStairs.java:12-14 sets hardness from base.blockHardness, resistance from base.blockResistance/3.0F, stepSound from base.stepSound
	// Beta: StairTile.java:20-22 sets destroyTime from base.destroySpeed, explodeable from base.explosionResistance/3.0F, soundType from base.soundType
	// Since stairs are constructed before initTiles(), we need to re-initialize them here
	// Wood stairs: base is wood (destroyTime 2.0f, explodeable 5.0f, SOUND_WOOD)
	stairs_wood.setDestroyTime(2.0f).setExplodeable(5.0f / 3.0f).setSoundType(SOUND_WOOD);  // Beta: uses wood properties
	// Stone stairs: base is cobblestone (destroyTime 2.0f, explodeable 10.0f, SOUND_STONE)
	stairs_stone.setDestroyTime(2.0f).setExplodeable(10.0f / 3.0f).setSoundType(SOUND_STONE);  // Beta: uses cobblestone properties
	
	// Sign blocks - properties (Beta: Tile.java:196, 201)
	sign.setDestroyTime(1.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:196 (setDestroyTime(1.0F).setSoundType(SOUND_WOOD))
	wallSign.setDestroyTime(1.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:201 (setDestroyTime(1.0F).setSoundType(SOUND_WOOD))
	
	// Fence block - properties (Beta: Tile.java:229)
	fence.setDestroyTime(2.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:229 (setDestroyTime(2.0F).setSoundType(SOUND_WOOD))
	
	// Button block - properties (Beta: Tile.java:708)
	button.setDestroyTime(0.5f).setSoundType(SOUND_STONE);  // Beta: Tile.java:708 (setDestroyTime(0.5F).setSoundType(SOUND_STONE))
	
	// Lever block - properties (Beta: Tile.java:202)
	lever.setDestroyTime(0.5f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:202 (setDestroyTime(0.5F).setSoundType(SOUND_WOOD))
	
	// Door blocks - properties (Beta: Tile.java:197, 204)
	door_wood.setDestroyTime(3.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:197 (setDestroyTime(3.0F).setSoundType(SOUND_WOOD))
	door_iron.setDestroyTime(5.0f).setSoundType(SOUND_METAL);  // Beta: Tile.java:204 (setDestroyTime(5.0F).setSoundType(SOUND_METAL))
	
	// Redstone components - properties (Phase 4)
	// RedStoneDust - properties (Beta: Tile.java:180)
	redStoneDust.setDestroyTime(0.0f).setSoundType(SOUND_NORMAL);  // Beta: Tile.java:180 (setDestroyTime(0.0F).setSoundType(SOUND_NORMAL))
	// Beta: Redstone dust should not block light (like other decoration blocks)
	lightBlock[55] = 0;  // Redstone dust (ID 55) - doesn't block light
	translucent[55] = true;  // Redstone dust is translucent
	
	// RedStoneOre - properties (Beta: Tile.java:206-210)
	redStoneOre.setDestroyTime(3.0f).setExplodeable(5.0f).setSoundType(SOUND_STONE);  // Beta: Tile.java:206-210 (setDestroyTime(3.0F), setExplodeable(5.0F), setSoundType(SOUND_STONE))
	
	// RedStoneOre_lit - properties (Beta: Tile.java:211-216)
	redStoneOre_lit.setLightEmission(10).setDestroyTime(3.0f).setExplodeable(5.0f).setSoundType(SOUND_STONE);  // Beta: Tile.java:211-216 (setLightEmission(0.625F*16=10), setDestroyTime(3.0F), setExplodeable(5.0F), setSoundType(SOUND_STONE))
	
	// NotGate_off - properties (Beta: Tile.java:217)
	notGate_off.setDestroyTime(0.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:217 (setDestroyTime(0.0F).setSoundType(SOUND_WOOD))
	// Alpha 1.2.6: Redstone torches should not block light (like normal torches before fix)
	// They also dim the block they're placed on (old torch behavior)
	lightBlock[75] = 0;  // Redstone torch off (ID 75) - doesn't block light
	translucent[75] = true;  // Redstone torch is translucent
	
	// NotGate_on - properties (Beta: Tile.java:218-222)
	notGate_on.setDestroyTime(0.0f).setLightEmission(8).setSoundType(SOUND_WOOD);  // Beta: Tile.java:218-222 (setDestroyTime(0.0F), setLightEmission(0.5F*16=8), setSoundType(SOUND_WOOD))
	// Alpha 1.2.6: Redstone torches should not block light
	lightBlock[76] = 0;  // Redstone torch on (ID 76) - doesn't block light
	translucent[76] = true;  // Redstone torch is translucent
	
	// PressurePlate_stone - properties (Beta: Tile.java:700-703)
	pressurePlate_stone.setDestroyTime(0.5f).setSoundType(SOUND_STONE);  // Beta: Tile.java:700-703 (setDestroyTime(0.5F).setSoundType(SOUND_STONE))
	
	// PressurePlate_wood - properties (Beta: Tile.java:704-707)
	pressurePlate_wood.setDestroyTime(0.5f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:704-707 (setDestroyTime(0.5F).setSoundType(SOUND_WOOD))
	
	// Phase 5: Advanced blocks - properties
	// RecordPlayer - properties (Beta: Tile.java:230)
	recordPlayer.setDestroyTime(2.0f).setSoundType(SOUND_WOOD);  // Beta: Material.wood defaults, setDestroyTime(2.0F) typical for wood
	
	// Pumpkin - properties (Beta: Tile.java:236)
	pumpkin.setDestroyTime(1.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:236 (setDestroyTime(1.0F).setSoundType(SOUND_WOOD))
	
	// LitPumpkin - properties (Beta: Tile.java:249)
	litPumpkin.setDestroyTime(1.0f).setLightEmission(15).setSoundType(SOUND_WOOD);  // Beta: Tile.java:249-252 (setDestroyTime(1.0F), setLightEmission(1.0F*15=15), setSoundType(SOUND_WOOD))
	
	// HellRock - properties (Beta: Tile.java:237)
	hellRock.setDestroyTime(0.4f).setSoundType(SOUND_STONE);  // Beta: Tile.java:237 (setDestroyTime(0.4F).setSoundType(SOUND_STONE))
	
	// HellSand - properties (Beta: Tile.java:238)
	hellSand.setDestroyTime(0.5f).setSoundType(SOUND_SAND);  // Beta: Tile.java:238 (setDestroyTime(0.5F).setSoundType(SOUND_SAND))
	
	// LightGem - properties (Beta: Tile.java:239-243)
	lightGem.setDestroyTime(0.3f).setLightEmission(15).setSoundType(SOUND_GLASS);  // Beta: Tile.java:239-243 (setDestroyTime(0.3F), setLightEmission(1.0F*15=15), setSoundType(SOUND_GLASS))
	
	// PortalTile - properties (Alpha 1.2.6: Block.java:112)
	// Alpha: portal.setHardness(-1.0F).setStepSound(soundGlassFootstep).setLightValue(12.0F / 16.0F)
	// Portal is unbreakable (hardness -1.0), uses glass sound, emits light (12/16 = 0.75 * 15 = 11.25 ≈ 11)
	portalTile.setDestroyTime(-1.0f).setSoundType(SOUND_GLASS).setLightEmission(11);  // Alpha: Block.java:112
	// Portal is non-solid and translucent (handled by TransparentTile base class)
	lightBlock[90] = 0;  // Portal doesn't block light (isTranslucent() should return true)
	translucent[90] = true;  // Portal is translucent
	
	// Torch block - properties (Beta: Tile.java:195)
	// Alpha 1.2.6: Torch doesn't block light (blocksLight() returns false)
	torch.setDestroyTime(0.0f).setSoundType(SOUND_WOOD);  // Beta: Tile.java:195 (setDestroyTime(0.0F).setSoundType(SOUND_WOOD))
	lightBlock[50] = 0;  // Alpha: Torch doesn't block light (blocksLight() returns false)
	translucent[50] = true;  // Torch is translucent for proper lighting
	
	// Fence - properties (Beta: FenceTile.java:27-29)
	// Beta: FenceTile.blocksLight() returns false, so light should pass through
	lightBlock[85] = 0;  // Beta: Fence doesn't block light (blocksLight() returns false)
	translucent[85] = true;  // Fence is translucent for proper lighting
	
	// Fire - properties (Beta: Tile.java:172)
	// Beta: FireTile.blocksLight() returns false, so light should pass through
	// Beta: FireTile.setLightEmission(1.0F) = 15 (full brightness)
	fire.setSoundType(SOUND_WOOD);  // Beta: Tile.java:172 (setSoundType(SOUND_WOOD))
	lightBlock[51] = 0;  // Beta: Fire doesn't block light (blocksLight() returns false)
	translucent[51] = true;  // Fire is translucent for proper lighting
}

// Impl
const jstring Tile::TILE_DESCRIPTION_PREFIX = u"tile.";

Tile::Tile(int_t id, const Material &material) : material(material)
{
	if (tiles.at(id) != nullptr)
		throw std::runtime_error("Slot " + std::to_string(id) + " is already occupied");
	tiles[id] = this;

	this->id = id;
	
	setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	
	// Note: isSolidRender() is virtual, so derived classes can override it
	// This is called during construction, so virtual dispatch should work correctly
	solid[id] = isSolidRender();
	lightBlock[id] = isSolidRender() ? 255 : 0;
	translucent[id] = isTranslucent();
	isEntityTile[id] = false;
}

Tile::Tile(int_t id, int_t tex, const Material &material) : Tile(id, material)
{
	this->tex = tex;
}

Tile &Tile::setLightBlock(int_t lightBlock)
{
	Tile::lightBlock[id] = lightBlock;
	return *this;
}

Tile &Tile::setLightEmission(int_t lightEmission)
{
	Tile::lightEmission[id] = lightEmission;
	return *this;
}

Tile &Tile::setExplodeable(float resistance)
{
	explosionResistance = resistance * 3.0f;
	return *this;
}

Tile &Tile::setExplosionResistance(float resistance)
{
	explosionResistance = resistance;
	return *this;
}

bool Tile::isTranslucent()
{
	return false;
}

bool Tile::isCubeShaped()
{
	return true;
}

Tile::Shape Tile::getRenderShape()
{
	return SHAPE_BLOCK;
}

Tile &Tile::setDestroyTime(float time)
{
	destroySpeed = time;
	// Alpha: setHardness() also sets minimum resistance = hardness * 5.0F (Block.java:184-190)
	// Only set minimum if no explicit resistance was set (explosionResistance still 0)
	if (explosionResistance < time * 5.0f) {
		explosionResistance = time * 5.0f;
	}
	return *this;
}

void Tile::setTicking(bool ticking)
{
	shouldTick[id] = ticking;
}

void Tile::setShape(float x0, float y0, float z0, float x1, float y1, float z1)
{
	xx0 = x0;
	yy0 = y0;
	zz0 = z0;
	xx1 = x1;
	yy1 = y1;
	zz1 = z1;
}

float Tile::getBrightness(LevelSource &level, int_t x, int_t y, int_t z)
{
	return level.getBrightness(x, y, z);
}

bool Tile::isFaceVisible(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	if (face == Facing::DOWN)
		y--;
	if (face == Facing::UP)
		y++;
	if (face == Facing::NORTH)
		z--;
	if (face == Facing::SOUTH)
		z++;
	if (face == Facing::WEST)
		x--;
	if (face == Facing::EAST)
		x++;
	return !level.isSolidTile(x, y, z);
}

bool Tile::shouldRenderFace(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	// Beta: Tile.shouldRenderFace() - coordinates passed are already the neighbor's position
	// Beta 1.2: TileRenderer calls shouldRenderFace(level, x, y, z - 1, Facing::NORTH)
	// So x, y, z are already the neighbor's coordinates, not the current block's
	if (face == Facing::DOWN && yy0 > 0.0)
		return true;
	else if (face == Facing::UP && yy1 < 1.0)
		return true;
	else if (face == Facing::NORTH && zz0 > 0.0)
		return true;
	else if (face == Facing::SOUTH && zz1 < 1.0)
		return true;
	else if (face == Facing::WEST && xx0 > 0.0)
		return true;
	else if (face == Facing::EAST && xx1 < 1.0)
		return true;
	else
	{
		// Beta: x, y, z are already the neighbor's position, so check isSolidTile directly
		// Beta 1.2: return var5 == 5 && this.xx1 < 1.0 ? true : !var1.isSolidTile(var2, var3, var4);
		// Beta: Also check if neighbor is a liquid - liquids are not solid but we should still render faces
		const Material &neighborMat = level.getMaterial(x, y, z);
		if (neighborMat.isLiquid())
			return true;  // Always render faces next to liquids
		return !level.isSolidTile(x, y, z);
	}
}

int_t Tile::getTexture(LevelSource &level, int_t x, int_t y, int_t z, Facing face)
{
	return getTexture(face, level.getData(x, y, z));
}

int_t Tile::getTexture(Facing face, int_t data)
{
	return getTexture(face);
}

int_t Tile::getTexture(Facing face)
{
	return tex;
}

AABB *Tile::getTileAABB(Level &level, int_t x, int_t y, int_t z)
{
	return AABB::newTemp(x + xx0, y + yy0, z + zz0, x + xx1, y + yy1, z + zz1);
}

void Tile::addAABBs(Level &level, int_t x, int_t y, int_t z, AABB &bb, std::vector<AABB *> &aabbList)
{
	AABB *aabb = getAABB(level, x, y, z);
	if (aabb != nullptr && aabb->intersects(bb))
		aabbList.push_back(aabb);
}

AABB *Tile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	return AABB::newTemp(x + xx0, y + yy0, z + zz0, x + xx1, y + yy1, z + zz1);
}

bool Tile::isSolidRender()
{
	return true;
}

bool Tile::mayPick(int_t data, bool canPickLiquid)
{
	return mayPick();
}

bool Tile::mayPick()
{
	return true;
}

// Beta: Tile.mayPlace() - checks if block can be placed (Tile.java:618-621)
bool Tile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	int_t tileId = level.getTile(x, y, z);
	if (tileId == 0)
		return true;
	Tile *tile = Tile::tiles[tileId];
	if (tile == nullptr)
		return false;
	return tile->material.isLiquid();
}

void Tile::tick(Level &level, int_t x, int_t y, int_t z, Random &random)
{

}

void Tile::animateTick(Level &level, int_t x, int_t y, int_t z, Random &random)
{

}

void Tile::destroy(Level &level, int_t x, int_t y, int_t z, int_t data)
{

}

void Tile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{

}

void Tile::addLights(Level &level, int_t x, int_t y, int_t z)
{

}

int_t Tile::getTickDelay()
{
	return 10;
}

void Tile::onPlace(Level &level, int_t x, int_t y, int_t z)
{

}

void Tile::onRemove(Level &level, int_t x, int_t y, int_t z)
{

}

int_t Tile::getResourceCount(Random &random)
{
	return 1;
}

int_t Tile::getResource(int_t data, Random &random)
{
	return id;
}

float Tile::getDestroyProgress(Player &player)
{
	if (destroySpeed < 0.0f)
		return 0.0f;

	// Alpha: blockStrength() formula (Block.java:284-285)
	// If canHarvestBlock() is false: speed = 1.0F / hardness / 100.0F (very slow)
	// If canHarvestBlock() is true: speed = getCurrentPlayerStrVsBlock() / hardness / 30.0F
	bool canHarvest = player.canDestroy(*this);
	float speed = player.getDestroySpeed(*this);
	
	#ifdef DEBUG_BREAK_SPEED
	std::cout << "Break speed debug: blockId=" << id << " hardness=" << destroySpeed 
	          << " toolId=" << (player.inventory.getCurrentItem() ? player.inventory.getCurrentItem()->itemID : 0)
	          << " effective=" << (speed > 1.0f ? "yes" : "no")
	          << " speed=" << speed << " canHarvest=" << (canHarvest ? "yes" : "no") << std::endl;
	#endif
	
	if (!canHarvest)
		return 1.0f / destroySpeed / 100.0f;
	
	return speed / destroySpeed / 30.0f;
}

void Tile::spawnResources(Level &level, int_t x, int_t y, int_t z, int_t data)
{
	spawnResources(level, x, y, z, data, 1.0f);
}

void Tile::spawnResources(Level &level, int_t x, int_t y, int_t z, int_t data, float chance)
{
	// Alpha: Block.harvestBlock() spawns EntityItem (Block.java:296-310)
	int_t count = getResourceCount(level.random);
	
	for (int_t i = 0; i < count; ++i)
	{
		if (level.random.nextFloat() <= chance)  // Alpha: rand.nextFloat() <= var6 (Block.java:297)
		{
			int_t dropId = getResource(data, level.random);
			if (dropId > 0)
			{
				// Alpha: Spawn position randomization (Block.java:300-303)
				float spread = 0.7f;  // Alpha: var10 = 0.7F
				double offsetX = (double)(level.random.nextFloat() * spread) + (double)(1.0f - spread) * 0.5;
				double offsetY = (double)(level.random.nextFloat() * spread) + (double)(1.0f - spread) * 0.5;
				double offsetZ = (double)(level.random.nextFloat() * spread) + (double)(1.0f - spread) * 0.5;
				
				// Alpha: Create EntityItem and add to world (Block.java:304-306)
				// dropId can be blockID (0-255) or item shiftedIndex (256+)
				// In Alpha, ItemStack can be constructed from blockID directly
				// For now, use dropId as-is (blocks: 0-255, items: 256+)
				// TODO: ItemStack should handle block IDs properly (blocks don't have shiftedIndex)
				int_t itemId = dropId;  // Use dropId directly (blockID for blocks, shiftedIndex for items)
				if (dropId < 256)
				{
					// Block drop - use blockID directly (ItemStack will need to handle this)
					itemId = dropId;
				}
				ItemStack dropStack(itemId, 1);
				double entityX = (double)x + offsetX;
				double entityY = (double)y + offsetY;
				double entityZ = (double)z + offsetZ;
				auto itemEntity = std::make_shared<EntityItem>(level, entityX, entityY, entityZ, dropStack);
				
				level.addEntity(itemEntity);  // Alpha: entityJoinedWorld(var17)
			}
		}
	}
}

int_t Tile::getSpawnResourcesAuxValue(int_t data)
{
	return 0;
}

HitResult Tile::clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to)
{
	updateShape(level, x, y, z);

	Vec3 *localFrom = from.add(-x, -y, -z);
	Vec3 *localTo = to.add(-x, -y, -z);
	Vec3 *cxx0 = localFrom->clipX(*localTo, xx0);
	Vec3 *cxx1 = localFrom->clipX(*localTo, xx1);
	Vec3 *cyy0 = localFrom->clipY(*localTo, yy0);
	Vec3 *cyy1 = localFrom->clipY(*localTo, yy1);
	Vec3 *czz0 = localFrom->clipZ(*localTo, zz0);
	Vec3 *czz1 = localFrom->clipZ(*localTo, zz1);

	// Check bounds validity - clipX/Y/Z may return null, and containsX/Y/Z handles null safely
	if (cxx0 != nullptr && !containsX(cxx0)) cxx0 = nullptr;
	if (cxx1 != nullptr && !containsX(cxx1)) cxx1 = nullptr;
	if (cyy0 != nullptr && !containsY(cyy0)) cyy0 = nullptr;
	if (cyy1 != nullptr && !containsY(cyy1)) cyy1 = nullptr;
	if (czz0 != nullptr && !containsZ(czz0)) czz0 = nullptr;
	if (czz1 != nullptr && !containsZ(czz1)) czz1 = nullptr;

	Vec3 *pick = nullptr;
	if (cxx0 != nullptr && (pick == nullptr || localFrom->distanceTo(*cxx0) < localFrom->distanceTo(*pick))) pick = cxx0;
	if (cxx1 != nullptr && (pick == nullptr || localFrom->distanceTo(*cxx1) < localFrom->distanceTo(*pick))) pick = cxx1;
	if (cyy0 != nullptr && (pick == nullptr || localFrom->distanceTo(*cyy0) < localFrom->distanceTo(*pick))) pick = cyy0;
	if (cyy1 != nullptr && (pick == nullptr || localFrom->distanceTo(*cyy1) < localFrom->distanceTo(*pick))) pick = cyy1;
	if (czz0 != nullptr && (pick == nullptr || localFrom->distanceTo(*czz0) < localFrom->distanceTo(*pick))) pick = czz0;
	if (czz1 != nullptr && (pick == nullptr || localFrom->distanceTo(*czz1) < localFrom->distanceTo(*pick))) pick = czz1;
	if (pick == nullptr)
		return HitResult();

	Facing face = Facing::NONE;
	if (pick == cxx0)
		face = Facing::WEST;
	else if (pick == cxx1)
		face = Facing::EAST;
	else if (pick == cyy0)
		face = Facing::DOWN;
	else if (pick == cyy1)
		face = Facing::UP;
	else if (pick == czz0)
		face = Facing::NORTH;
	else if (pick == czz1)
		face = Facing::SOUTH;

	return HitResult(x, y, z, face, *pick->add(x, y, z));
}

bool Tile::containsX(Vec3 *vec)
{
	if (vec == nullptr)
		return false;
	return vec->y >= yy0 && vec->y <= yy1 && vec->z >= zz0 && vec->z <= zz1;
}

bool Tile::containsY(Vec3 *vec)
{
	if (vec == nullptr)
		return false;
	return vec->x >= xx0 && vec->x <= xx1 && vec->z >= zz0 && vec->z <= zz1;
}

bool Tile::containsZ(Vec3 *vec)
{
	if (vec == nullptr)
		return false;
	return vec->x >= xx0 && vec->x <= xx1 && vec->y >= yy0 && vec->y <= yy1;
}

int_t Tile::getRenderLayer()
{
	return 0;
}

void Tile::stepOn(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{

}

void Tile::setPlacedOnFace(Level &level, int_t x, int_t y, int_t z, Facing face)
{

}

void Tile::setPlacedBy(Level &level, int_t x, int_t y, int_t z, Mob &mob)
{
	// Beta: Default implementation does nothing (Tile.java doesn't have default implementation)
}

void Tile::prepareRender(Level &level, int_t x, int_t y, int_t z)
{

}

void Tile::attack(Level &level, int_t x, int_t y, int_t z, Player &player)
{

}

// Beta: Tile.use() - called when player right-clicks block (Tile.java:623-625)
bool Tile::use(Level &level, int_t x, int_t y, int_t z, Player &player)
{
	return false;  // Beta: return false (default behavior)
}

void Tile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{

}

int_t Tile::getColor(LevelSource &level, int_t x, int_t y, int_t z)
{
	return 0xFFFFFF;
}

void Tile::entityInside(Level &level, int_t x, int_t y, int_t z, Entity &entity)
{

}

void Tile::updateDefaultShape()
{

}

void Tile::playerDestroy(Level &level, int_t x, int_t y, int_t z, int_t data)
{
	spawnResources(level, x, y, z, data);
}
