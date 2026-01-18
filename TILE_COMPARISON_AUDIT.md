# Tile Definitions Comparison Audit
## Alpha 1.2.6 vs Beta 1.2 vs C++ Implementation

This document compares tile definitions between Alpha 1.2.6 (Block.java), Beta 1.2 (Tile.java), and our C++ implementation (Tile.cpp) to ensure accuracy.

**Format**: `ID | Name | Texture | Material | Hardness | Resistance | Sound | Light | LightOpacity`

---

## Basic Blocks

### Stone (ID 1)
- **Alpha**: `BlockStone(1, 1)` → `Material.rock`, hardness 1.5F, resistance 10.0F, soundStoneFootstep
- **Beta**: `StoneTile(1, 1)` → `Material.stone`, destroyTime 1.5F, explodeable 10.0F, SOUND_STONE
- **C++**: `StoneTile(1, 1)` → `Material::stone`, destroyTime 1.5f, explodeable 10.0f, SOUND_STONE ✅

### Grass (ID 2)
- **Alpha**: `BlockGrass(2)` → `Material.ground`, hardness 0.6F, soundGrassFootstep
- **Beta**: `GrassTile(2)` → `Material.dirt`, destroyTime 0.6F, SOUND_GRASS
- **C++**: `GrassTile(2)` → `Material::dirt`, destroyTime 0.6f, SOUND_GRASS ✅

### Dirt (ID 3)
- **Alpha**: `BlockDirt(3, 2)` → `Material.ground`, hardness 0.5F, soundGravelFootstep
- **Beta**: `DirtTile(3, 2)` → `Material.dirt`, destroyTime 0.5F, SOUND_GRAVEL
- **C++**: `DirtTile(3, 2)` → `Material::dirt`, destroyTime 0.5f, SOUND_GRAVEL ✅

### Cobblestone (ID 4)
- **Alpha**: `Block(4, 16, Material.rock)` → hardness 2.0F, resistance 10.0F, soundStoneFootstep
- **Beta**: `Tile(4, 16, Material.stone)` → destroyTime 2.0F, explodeable 10.0F, SOUND_STONE
- **C++**: `CobblestoneTile(4, 16)` → `Material::stone`, destroyTime 2.0f, explodeable 10.0f, SOUND_STONE ✅

### Wood/Planks (ID 5)
- **Alpha**: `Block(5, 4, Material.wood)` → hardness 2.0F, resistance 5.0F, soundWoodFootstep
- **Beta**: `Tile(5, 4, Material.wood)` → destroyTime 2.0F, explodeable 5.0F, SOUND_WOOD
- **C++**: `WoodTile(5, 4)` → `Material::wood`, destroyTime 2.0f, explodeable 5.0f, SOUND_WOOD ✅

### Sapling (ID 6)
- **Alpha**: `BlockSapling(6, 15)` → hardness 0.0F, soundGrassFootstep
- **Beta**: `Sapling(6, 15)` → destroyTime 0.0F, SOUND_GRASS
- **C++**: `SaplingTile(6, 15)` → destroyTime 0.0f, SOUND_GRASS ✅

### Bedrock (ID 7)
- **Alpha**: `Block(7, 17, Material.rock)` → hardness -1.0F, resistance 6000000.0F, soundStoneFootstep
- **Beta**: `Tile(7, 17, Material.stone)` → destroyTime -1.0F, explodeable 6000000.0F, SOUND_STONE
- **C++**: `UnbreakableTile(7, 17)` → `Material::stone`, destroyTime -1.0f, explodeable 6000000.0f, SOUND_STONE ✅

### Water Still (ID 8)
- **Alpha**: `BlockFlowing(8, Material.water)` → hardness 100.0F, lightOpacity 3
- **Beta**: `LiquidTileDynamic(8, Material.water)` → destroyTime 100.0F, lightBlock 3
- **C++**: `FluidFlowingTile(8, Material::water)` → destroyTime 100.0f, lightBlock 3 ✅

### Water Moving (ID 9)
- **Alpha**: `BlockStationary(9, Material.water)` → hardness 100.0F, lightOpacity 3
- **Beta**: `LiquidTileStatic(9, Material.water)` → destroyTime 100.0F, lightBlock 3
- **C++**: `FluidStationaryTile(9, Material::water)` → destroyTime 100.0f, lightBlock 3 ✅

### Lava Still (ID 10)
- **Alpha**: `BlockFlowing(10, Material.lava)` → hardness 0.0F, lightValue 1.0F, lightOpacity 255
- **Beta**: `LiquidTileDynamic(10, Material.lava)` → destroyTime 0.0F, lightEmission 1.0F, lightBlock 255
- **C++**: `FluidFlowingTile(10, Material::lava)` → destroyTime 0.0f, lightEmission 15 (1.0F*15), lightBlock 255 ✅

### Lava Moving (ID 11)
- **Alpha**: `BlockStationary(11, Material.lava)` → hardness 100.0F, lightValue 1.0F, lightOpacity 255
- **Beta**: `LiquidTileStatic(11, Material.lava)` → destroyTime 100.0F, lightEmission 1.0F, lightBlock 255
- **C++**: `FluidStationaryTile(11, Material::lava)` → destroyTime 100.0f, lightEmission 15, lightBlock 255 ✅

### Sand (ID 12)
- **Alpha**: `BlockSand(12, 18)` → hardness 0.5F, soundSandFootstep
- **Beta**: `SandTile(12, 18)` → destroyTime 0.5F, SOUND_SAND
- **C++**: `SandTile(12, 18)` → destroyTime 0.5f, SOUND_SAND ✅

### Gravel (ID 13)
- **Alpha**: `BlockGravel(13, 19)` → hardness 0.6F, soundGravelFootstep
- **Beta**: `GravelTile(13, 19)` → destroyTime 0.6F, SOUND_GRAVEL
- **C++**: `GravelTile(13, 19)` → destroyTime 0.6f, SOUND_GRAVEL ✅

### Gold Ore (ID 14)
- **Alpha**: `BlockOre(14, 32)` → hardness 3.0F, resistance 5.0F, soundStoneFootstep
- **Beta**: `OreTile(14, 32)` → destroyTime 3.0F, explodeable 5.0F, SOUND_STONE
- **C++**: `OreTile(14, 32, 0)` → destroyTime 3.0f, explodeable 5.0f, SOUND_STONE ✅

### Iron Ore (ID 15)
- **Alpha**: `BlockOre(15, 33)` → hardness 3.0F, resistance 5.0F, soundStoneFootstep
- **Beta**: `OreTile(15, 33)` → destroyTime 3.0F, explodeable 5.0F, SOUND_STONE
- **C++**: `OreTile(15, 33, 0)` → destroyTime 3.0f, explodeable 5.0f, SOUND_STONE ✅

### Coal Ore (ID 16)
- **Alpha**: `BlockOre(16, 34)` → hardness 3.0F, resistance 5.0F, soundStoneFootstep
- **Beta**: `OreTile(16, 34)` → destroyTime 3.0F, explodeable 5.0F, SOUND_STONE
- **C++**: `OreTile(16, 34, 263)` → destroyTime 3.0f, explodeable 5.0f, SOUND_STONE ✅ (drops coal item)

### Log/Wood (ID 17)
- **Alpha**: `BlockLog(17)` → `Material.wood`, hardness 2.0F, soundWoodFootstep
- **Beta**: `TreeTile(17)` → `Material.wood`, destroyTime 2.0F, SOUND_WOOD
- **C++**: `TreeTile(17)` → `Material::wood`, destroyTime 2.0f, SOUND_WOOD ✅

### Leaves (ID 18)
- **Alpha**: `BlockLeaves(18, 52)` → hardness 0.2F, lightOpacity 1, soundGrassFootstep
- **Beta**: `LeafTile(18, 52)` → destroyTime 0.2F, lightBlock 1, SOUND_GRASS
- **C++**: `LeafTile(18, 52)` → destroyTime 0.2f, lightBlock 1, SOUND_GRASS ✅

### Sponge (ID 19)
- **Alpha**: `BlockSponge(19)` → hardness 0.6F, soundGrassFootstep
- **Beta**: `Sponge(19)` → destroyTime 0.6F, SOUND_GRASS
- **C++**: `SpongeTile(19)` → destroyTime 0.6f, SOUND_GRASS ✅

### Glass (ID 20)
- **Alpha**: `BlockGlass(20, 49, Material.glass, false)` → hardness 0.3F, soundGlassFootstep
- **Beta**: `GlassTile(20, 49, Material.glass, false)` → destroyTime 0.3F, SOUND_GLASS
- **C++**: `GlassTile(20, 49, Material::glass, false)` → destroyTime 0.3f, SOUND_GLASS ✅

---

## Issues Found

### Material Discrepancies
1. **Grass**: Alpha uses `Material.ground`, Beta uses `Material.dirt` - C++ uses `Material::dirt` (Beta) ✅
2. **Dirt**: Alpha uses `Material.ground`, Beta uses `Material.dirt` - C++ uses `Material::dirt` (Beta) ✅

### Missing Properties
- Need to verify all light values match
- Need to verify all light opacity values match
- Need to verify all resistance values match

---

## Next Steps
1. Continue systematic comparison for all 88+ tiles
2. Fix any material mismatches
3. Verify all hardness/resistance/sound values
4. Check light emission and opacity values
