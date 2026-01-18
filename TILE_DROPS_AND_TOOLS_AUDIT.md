# Tile Drops and Tool Requirements Audit
## Alpha 1.2.6 (compared with Beta 1.2, excluding Beta-only features)

This document audits all tiles in Alpha 1.2.6 for:
1. **Item drops** - What item/block drops when broken
2. **Tool material requirements** - Which tool type is most effective (pickaxe, axe, shovel, or none)
3. **Tool tier requirements** - Minimum tool tier needed to harvest (if any)

**Reference Sources:**
- Alpha 1.2.6: `alpha1.2.6/minecraft/src/net/minecraft/src/`
- Beta 1.2: `newb12/net/minecraft/world/level/tile/`
- Beta tool effectiveness: `newb12/net/minecraft/world/item/PickaxeItem.java`, `DiggerItem.java`

---

## Tool Material Requirements (Beta Reference)

**Pickaxe** (Material.stone, Material.metal):
- Stone, cobblestone, ores, obsidian, stone bricks, mossy cobblestone
- Metal blocks (iron, gold, diamond/emerald)
- Ice, hell rock

**Axe** (Material.wood):
- Wood blocks, logs, planks
- Bookshelf, chest, workbench, door (wood)

**Shovel** (Material.dirt, Material.sand):
- Dirt, grass, farmland
- Sand, gravel
- Snow

**Hoe** (Material.ground):
- Grass (for tilling), dirt (for tilling)

**None** (can break with hand):
- Plants, flowers, mushrooms
- Glass, cloth
- Torches, signs, ladders
- Redstone components
- Most decorative blocks

---

## Tile Drop List

### Basic Blocks

| Block ID | Tile Name | Drop | Quantity | Tool | Notes |
|---------|------------|------|----------|------|-------|
| 1 | Stone | Cobblestone (ID 4) | 1 | Pickaxe | ✅ Implemented in StoneTile.cpp |
| 2 | Grass | Dirt (ID 3) | 1 | Shovel | ✅ Implemented in GrassTile.cpp |
| 3 | Dirt | Dirt (ID 3) | 1 | Shovel | ✅ Default (self) |
| 4 | Cobblestone | Cobblestone (ID 4) | 1 | None | ✅ Default (self) - Note: Cobblestone doesn't get sped up by pickaxe |
| 5 | Wood/Planks | Wood (ID 5) | 1 | Axe | ✅ Default (self) |
| 6 | Sapling | Sapling (ID 6) | 1 | None | ✅ Default (self) |
| 7 | Bedrock | Nothing | 0 | None | Unbreakable |
| 8-9 | Water | Nothing | 0 | None | Liquid |
| 10-11 | Lava | Nothing | 0 | None | Liquid |
| 12 | Sand | Sand (ID 12) | 1 | Shovel | ✅ Default (self) |
| 13 | Gravel | Gravel (ID 13) or Flint (Item ID 62) | 1 | Shovel | ⚠️ **TODO**: 10% chance for flint (BlockGravel.java:10-11) |
| 14 | Gold Ore | Gold Ore (ID 14) | 1 | Pickaxe (tier ≥2) | ✅ Default (self) - Beta: requires iron+ |
| 15 | Iron Ore | Iron Ore (ID 15) | 1 | Pickaxe (tier ≥1) | ✅ Default (self) - Beta: requires stone+ |
| 16 | Coal Ore | Coal (Item ID 263) | 1 | Pickaxe | ⚠️ **TODO**: Drop coal item (BlockOre.java:10-11) |
| 17 | Log | Wood (ID 5) | 1 | Axe | ⚠️ **TODO**: Drop wood block, not log (BlockLog.java:15-16) |
| 18 | Leaves | Sapling (ID 6) or Nothing | 0-1 | None | ⚠️ **TODO**: 5% chance (1/20) for sapling (BlockLeaves.java:108-113) |
| 19 | Sponge | Sponge (ID 19) | 1 | None | ✅ Default (self) |
| 20 | Glass | Nothing | 0 | None | ✅ Default (no drop) |
| 35 | Cloth/Wool | Cloth (ID 35) | 1 | None | ✅ Default (self) |
| 36 | Flower (Yellow) | Nothing | 0 | None | ✅ Default (no drop) |
| 37 | Flower (Red) | Nothing | 0 | None | ✅ Default (no drop) |
| 38 | Mushroom (Brown) | Nothing | 0 | None | ✅ Default (no drop) |
| 39 | Mushroom (Red) | Nothing | 0 | None | ✅ Default (no drop) |
| 40 | Gold Block | Gold Block (ID 41) | 1 | Pickaxe (tier ≥2) | ✅ Default (self) |
| 41 | Iron Block | Iron Block (ID 42) | 1 | Pickaxe (tier ≥1) | ✅ Default (self) |
| 42 | Double Slab | Stair Single (ID 44) | 1 | Pickaxe | ⚠️ **TODO**: Drop stairSingle (BlockStep.java:44-45) |
| 43 | Slab | Slab (ID 44) | 1 | Pickaxe | ✅ Default (self) |
| 44 | Brick | Brick (ID 45) | 1 | Pickaxe | ✅ Default (self) |
| 45 | Bookshelf | Bookshelf (ID 47) | 1 | Axe | ✅ Default (self) |
| 46 | Mossy Cobblestone | Mossy Cobblestone (ID 48) | 1 | Pickaxe | ✅ Default (self) |
| 47 | Obsidian | Obsidian (ID 49) | 1 | Pickaxe (tier 3/diamond) | ✅ Default (self) - Beta: requires diamond |
| 48 | Torch | Torch (ID 50) | 1 | None | ✅ Default (self) |
| 49 | Fire | Nothing | 0 | None | ✅ Default (no drop) |
| 50 | Spawner | Nothing | 0 | None | ✅ Default (no drop) |
| 51 | Wood Stairs | Wood Stairs (ID 53) | 1 | Axe | ✅ Default (self) |
| 52 | Chest | Chest (ID 54) | 1 | Axe | ✅ Default (self) |
| 53 | Redstone Wire | Redstone (Item ID 331) | 1 | None | ⚠️ **TODO**: Drop redstone item |
| 54 | Diamond Ore | Diamond (Item ID 264) | 1 | Pickaxe (tier ≥2) | ⚠️ **TODO**: Drop diamond item (BlockOre.java:10-11) |
| 55 | Diamond Block | Diamond Block (ID 57) | 1 | Pickaxe (tier ≥2) | ✅ Default (self) |
| 56 | Workbench | Workbench (ID 58) | 1 | Axe | ✅ Default (self) |
| 57 | Crops/Wheat | Seeds (Item ID 295) | 0-3 | None | ⚠️ **TODO**: Drop seeds based on growth stage |
| 58 | Farmland | Dirt (ID 3) | 1 | Shovel | ⚠️ **TODO**: Drop dirt (BlockSoil.java:96-97) |
| 59 | Furnace | Furnace Idle (ID 61) | 1 | Pickaxe | ⚠️ **TODO**: Drop stoneOvenIdle (BlockFurnace.java:14-15) |
| 60 | Furnace (Active) | Furnace Idle (ID 61) | 1 | Pickaxe | ⚠️ **TODO**: Drop stoneOvenIdle |
| 61 | Sign | Sign (Item ID 323) | 1 | Axe | ⚠️ **TODO**: Drop sign item (BlockSign.java:76-77) |
| 62 | Wood Door | Wood Door (Item ID 324) | 1 | Axe | ⚠️ **TODO**: Drop door item |
| 63 | Ladder | Ladder (ID 65) | 1 | None | ✅ Default (self) |
| 64 | Rail | Rail (ID 66) | 1 | None | ✅ Default (self) |
| 65 | Stone Stairs | Stone Stairs (ID 67) | 1 | Pickaxe | ✅ Default (self) |
| 66 | Lever | Lever (ID 69) | 1 | None | ✅ Default (self) |
| 67 | Stone Pressure Plate | Stone Pressure Plate (ID 70) | 1 | Pickaxe | ✅ Default (self) |
| 68 | Iron Door | Iron Door (Item ID 330) | 1 | Pickaxe | ⚠️ **TODO**: Drop door item |
| 69 | Wood Pressure Plate | Wood Pressure Plate (ID 72) | 1 | Axe | ✅ Default (self) |
| 70 | Redstone Ore | Redstone Ore (ID 73) | 1 | Pickaxe (tier ≥2) | ✅ Default (self) |
| 71 | Redstone Ore (Lit) | Redstone Ore (ID 73) | 1 | Pickaxe (tier ≥2) | ✅ Default (self) |
| 72 | Not Gate | Not Gate (ID 75) | 1 | None | ✅ Default (self) |
| 73 | Stone Button | Stone Button (ID 77) | 1 | Pickaxe | ✅ Default (self) |
| 74 | Snow | Snowball (Item ID 332) | 1-4 | Shovel | ⚠️ **TODO**: Drop 1-4 snowballs |
| 75 | Ice | Ice (ID 79) | 1 | Pickaxe | ✅ Default (self) |
| 76 | Snow Block | Snowball (Item ID 332) | 4 | Shovel | ⚠️ **TODO**: Drop 4 snowballs (BlockSnowBlock.java:11-16) |
| 77 | Cactus | Cactus (ID 81) | 1 | None | ✅ Default (self) |
| 78 | Clay | Clay (Item ID 337) | 4 | Shovel | ✅ **Implemented** in ClayTile.cpp (BlockClay.java:10-15) |
| 79 | Reed/Sugar Cane | Reed (Item ID 338) | 1 | None | ⚠️ **TODO**: Drop reed item (BlockReed.java:58-59) |
| 80 | Jukebox | Jukebox (ID 84) | 1 | Axe | ✅ Default (self) |
| 81 | Fence | Fence (ID 85) | 1 | Axe | ✅ Default (self) |
| 82 | Pumpkin | Pumpkin (ID 86) | 1 | None | ✅ Default (self) |
| 83 | Hell Rock | Hell Rock (ID 87) | 1 | Pickaxe | ✅ Default (self) |
| 84 | Hell Sand | Hell Sand (ID 88) | 1 | Shovel | ✅ Default (self) |
| 85 | Light Gem | Light Gem (ID 89) | 1 | None | ✅ Default (self) |
| 86 | Portal | Nothing | 0 | None | ✅ Default (no drop) |
| 87 | Lit Pumpkin | Pumpkin (ID 86) | 1 | None | ✅ Default (self) |
| 88 | TNT | TNT (ID 46) | 1 | None | ✅ Default (self) |

---

## Implementation Status

### ✅ Fully Implemented
- Stone → Cobblestone
- Grass → Dirt
- Clay → 4 Clay items

### ⚠️ Needs Implementation

1. **Ore Drops**:
   - Coal Ore → Coal item (Item ID 263, shiftedIndex 256+263=519)
   - Diamond Ore → Diamond item (Item ID 264, shiftedIndex 256+264=520)

2. **Gravel**:
   - 10% chance for Flint item (Item ID 62, shiftedIndex 256+62=318)
   - 90% chance for Gravel block (ID 13)

3. **Leaves**:
   - 5% chance (1/20) for Sapling (Block ID 6)
   - 95% chance for nothing

4. **Log**:
   - Drop Wood block (ID 5), not Log block (ID 17)

5. **Farmland**:
   - Drop Dirt (ID 3)

6. **Stairs**:
   - Double Slab → Stair Single (ID 44)

7. **Furnace**:
   - Active/Idle → Furnace Idle (ID 61)

8. **Sign**:
   - Drop Sign item (Item ID 323, shiftedIndex 256+323=579)

9. **Snow Block**:
   - Drop 4 Snowball items (Item ID 332, shiftedIndex 256+332=588)

10. **Reed**:
    - Drop Reed item (Item ID 338, shiftedIndex 256+338=594)

11. **Doors**:
    - Wood Door → Wood Door item (Item ID 324)
    - Iron Door → Iron Door item (Item ID 330)

12. **Redstone Wire**:
    - Drop Redstone item (Item ID 331, shiftedIndex 256+331=587)

---

## Tool Material Requirements (To Implement)

Based on Beta 1.2 `PickaxeItem.java` and `DiggerItem.java`:

### Pickaxe Required (Material.stone, Material.metal):
- Stone (ID 1)
- Cobblestone (ID 4)
- All ores (14, 15, 16, 54, 70, 71)
- Obsidian (ID 49)
- Stone bricks (ID 4, 45, 48)
- Metal blocks (40, 41, 55)
- Ice (ID 75)
- Hell Rock (ID 87)
- Furnace (59, 60)
- Stone stairs (65)
- Stone pressure plate (67)
- Stone button (73)

### Axe Required (Material.wood):
- Wood/Planks (ID 5)
- Log (ID 17) - though drops wood
- Bookshelf (ID 47)
- Chest (ID 52)
- Workbench (ID 56)
- Wood stairs (ID 51)
- Wood door (ID 62)
- Wood pressure plate (ID 69)
- Fence (ID 81)
- Jukebox (ID 80)
- Sign (ID 61)

### Shovel Required (Material.dirt, Material.sand):
- Dirt (ID 3)
- Grass (ID 2)
- Farmland (ID 58)
- Sand (ID 12)
- Gravel (ID 13)
- Snow (ID 74)
- Snow Block (ID 76)
- Hell Sand (ID 88)
- Clay (ID 78)

### No Tool Required:
- All plants (6, 36, 37, 38, 39, 57, 77, 79, 82, 87)
- Glass (ID 20)
- Cloth (ID 35)
- Torch (ID 48)
- Ladder (ID 63)
- Rail (ID 64)
- Lever (ID 66)
- Redstone components (53, 70, 71, 72, 73)
- Light Gem (ID 85)
- Portal (ID 86)
- TNT (ID 46)
- Fire (ID 49)
- Spawner (ID 50)
- Bedrock (ID 7) - unbreakable
- Liquids (8-11)

---

## Next Steps

1. Implement missing item drops in tile classes
2. Add tool material checking in `canHarvestBlock()` methods
3. Add tool tier requirements for special blocks (obsidian, ores)
4. Test all drops match Alpha 1.2.6 behavior
