# Beta 1.0 to Beta 1.2_02 Additions (Not in Alpha 1.2.6)

## Overview
This document lists all features, blocks, items, mobs, mechanics, and changes added between **Minecraft Beta 1.0** and **Beta 1.2_02** that do **not** exist in **Alpha 1.2.6**. This is for reference purposes when porting Alpha 1.2.6.

**Note**: This document is for informational purposes only. Since this is an **Alpha 1.2.6 port**, these Beta-exclusive features should **NOT** be implemented unless specifically needed for compatibility or reference purposes.

**References**: Information compiled from [Minecraft Wiki](https://minecraft.wiki/) - [Beta 1.0](https://minecraft.wiki/w/Java_Edition_Beta_1.0), Beta 1.1, and Beta 1.2 pages.

---

## Beta 1.0 Additions (December 20, 2010)

According to the [Minecraft Wiki Beta 1.0 page](https://minecraft.wiki/w/Java_Edition_Beta_1.0), Beta 1.0 had very few additions - it was primarily a technical update with server-side inventory implementation.

### General Additions

1. **Server-Side Inventory**
   - Working server-side inventory system
   - Items now show their names while hovering the mouse over them (tooltips)
   - Removed armor outlines in the armor slots
   - Players can't store items in the inventory's crafting fields anymore
   - **Status**: NOT in Alpha 1.2.6

2. **Capes Support**
   - Added support for capes (initially known as cloaks)
   - Only the Mojang cape existed at the time
   - Added support for deadmau5's ears
   - **Status**: NOT in Alpha 1.2.6

3. **Leaf Decay**
   - Reintroduced leaf decay mechanics (acts differently from before)
   - Leaves now decay when not connected to logs
   - **Status**: NOT in Alpha 1.2.6 (no leaf decay in Alpha)

4. **Egg Throwing**
   - Eggs can now be thrown
   - Throwing an egg has a 1⁄8 (12.5%) chance of spawning a chicken
   - Eggs now stack to 16 instead of 64
   - **Status**: NOT in Alpha 1.2.6

### Gameplay Changes

1. **Chest Behavior**
   - Moving too far away from a chest, or having it blow up, closes the inventory screen
   - **Status**: Changed from Alpha 1.2.6

2. **Multiplayer**
   - Made servers save chunks way less often in most cases
   - Chunks don't resave if they got saved in the last 30 seconds
   - **Status**: Changed from Alpha 1.2.6

### Bug Fixes
- Fixed `/kill` command
- Tools thrown on the ground do not repair themselves anymore
- Many issues fixed as a result of a working server-side inventory

---

## Beta 1.1 Additions (December 22, 2010)

According to the Minecraft Wiki, Beta 1.1 was a very minor update:

1. **Leaf Decay Code Rewritten**
   - Improved leaf decay mechanics and performance
   - **Status**: Improvement over Beta 1.0

2. **Seasonal Splash Texts**
   - Added holiday-themed splash texts for Christmas and New Year's
   - **Status**: NOT in Alpha 1.2.6

---

## Beta 1.2 Additions (January 13, 2011)

Beta 1.2 was the major content update that introduced most of the new blocks, items, mobs, and mechanics.

### Blocks

1. **Lapis Lazuli Ore (Block ID 21)**
   - New ore block that drops Lapis Lazuli when mined
   - Generates in the world (similar to other ores)
   - Used for crafting blue dye and lapis lazuli blocks
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

2. **Lapis Lazuli Block (Block ID 22)**
   - Decorative block crafted from 9 Lapis Lazuli
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

3. **Dispenser (Block ID 23)**
   - Redstone-powered block that stores and dispenses items
   - Activates when powered by redstone
   - Can dispense items, arrows, fire charges, etc.
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

4. **Sandstone (Block ID 24)**
   - Decorative block crafted from sand (4 sand = 4 sandstone)
   - Found naturally beneath sand layers in desert biomes
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

5. **Note Block (Block ID 25)**
   - Musical block that produces notes when activated
   - Right-click to tune the pitch
   - Instrument sound varies based on block beneath it
   - Can be powered by redstone
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

6. **Cake (Block ID 92)**
   - Placeable food item
   - Can be consumed in slices (7 slices total)
   - Restores hunger when eaten
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

### Items

1. **Charcoal**
   - Alternative fuel source to coal
   - Obtained by smelting wood/logs in a furnace
   - Same properties as coal (8 items per fuel unit)
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

2. **Bone**
   - Dropped by skeletons
   - Can be crafted into bone meal
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

3. **Bone Meal**
   - Crafted from bones (1 bone = 3 bone meal)
   - Used as fertilizer for crops (instantly grows wheat)
   - Can also be used as a white dye
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

4. **Ink Sac**
   - Dropped by squids
   - Used as black dye
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

5. **Lapis Lazuli**
   - Dropped by lapis lazuli ore
   - Used as blue dye
   - Can be crafted into lapis lazuli blocks
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

6. **Sugar**
   - Crafted from sugar cane (1 sugar cane = 1 sugar)
   - Used in cake recipe
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

7. **Dyes (15 colors total)**
   - System for dyeing wool blocks and sheep
   - **White Dye**: Bone meal
   - **Orange Dye**: Orange tulip OR red + yellow dye
   - **Magenta Dye**: Allium OR pink + purple dye
   - **Light Blue Dye**: Blue orchid OR blue + white dye
   - **Yellow Dye**: Dandelion OR sunflower
   - **Lime Dye**: Green + white dye
   - **Pink Dye**: Pink tulip OR peony
   - **Gray Dye**: Black + white dye
   - **Light Gray Dye**: Gray + white dye OR azure bluet/oxeye daisy/white tulip
   - **Cyan Dye**: Green + blue dye
   - **Purple Dye**: Red + blue dye
   - **Blue Dye**: Lapis lazuli
   - **Brown Dye**: Cocoa beans (added in Beta 1.2, but unobtainable until later)
   - **Green Dye**: Cactus (smelted)
   - **Red Dye**: Rose/poppy OR beetroot
   - **Black Dye**: Ink sac
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

8. **Dyed Wool**
   - Wool blocks that have been dyed in various colors
   - Can be crafted by combining wool with dye
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

9. **Cocoa Beans (Item)**
   - Brown dye source
   - Added in Beta 1.2 but unobtainable in survival mode initially
   - Later used to grow cocoa on jungle trees
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

### Mobs

1. **Squid**
   - Aquatic passive mob
   - Spawns in water
   - Drops ink sacs (1-3) upon death
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

2. **Slimes (Reintroduced)**
   - Hostile mob that spawns in specific chunks
   - Was removed in earlier versions, reintroduced in Beta 1.2
   - Drops slimeballs
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

3. **Colored Sheep Variants**
   - Sheep can now spawn in gray, light gray, and black
   - Previously only white sheep existed in Alpha
   - Drop wool matching their color
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

### World Generation

1. **Birch Trees**
   - New tree type with light-colored wood
   - Spawns in birch forest biomes
   - Drops birch logs and birch leaves
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

2. **Spruce/Pine Trees**
   - New tree type with dark wood
   - Spawns in taiga/forest biomes
   - Drops spruce logs and spruce leaves
   - Taller variant than oak trees
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

### Gameplay Mechanics

1. **Wool Dyeing System**
   - Players can dye wool blocks using dyes
   - 15 different colored wool variants
   - Can dye sheep by right-clicking with dye
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

2. **Sheep Dyeing**
   - Right-click sheep with dye to change their color
   - Dyed sheep drop colored wool when sheared
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

3. **Note Block Mechanics**
   - Musical blocks that play notes
   - Pitch can be adjusted (24 different pitches)
   - Instrument sound depends on block beneath
   - Redstone-powered activation
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

4. **Dispenser Mechanics**
   - Stores up to 9 item stacks
   - Dispenses items when powered by redstone
   - Special behavior for different items (arrows, fire charges, etc.)
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

5. **Bone Meal Fertilization**
   - Right-click crops with bone meal to instantly grow them
   - Works on wheat
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

6. **Cake Consumption**
   - Place cake block and right-click to eat a slice
   - Restores hunger per slice
   - 7 slices total per cake
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6

### Gameplay Changes

1. **Reeds Renamed to Sugar Cane**
   - Item name changed from "Reeds" to "Sugar Cane"
   - Still functions the same way
   - **Added in**: Beta 1.2
   - **Status**: NOT in Alpha 1.2.6 (still called "Reeds")

---

## Beta 1.2_01 Additions (January 14, 2011)

Minor bugfix update:

1. **Bug Fixes**
   - Fixed painting disappearing bug when chunk was unloaded
   - Various other minor fixes

---

## Beta 1.2_02 Additions (January 21, 2011)

Minor bugfix update:

1. **Skin Download Source**
   - Changed where skins are downloaded from
   - Now using Amazon S3 instead of Minecraft.net
   - **Added in**: Beta 1.2_02
   - **Status**: NOT in Alpha 1.2.6

2. **Screenshot Functionality**
   - Added Shift+F2 for large TGA format screenshots
   - **Added in**: Beta 1.2_02
   - **Status**: NOT in Alpha 1.2.6

---

## Summary for Alpha 1.2.6 Port

**IMPORTANT**: Since this is an **Alpha 1.2.6 port**, the following Beta-exclusive features should **NOT** be implemented:

### Beta 1.0 Features (Technical Only)
- ❌ Server-side inventory system
- ❌ Inventory tooltips
- ❌ Capes support
- ❌ Leaf decay mechanics
- ❌ Egg throwing (chicken spawning)

### Beta 1.2 Features (Content)
- ❌ Lapis Lazuli Ore and Block
- ❌ Dispenser
- ❌ Sandstone
- ❌ Note Block
- ❌ Cake
- ❌ Charcoal
- ❌ Bone and Bone Meal
- ❌ Ink Sac
- ❌ Lapis Lazuli (item)
- ❌ Sugar (item)
- ❌ Dye system (15 dyes)
- ❌ Dyed wool
- ❌ Squids
- ❌ Slimes (they were removed in Alpha)
- ❌ Colored sheep variants (only white sheep in Alpha)
- ❌ Birch and Spruce trees
- ❌ Note block mechanics
- ❌ Dispenser mechanics
- ❌ Wool dyeing system
- ❌ Sheep dyeing
- ❌ Bone meal fertilization
- ❌ Cake consumption

These features should only be used as **reference** when porting Alpha 1.2.6 features that may have similar implementations in Beta code, but should not be included in the final Alpha 1.2.6 port.

---

## References

This document was compiled from:
- [Minecraft Wiki - Java Edition Beta 1.0](https://minecraft.wiki/w/Java_Edition_Beta_1.0)
- Minecraft Wiki - Java Edition Beta 1.1
- Minecraft Wiki - Java Edition Beta 1.2
- Minecraft Wiki - Java Edition Beta 1.2_01
- Minecraft Wiki - Java Edition Beta 1.2_02
- Official Minecraft changelogs
- Community documentation

**Last Updated**: Based on Minecraft Wiki pages for Beta 1.0 through Beta 1.2_02
