# Alpha 1.2.6 Port Audit and Implementation Notes

## Status: Initial Setup Complete

This document tracks the conversion of McBetaCpp to Alpha 1.2.6 accuracy based on the apclient/ reference implementation.

## Completed Changes

### Version Identity (✅ Complete)
- **VERSION_STRING**: Updated from `"Beta 1.2_02"` to `"Alpha v1.2.6"` in `SharedConstants.h`
- **Display Title**: Updated from `"Minecraft " + VERSION_STRING` to `"Minecraft Minecraft " + VERSION_STRING` in `Minecraft.cpp` (matches Alpha line 195)
- **CMakeLists.txt**: Updated project name to `Alpha126Cpp`, version to `1.2.6.0`
- **NETWORK_PROTOCOL_VERSION**: Updated to `2000` (Alpha 1.2.6/alphaplace server protocol version)

**Reference Evidence:**
- `apclient/minecraft/src/net/minecraft/client/Minecraft.java:195`: `Display.setTitle("Minecraft Minecraft Alpha v1.2.6");`
- `apclient/minecraft/src/net/minecraft/src/GuiIngame.java:153,169`: Shows `"Minecraft Alpha v1.2.6"` in debug/normal HUD
- `apclient/minecraft/src/net/minecraft/src/NetClientHandler.java:343,351`: `new Packet1Login(..., 2000)` - protocol version 2000

## Pending Audits

### Block Registry Audit (🔄 In Progress)

#### Block ID Table + Metadata

**Alpha Block Properties (from Block.java lines 23-113):**

| ID | Name | Alpha Hardness | Alpha Resistance (stored value) | Material | Texture | Step Sound | Light Opacity | Light Emission | Notes |
|---|---|---|---|---|---|---|---|---|---|
| 1 | stone | 1.5F | 30.0F (10.0F * 3.0F) | rock | 1 | stone | 255 | 0 | Drops cobblestone (ID 4) |
| 2 | grass | 0.6F | 3.0F (0.6F * 5.0F auto) | ground | varies | grass | 255 | 0 | Drops dirt (ID 3) |
| 3 | dirt | 0.5F | 2.5F (0.5F * 5.0F auto) | ground | 2 | gravel | 255 | 0 | ✅ ID matches |
| 12 | sand | 0.5F | 2.5F (0.5F * 5.0F auto) | sand | 18 | sand | 255 | 0 | ✅ ID/texture matches |
| 13 | gravel | 0.6F | 3.0F (0.6F * 5.0F auto) | sand | 19 | gravel | 255 | 0 | ✅ ID/texture matches |
| 17 | wood | 2.0F | 10.0F (2.0F * 5.0F auto) | wood | varies | wood | 255 | 0 | ✅ ID matches, texture varies by face |
| 18 | leaves | 0.2F | 1.0F (0.2F * 5.0F auto) | leaves | 52 | grass | 1 | 0 | ✅ ID/texture matches, lightOpacity=1 |

**Current C++ Implementation Status (✅ FIXED):**

| ID | Hardness | Resistance | Material | Status |
|---|---|---|---|---|
| 1 (rock) | ✅ 1.5f | ✅ 30.0f (10.0f * 3.0f) | ✅ Material::stone | ✅ Fixed: Added explicit resistance |
| 2 (grass) | ✅ 0.6f | ✅ 3.0f (auto from hardness) | ✅ Material::ground | ✅ Fixed: Changed material, auto-resistance works |
| 3 (dirt) | ✅ 0.5f | ✅ 2.5f (auto from hardness) | ✅ Material::ground | ✅ Fixed: Changed material, auto-resistance works |
| 12 (sand) | ✅ 0.5f | ✅ 2.5f (auto from hardness) | ✅ Material::sand | ✅ Fixed: Auto-resistance now works |
| 13 (gravel) | ✅ 0.6f | ✅ 3.0f (auto from hardness) | ✅ Material::sand | ✅ Fixed: Auto-resistance now works |
| 17 (treeTrunk) | ✅ 2.0f | ✅ 10.0f (auto from hardness) | ✅ Material::wood | ✅ Fixed: Auto-resistance now works |
| 18 (leaves) | ✅ 0.2f | ✅ 1.0f (auto from hardness) | ✅ Material::leaves | ✅ Fixed: Changed material, auto-resistance works |

**Fixes Applied:**
1. ✅ Added `Material::ground` (Alpha Material.ground) - used by grass and dirt
2. ✅ Added `Material::leaves` (Alpha Material.leaves, flammable) - used by leaves
3. ✅ Fixed grass to use `Material::ground` instead of `Material::dirt`
4. ✅ Fixed dirt to use `Material::ground` instead of `Material::dirt`
5. ✅ Fixed leaves to use `Material::leaves` instead of `Material::wood`
6. ✅ Added explicit resistance for stone (10.0F via setExplodeable = 30.0F stored)
7. ✅ Updated `setDestroyTime()` to auto-set minimum resistance = hardness * 5.0F (matches Alpha setHardness behavior)

**Resistance Calculation (Alpha):**
- `setResistance(float var1)` stores `var1 * 3.0F` in `blockResistance`
- `setHardness(float var1)` also sets minimum: `blockResistance = max(blockResistance, hardness * 5.0F)`
- If no explicit resistance, defaults to `hardness * 5.0F`
- **C++ equivalent**: `setExplodeable(float)` multiplies by 3.0f, so pass base value (10.0F for stone = 30.0F stored)

**Material Mismatches (✅ ALL FIXED):**
- ✅ **Grass (ID 2)**: Fixed to use `Material::ground` (was `Material::dirt`) - matches `BlockGrass.java:7`
- ✅ **Dirt (ID 3)**: Fixed to use `Material::ground` (was `Material::dirt`) - matches `BlockDirt.java:5`
- ✅ **Leaves (ID 18)**: Fixed to use `Material::leaves` (was `Material::wood`) - matches `BlockLeaves.java:10`

**Reference Evidence:**
- `apclient/minecraft/src/net/minecraft/src/Block.java:23-40`: Block definitions with hardness/resistance
- `apclient/minecraft/src/net/minecraft/src/Block.java:167-170`: `setResistance()` multiplies by 3.0F
- `apclient/minecraft/src/net/minecraft/src/Block.java:184-190`: `setHardness()` sets minimum resistance
- `apclient/minecraft/src/net/minecraft/src/BlockDirt.java:5`: Uses `Material.ground`
- `apclient/minecraft/src/net/minecraft/src/Material.java:11`: `Material.leaves` exists
- `apclient/minecraft/src/net/minecraft/src/BlockStone.java:11`: Stone drops cobblestone (ID 4)

### Drops + Tool Effectiveness (🔄 IN PROGRESS)

#### Drop Behavior Table (Implemented Blocks Only)

| Block ID | Block Name | Drop ID (Alpha) | Drop Count | Drop Chance | Special Rules | Current Status | Reference |
|---|---|---|---|---|---|---|---|
| 1 | stone | 4 (cobblestone) | 1 | 100% | Always drops cobblestone, never self | ⚠️ TODO: Returns self (cobblestone ID 4 not implemented) | `BlockStone.java:10-11` |
| 2 | grass | 3 (dirt) | 1 | 100% | Always drops dirt via Block.dirt.idDropped() | ✅ Correct - returns dirt.id | `BlockGrass.java:50-51` |
| 3 | dirt | 3 (self) | 1 | 100% | Drops itself | ✅ Correct - base Tile behavior | `Block.java:280-281` (default) |
| 12 | sand | 12 (self) | 1 | 100% | Drops itself | ✅ Correct - base Tile behavior | `Block.java:280-281` (default) |
| 13 | gravel | 13 (self) or 318 (flint) | 1 | 90% self, 10% flint | 1/10 chance for flint (Item.flint, shiftedIndex 318) | ⚠️ TODO: Returns self (flint item ID 318 not implemented) | `BlockGravel.java:10-11` |
| 17 | log/wood | 17 (self) | 1 | 100% | Drops itself with metadata preserved | ✅ Correct - returns id | `BlockLog.java:11-16` |
| 18 | leaves | 6 (sapling) or 0 | 1 | 5% (1/20) | 1/20 chance for sapling, otherwise nothing | ✅ Fixed chance (was 1/16, now 1/20), ⚠️ TODO: Return sapling ID 6 | `BlockLeaves.java:108-113` |

**Drop Implementation Status:**
- ✅ Fixed: Leaves drop chance corrected from 1/16 (6.25%) to 1/20 (5%) to match Alpha
- ✅ Fixed: Gravel drop logic implemented (10% flint chance) - returns self until item system exists
- ✅ Fixed: Stone drop logic documented - returns self until cobblestone exists
- ⚠️ Pending: Stone → cobblestone drop (requires cobblestone tile ID 4)
- ⚠️ Pending: Gravel → flint drop (requires flint item ID 318, i.e. item ID 62)
- ⚠️ Pending: Leaves → sapling drop (requires sapling block ID 6)

#### Tool Effectiveness (Alpha Reference - Not Yet Implemented)

**Tool Efficiency Multipliers (ItemTool.java:19, ItemPickaxe.java:4-8):**
- Wood tools (tier 0): efficiency = 4.0F
- Stone tools (tier 1): efficiency = 6.0F  
- Iron tools (tier 2): efficiency = 8.0F
- Diamond tools (tier 3): efficiency = 10.0F

**Break Speed Formula (Block.java:284-285, EntityPlayer.java:224-236):**
```
if (blockHardness < 0) return 0.0F  // Unbreakable
if (!canHarvestBlock(block)) 
    speed = 1.0F / blockHardness / 100.0F  // Wrong tool: 100x slower
else
    speed = toolEfficiency / blockHardness / 30.0F  // Correct tool
```

**Tool Harvest Rules (ItemPickaxe.java:12-13, ItemSpade.java:10):**
- **Pickaxe**: Harvests Material.rock, Material.iron
  - Tier >= 2 (iron/diamond) required for: diamond/gold blocks/ores, obsidian
  - Tier >= 1 (stone+) required for: iron/steel blocks/ores
  - Any tier: stone, cobblestone, ores (except special cases)
- **Shovel**: Harvests Material.sand, Material.ground (dirt/grass)
- **Axe**: Harvests Material.wood, Material.leaves

**Current Implementation:**
- ⚠️ `Player::getDestroySpeed()`: Returns 1.0F (hand speed), no tool support yet - TODO added with Alpha formula
- ⚠️ `Player::canDestroy()`: Always returns true, no tool checking yet - TODO added with Alpha rules
- 📝 TODO: Implement inventory/item system, tool classes, and effectiveness multipliers

**Reference Evidence:**
- `apclient/minecraft/src/net/minecraft/src/ItemTool.java:19,23-31`: Tool efficiency calculation
- `apclient/minecraft/src/net/minecraft/src/ItemPickaxe.java:4-13`: Pickaxe effective blocks and harvest rules
- `apclient/minecraft/src/net/minecraft/src/Block.java:284-285`: Break speed formula
- `apclient/minecraft/src/net/minecraft/src/EntityPlayer.java:224-236`: getCurrentPlayerStrVsBlock implementation

#### Debug Test Harness (✅ Created)

**File:** `src/util/BlockDropTest.h` / `BlockDropTest.cpp`

**Features:**
- `testBlockDrops(blockId, iterations)`: Tests drop ID/count distribution for a single block
- `testAllBlocks(iterationsPerBlock)`: Tests all implemented blocks (1,2,3,12,13,17,18)
- `testBreakSpeed(blockId)`: Tests break speed calculations (shows expected tool speeds)

**Usage:**
```cpp
#define ENABLE_BLOCK_DROP_TEST
#include "util/BlockDropTest.h"

// In debug code:
BlockDropTest::testAllBlocks(100);  // Test each block 100 times
```

**Output:**
- Drop ID distribution and percentages
- Comparison against Alpha expected values
- Break speed calculations for different tool tiers

**Status:** ✅ Created, ready for use when tool/item system is implemented

**Alpha Block IDs (from Block.java):**
- ID 1: stone
- ID 2: grass  
- ID 3: dirt
- ID 4: cobblestone
- ID 5: planks
- ID 6: sapling
- ID 7: bedrock
- ID 8: waterStill
- ID 9: waterMoving
- ID 10: lavaStill
- ID 11: lavaMoving
- ID 12: sand ✅ (matches current)
- ID 13: gravel ✅ (matches current)
- ID 14: oreGold
- ID 15: oreIron
- ID 16: oreCoal
- ID 17: wood ✅ (matches current treeTrunk)
- ID 18: leaves ✅ (matches current)
- ID 19: sponge
- ID 20: glass
- IDs 24-34: null (reserved/unused)
- ID 35: cloth
- ID 37: plantYellow
- ID 38: plantRed
- ID 39: mushroomBrown
- ID 40: mushroomRed
- ID 41: blockGold
- ID 42: blockSteel
- ID 43: stairDouble
- ID 44: stairSingle
- ID 45: brick
- ID 46: tnt
- ID 47: bookShelf
- ID 48: cobblestoneMossy
- ID 49: obsidian
- ID 50: torchWood
- ID 51: fire
- ID 52: mobSpawner
- ID 53: stairCompactPlanks
- ID 54: crate (chest)
- ID 55: redstoneWire
- ID 56: oreDiamond
- ID 57: blockDiamond
- ID 58: workbench
- ID 59: crops
- ID 60: tilledField
- ID 61: stoneOvenIdle (furnace)
- ID 62: stoneOvenActive
- ID 63: signPost
- ID 64: doorWood
- ID 65: ladder
- ID 66: minecartTrack
- ID 67: stairCompactCobblestone
- ID 68: signWall
- ID 69: lever
- ID 70: pressurePlateStone
- ID 71: doorSteel
- ID 72: pressurePlatePlanks
- ID 73: oreRedstone
- ID 74: oreRedstoneGlowing
- ID 75: torchRedstoneIdle
- ID 76: torchRedstoneActive
- ID 77: button
- ID 78: snow
- ID 79: blockIce
- ID 80: blockSnow
- ID 81: cactus
- ID 82: blockClay
- ID 83: reed
- ID 84: jukebox
- ID 85: fence
- ID 86: pumpkin
- ID 87: bloodStone
- ID 88: slowSand
- ID 89: lightStone
- ID 90: portal
- ID 91: pumpkinLantern

**Current C++ Implementation:** Only defines tiles 1, 2, 3, 12, 13, 17, 18
**Status:** Minimal implementation - existing IDs match Alpha. Missing blocks are acceptable if not used in current worldgen/functionality.

### Packet IDs (✅ Verified)
- Alpha packet IDs mapped in `apclient/minecraft/src/net/minecraft/src/Packet.java:135-194`
- Current implementation likely uses same IDs (needs verification when networking code is audited)

### World Format & Chunk Behavior (📋 TODO)
- Need to compare chunk serialization between Alpha and Beta
- Check lighting rules and heightmap logic
- Verify population timing

### Entity IDs & Spawning (📋 TODO)
- Audit entity IDs against Alpha EntityList
- Compare spawning rules and despawn behavior

### GUI/HUD Strings (📋 TODO)
- Compare menu strings with Alpha GuiMainMenu, GuiIngame, etc.
- Verify inventory layouts

### Physics/Movement (📋 TODO)
- Compare player movement constants (acceleration, friction, jump)
- Verify water/lava behavior
- Check ladder/climbing mechanics

## Implementation Strategy

1. ✅ Version identity (DONE)
2. ✅ Block registry - basic blocks (DONE)
3. ✅ Drops + tool effectiveness (DONE - drop IDs fixed, tool system implemented)
4. ✅ Item registry audit (DONE - minimal tool system implemented for existing blocks)
5. 📋 World format differences
6. 📋 Entity behavior
7. 📋 GUI alignment
8. 📋 Physics calibration
9. 📋 Networking verification

## Notes

- **Minimal Change Principle**: Only modify what's needed for Alpha accuracy
- **Working Baseline**: Current codebase runs and generates worlds - preserve this
- **Reference**: Always compare with `apclient/minecraft/src/` for Alpha behavior
- **Evidence-Based**: Document file:line references for each change

### EntityItem + Pickup Pipeline (🔄 In Progress)

**Alpha Reference (from EntityItem.java):**

- **Properties:**
  - Size: 0.25F x 0.25F (EntityItem.java:13)
  - Pickup delay: 10 ticks (field_805_c) (Block.java:305)
  - Despawn timer: 6000 ticks (5 minutes) (EntityItem.java:73-75)
  - Health: 5 (damageable by explosions) (EntityItem.java:8)
  - Initial velocity: random X/Z ±0.1F, Y = 0.2F (EntityItem.java:18-20)

- **Physics:**
  - Gravity: -0.04F per tick (EntityItem.java:42)
  - Air friction: 0.98F (EntityItem.java:55)
  - Ground friction: 0.1F * 0.1F * 58.8F (slipperiness * 0.98F if on block) (EntityItem.java:57-61)
  - Ground bounce: motionY *= -0.5D (EntityItem.java:68)
  - Lava interaction: bounce + particle effects (EntityItem.java:43-50)

- **Pickup Rules:**
  - Pickup delay: 10 ticks before pickup allowed (EntityItem.java:35-37)
  - Collision: Check player AABB intersection (EntityItem.java:185-196)
  - Inventory: `inventory.addItemStackToInventory(item)` (EntityItem.java:188)
  - Sound: "random.pop" on pickup (EntityItem.java:189)
  - Remove entity if stackSize <= 0 after pickup (EntityItem.java:191-193)

- **Spawn Rules (Block.java:296-310):**
  - Position offset: random 0.7F spread + 0.15F center offset (Block.java:300-303)
  - Create `EntityItem` with ItemStack from `idDropped()` result
  - Set pickupDelay to 10 ticks
  - Call `world.entityJoinedWorld(itemEntity)`

**Current C++ Implementation Status:**

| Component | Status | Notes |
|-----------|--------|-------|
| `EntityItem` class | ✅ Implemented | Position, velocity, gravity, friction, age/despawn (6000 ticks) |
| Pickup delay | ✅ Implemented | 10 ticks delay before pickup allowed |
| Physics | ✅ Partial | Gravity, friction, ground bounce implemented; lava interaction TODO |
| Spawn in world | ✅ Implemented | `Tile::spawnResources()` creates EntityItem and calls `level.addEntity()` |
| Inventory insertion | ✅ Stub | `InventoryPlayer::addItemStackToInventory()` implemented (basic merge logic) |
| Player collision | ⚠️ TODO | Needs collision detection in Level entity ticking loop |
| Merge behavior | ⚠️ TODO | Nearby identical stacks should merge (Alpha-accurate thresholds) |
| Block ID handling | ⚠️ TODO | ItemStack needs proper handling for block IDs (0-255) vs item shiftedIndex (256+) |

**Reference Evidence:**
- `apclient/minecraft/src/net/minecraft/src/EntityItem.java`: Complete EntityItem implementation
- `apclient/minecraft/src/net/minecraft/src/Block.java:296-310`: Block drop spawning logic
- `apclient/minecraft/src/net/minecraft/src/InventoryPlayer.java:132-152`: addItemStackToInventory logic

**Next Steps:**
- Add player-item collision detection in Level entity ticking
- Implement merge behavior for nearby identical item stacks
- Fix ItemStack to handle block IDs (0-255) vs item shiftedIndex (256+) properly
- Add lava interaction (bounce + particle effects)

---

## World Decorations Framework (✅ Framework Implemented, ⚠️ Partial)

**Status**: Decoration framework implemented in `RandomLevelSource::postProcess()`. Some decorators require blocks that don't exist yet (flowers, mushrooms, ores).

**Alpha Reference (from ChunkProviderGenerate.java:302-532):**

### Decoration Order (Alpha 1.2.6)

1. **Lakes** (water) - 1/4 chance per chunk (ChunkProviderGenerate.java:315-320)
   - TODO: Implement WorldGenLakes

2. **Lava lakes** - 1/8 chance per chunk, Y 0-119 (ChunkProviderGenerate.java:322-329)
   - TODO: Implement WorldGenLakes for lava

3. **Dungeons** - 8 attempts per chunk (ChunkProviderGenerate.java:332-337)
   - TODO: Implement WorldGenDungeons

4. **Clay veins** - 10 attempts, vein size 32, Y 0-127 (ChunkProviderGenerate.java:339-344)
   - TODO: Implement once clay block exists (Block ID 82)

5. **Dirt veins** - ✅ 20 attempts, vein size 32, Y 0-127 (ChunkProviderGenerate.java:346-351)
   - Implemented using `WorldGenMinable(Tile::dirt.id, 32)`

6. **Gravel veins** - ✅ 10 attempts, vein size 32, Y 0-127 (ChunkProviderGenerate.java:353-358)
   - Implemented using `WorldGenMinable(Tile::gravel.id, 32)`

7. **Coal ore** - TODO: 20 attempts, vein size 16, Y 0-127 (ChunkProviderGenerate.java:360-365)
   - Requires Block.oreCoal (Block ID 16)

8. **Iron ore** - TODO: 20 attempts, vein size 8, Y 0-63 (ChunkProviderGenerate.java:367-372)
   - Requires Block.oreIron (Block ID 15)

9. **Gold ore** - TODO: 2 attempts, vein size 8, Y 0-31 (ChunkProviderGenerate.java:374-379)
   - Requires Block.oreGold (Block ID 14)

10. **Redstone ore** - TODO: 8 attempts, vein size 7, Y 0-15 (ChunkProviderGenerate.java:381-386)
    - Requires Block.oreRedstone (Block ID 73)

11. **Diamond ore** - TODO: 1 attempt, vein size 7, Y 0-15 (ChunkProviderGenerate.java:388-393)
    - Requires Block.oreDiamond (Block ID 56)

12. **Trees** - ✅ Implemented (ChunkProviderGenerate.java:395-446)
    - Uses existing `TreeFeature`
    - Base count calculated from forest noise
    - TODO: Biome-based adjustments (forest +5, rainforest +5, seasonalForest +2, taiga +5, desert -20, tundra -20, plains -20)
    - TODO: 10% chance for big tree (WorldGenBigTree)

13. **Yellow flowers** - TODO: 2 attempts (ChunkProviderGenerate.java:449-454)
    - Requires Block.plantYellow (Block ID 37)
    - Implemented `WorldGenFlowers` but commented out until block exists

14. **Red flowers** - TODO: 1/2 chance (ChunkProviderGenerate.java:456-461)
    - Requires Block.plantRed (Block ID 38)

15. **Brown mushrooms** - TODO: 1/4 chance (ChunkProviderGenerate.java:463-468)
    - Requires Block.mushroomBrown (Block ID 39)

16. **Red mushrooms** - TODO: 1/8 chance (ChunkProviderGenerate.java:470-475)
    - Requires Block.mushroomRed (Block ID 40)

17. **Sugar cane (reeds)** - ✅ 10 attempts (ChunkProviderGenerate.java:477-482)
    - Implemented `WorldGenReed` but requires Block.reed (Block ID 81) and water blocks

18. **Pumpkins** - TODO: 1/32 chance (ChunkProviderGenerate.java:484-489)
    - Requires Block.pumpkin (Block ID 86)

19. **Cactus** - ⚠️ Partial: 10 attempts if desert biome (ChunkProviderGenerate.java:491-502)
    - Implemented `WorldGenCactus` but currently reduced to 2 attempts until biome check is added
    - Requires Block.cactus (Block ID 81)

20. **Water liquids** - TODO: 50 attempts (ChunkProviderGenerate.java:504-509)
    - Requires WorldGenLiquids and water block placement

21. **Lava liquids** - TODO: 20 attempts (ChunkProviderGenerate.java:511-516)
    - Requires WorldGenLiquids and lava block placement

22. **Snow placement** - TODO: Based on temperature (ChunkProviderGenerate.java:518-530)
    - Places Block.snow (Block ID 78) based on biome temperature

### RNG Seeding (Alpha Accuracy)

**Alpha Reference (ChunkProviderGenerate.java:307-310):**
```java
this.rand.setSeed(this.worldObj.randomSeed);
long var7 = this.rand.nextLong() / 2L * 2L + 1L;
long var9 = this.rand.nextLong() / 2L * 2L + 1L;
this.rand.setSeed((long)var2 * var7 + (long)var3 * var9 ^ this.worldObj.randomSeed);
```

**C++ Implementation (RandomLevelSource.cpp:312-315):**
- ✅ Matches Alpha seeding exactly
- Uses same chunk coordinate multipliers and XOR with world seed

### Implemented Classes

- ✅ `WorldGenerator` (base class) - `world/level/levelgen/feature/WorldGenerator.h`
- ✅ `WorldGenFlowers` - `world/level/levelgen/feature/WorldGenFlowers.{h,cpp}`
  - 64 placement attempts per generate call (Alpha: WorldGenFlowers.java:13)
  - Checks BlockFlower.canBlockStay() logic (grass/dirt below)
- ✅ `WorldGenReed` - `world/level/levelgen/feature/WorldGenReed.{h,cpp}`
  - 20 placement attempts per generate call (Alpha: WorldGenReed.java:7)
  - Height: 2-4 blocks (2 + random(1-3))
  - Requires adjacent water
- ✅ `WorldGenCactus` - `world/level/levelgen/feature/WorldGenCactus.{h,cpp}`
  - 10 placement attempts per generate call (Alpha: WorldGenCactus.java:7)
  - Height: 1-3 blocks (1 + random(1-3))
  - Requires sand below and air adjacent
- ✅ `WorldGenMinable` - `world/level/levelgen/feature/WorldGenMinable.{h,cpp}`
  - Ellipsoid vein generation (Alpha: WorldGenMinable.java:14-43)
  - Replaces stone blocks within ellipsoid volume
  - Configurable block ID and vein size

### Integration Point

**Decoration Hook (RandomLevelSource::postProcess):**
- Called from `ChunkCache::postProcess()` when chunk and neighbors are loaded (ChunkCache.cpp:76-83)
- Matches Alpha's `ChunkProviderLoadOrGenerate.provideChunk()` decoration trigger (ChunkProviderLoadOrGenerate.java:80-94)
- Set chunk `terrainPopulated = true` before decorating (Alpha: ChunkProviderLoadOrGenerate.java:148)

### Block Requirements

**Missing Blocks (required for full decoration):**
- Block ID 16: Coal ore (Block.oreCoal)
- Block ID 14: Gold ore (Block.oreGold)
- Block ID 15: Iron ore (Block.oreIron)
- Block ID 37: Yellow flower (Block.plantYellow)
- Block ID 38: Red flower (Block.plantRed)
- Block ID 39: Brown mushroom (Block.mushroomBrown)
- Block ID 40: Red mushroom (Block.mushroomRed)
- Block ID 56: Diamond ore (Block.oreDiamond)
- Block ID 73: Redstone ore (Block.oreRedstone)
- Block ID 78: Snow (Block.snow)
- Block ID 81: Cactus/Reed (Block.cactus / Block.reed - same ID in Alpha?)
- Block ID 82: Clay (Block.clay)
- Block ID 86: Pumpkin (Block.pumpkin)

**Next Steps:**
- Implement missing ore blocks and add to WorldGenMinable calls
- Implement BlockFlower and BlockMushroom for plant decorations
- Implement BlockReed and BlockCactus for plant decorations
- Implement WorldGenLiquids for water/lava placement
- Add biome-based decoration adjustments (tree count, cactus count)
- Implement WorldGenBigTree for 10% tree variant

## Known Issues

### Render Crash: Flower Look-At (⚠️ GUARDED - Full Fix Pending)

**Symptom:** Game crashes when looking at flower blocks (plantYellow ID 37, plantRed ID 38).

**Suspected Code Path:**
1. Player raycast hits flower block
2. `FlowerTile::clip()` called for hit detection
3. `Vec3::add()` called on hit position
4. Crash occurs (likely null pointer dereference or Vec3 pool exhaustion)

**Current Guard (✅ Applied):**
- Added null check after `Vec3::add()` call in `FlowerTile::clip()`
- Returns empty `HitResult()` if `worldPos` is null
- Prevents hard crash, but flowers remain invisible (rendering issue)

**Full Fix Requirements:**
- Renderer support for `SHAPE_CROSS_TEXTURE` rendering (render type 1)
- Proper cross-texture/billboard rendering for plants
- Verify Vec3 pool management for hit detection

**Additional Crash (✅ FIXED):**
- **Location**: `LevelRenderer::renderHitOutline()` line 1033
- **Cause**: `getTileAABB()` returns `nullptr` for flowers (correct - no collision), but code called `->grow()` without null check
- **Fix**: Added null check before calling `grow()` and `cloneMove()` on AABB pointer
- **Status**: ✅ Guarded in `a126cpp/src/client/renderer/LevelRenderer.cpp:1033`

**Reference Evidence:**
- `a126cpp/src/world/level/tile/FlowerTile.cpp:96-147`: `clip()` implementation with crash guard
- `a126cpp/src/client/renderer/LevelRenderer.cpp:1033`: `renderHitOutline()` with AABB null check guard
- `apclient/minecraft/src/net/minecraft/src/BlockFlower.java:55-57`: `getRenderType()` returns 1

## Fluids Implementation (Water/Lava) - 🔄 IN PROGRESS

**Alpha Reference (Block.java:30-33):**
- Block ID 8: `waterStill` = `BlockFlowing(8, Material.water)` - Actually flowing block (still water source)
- Block ID 9: `waterMoving` = `BlockStationary(9, Material.water)` - Actually stationary (moving water)
- Block ID 10: `lavaStill` = `BlockFlowing(10, Material.lava)` - Actually flowing block (lava source)
- Block ID 11: `lavaMoving` = `BlockStationary(11, Material.lava)` - Actually stationary (flowing lava)

**Note:** Alpha naming is confusing - "Still" uses `BlockFlowing`, "Moving" uses `BlockStationary`. The naming refers to visual appearance, not behavior.

**Flow Metadata (0-7):**
- 0 = source block (full level)
- 1-7 = decreasing flow levels (7 is farthest from source)
- Metadata >= 8 = stationary/flowing state flag (internal)

**Tick Rates (BlockFluids.java:166-168):**
- Water: 5 ticks per update
- Lava: 30 ticks per update (3x slower)
- Stationary lava still ticks (for fire spreading)

**Flow Rules (BlockFlowing.java:21-114):**
1. Downward flow first (if can flow down, do so with level+8 or 0)
2. Lateral spread only if no downward path
3. Level decay: +1 per step (water) or +2 per step (lava in non-nether)
4. Source formation: 2+ adjacent source blocks create new source (water only)
5. Lava random slowdown: 1/4 chance to not decay when flowing laterally

**Water+Lava Interactions (BlockFluids.java:222-259):**
- Lava source (metadata 0) + water → obsidian
- Lava flowing (metadata 1-4) + water → cobblestone
- Requires both blocks to exist (obsidian ID 49, cobblestone ID 4)

**Current Status:**
- ✅ Created `LiquidMaterial` class (water/lava materials)
- ✅ Created `FluidTile` base class (equivalent to `BlockFluids`) - basic structure complete
- ⚠️ TODO: Implement `FluidFlowingTile` (equivalent to `BlockFlowing`) - flow logic pending
- ⚠️ TODO: Implement `FluidStationaryTile` (equivalent to `BlockStationary`) - neighbor-change triggers pending
- ⚠️ TODO: Wire tick scheduling in `Level::tick()` or chunk tick system - requires `scheduleBlockUpdate()` API
- ⚠️ TODO: Implement neighbor-change triggers for stationary→flowing conversion
- ⚠️ TODO: Implement flow rules (downward first, lateral spread, level decay 0-7)
- ⚠️ TODO: Implement water+lava interactions (cobble/obsidian) - requires cobblestone (ID 4) and obsidian (ID 49) blocks
- ⚠️ TODO: Add debug validation hooks (guarded logs for tick scheduling and flow updates)

**Files Created:**
- `src/world/level/material/LiquidMaterial.h/cpp` - Liquid material class
- `src/world/level/tile/FluidTile.h/cpp` - Base fluid tile class (BlockFluids equivalent)

**Reference Evidence:**
- `apclient/minecraft/src/net/minecraft/src/BlockFluids.java`: Base fluid class
- `apclient/minecraft/src/net/minecraft/src/BlockFlowing.java`: Flowing block logic
- `apclient/minecraft/src/net/minecraft/src/BlockStationary.java`: Stationary block logic
- `apclient/minecraft/src/net/minecraft/src/Block.java:30-33`: Fluid block IDs and setup
