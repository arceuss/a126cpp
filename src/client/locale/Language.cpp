#include "client/locale/Language.h"

#include "java/String.h"

Language Language::singleton;

Language::Language()
{
	// Hardcoded Alpha 1.2.6 language strings
	// GUI
	elements[String::fromUTF8("gui.done")] = String::fromUTF8("Done");
	elements[String::fromUTF8("gui.cancel")] = String::fromUTF8("Cancel");
	elements[String::fromUTF8("gui.toMenu")] = String::fromUTF8("Back to title screen");
	
	// Menu
	elements[String::fromUTF8("menu.singleplayer")] = String::fromUTF8("Singleplayer");
	elements[String::fromUTF8("menu.multiplayer")] = String::fromUTF8("Multiplayer");
	elements[String::fromUTF8("menu.mods")] = String::fromUTF8("Mods and Texture Packs");
	elements[String::fromUTF8("menu.options")] = String::fromUTF8("Options...");
	elements[String::fromUTF8("menu.quit")] = String::fromUTF8("Quit Game");
	
	// Select World
	elements[String::fromUTF8("selectWorld.title")] = String::fromUTF8("Select World");
	elements[String::fromUTF8("selectWorld.empty")] = String::fromUTF8("empty");
	elements[String::fromUTF8("selectWorld.world")] = String::fromUTF8("World");
	elements[String::fromUTF8("selectWorld.delete")] = String::fromUTF8("Delete world...");
	
	// Multiplayer
	elements[String::fromUTF8("multiplayer.title")] = String::fromUTF8("Play Multiplayer");
	elements[String::fromUTF8("multiplayer.connect")] = String::fromUTF8("Connect");
	elements[String::fromUTF8("multiplayer.info1")] = String::fromUTF8("Minecraft Multiplayer is currently not finished, but there");
	elements[String::fromUTF8("multiplayer.info2")] = String::fromUTF8("is some buggy early testing going on.");
	elements[String::fromUTF8("multiplayer.ipinfo")] = String::fromUTF8("Enter the IP of a server to connect to it:");
	elements[String::fromUTF8("multiplayer.downloadingTerrain")] = String::fromUTF8("Downloading terrain");
	
	// Connect
	elements[String::fromUTF8("connect.connecting")] = String::fromUTF8("Connecting to the server...");
	elements[String::fromUTF8("connect.authorizing")] = String::fromUTF8("Logging in...");
	elements[String::fromUTF8("connect.failed")] = String::fromUTF8("Failed to connect to the server");
	
	// Disconnect
	elements[String::fromUTF8("disconnect.genericReason")] = String::fromUTF8("%s");
	elements[String::fromUTF8("disconnect.disconnected")] = String::fromUTF8("Disconnected by Server");
	elements[String::fromUTF8("disconnect.lost")] = String::fromUTF8("Connection Lost");
	elements[String::fromUTF8("disconnect.kicked")] = String::fromUTF8("Was kicked from the game");
	elements[String::fromUTF8("disconnect.timeout")] = String::fromUTF8("Timed out");
	elements[String::fromUTF8("disconnect.closed")] = String::fromUTF8("Connection closed");
	elements[String::fromUTF8("disconnect.loginFailed")] = String::fromUTF8("Failed to login");
	elements[String::fromUTF8("disconnect.loginFailedInfo")] = String::fromUTF8("Failed to login: %s");
	elements[String::fromUTF8("disconnect.quitting")] = String::fromUTF8("Quitting");
	elements[String::fromUTF8("disconnect.endOfStream")] = String::fromUTF8("End of stream");
	elements[String::fromUTF8("disconnect.overflow")] = String::fromUTF8("Buffer overflow");
	
	// Options
	elements[String::fromUTF8("options.off")] = String::fromUTF8("OFF");
	elements[String::fromUTF8("options.on")] = String::fromUTF8("ON");
	elements[String::fromUTF8("options.title")] = String::fromUTF8("Options");
	elements[String::fromUTF8("options.controls")] = String::fromUTF8("Controls...");
	elements[String::fromUTF8("options.music")] = String::fromUTF8("Music");
	elements[String::fromUTF8("options.sound")] = String::fromUTF8("Sound");
	elements[String::fromUTF8("options.invertMouse")] = String::fromUTF8("Invert Mouse");
	elements[String::fromUTF8("options.sensitivity")] = String::fromUTF8("Sensitivity");
	elements[String::fromUTF8("options.sensitivity.min")] = String::fromUTF8("*yawn*");
	elements[String::fromUTF8("options.sensitivity.max")] = String::fromUTF8("HYPERSPEED!!!");
	elements[String::fromUTF8("options.renderDistance")] = String::fromUTF8("Render Distance");
	elements[String::fromUTF8("options.renderDistance.tiny")] = String::fromUTF8("Tiny");
	elements[String::fromUTF8("options.renderDistance.short")] = String::fromUTF8("Short");
	elements[String::fromUTF8("options.renderDistance.normal")] = String::fromUTF8("Normal");
	elements[String::fromUTF8("options.renderDistance.far")] = String::fromUTF8("Far");
	elements[String::fromUTF8("options.viewBobbing")] = String::fromUTF8("View Bobbing");
	elements[String::fromUTF8("options.anaglyph")] = String::fromUTF8("3D Anaglyph");
	elements[String::fromUTF8("options.limitFramerate")] = String::fromUTF8("Limit Framerate");
	elements[String::fromUTF8("options.difficulty")] = String::fromUTF8("Difficulty");
	elements[String::fromUTF8("options.difficulty.peaceful")] = String::fromUTF8("Peaceful");
	elements[String::fromUTF8("options.difficulty.easy")] = String::fromUTF8("Easy");
	elements[String::fromUTF8("options.difficulty.normal")] = String::fromUTF8("Normal");
	elements[String::fromUTF8("options.difficulty.hard")] = String::fromUTF8("Hard");
	elements[String::fromUTF8("options.graphics")] = String::fromUTF8("Graphics");
	elements[String::fromUTF8("options.graphics.fancy")] = String::fromUTF8("Fancy");
	elements[String::fromUTF8("options.graphics.fast")] = String::fromUTF8("Fast");
	
	// Controls
	elements[String::fromUTF8("controls.title")] = String::fromUTF8("Controls");
	elements[String::fromUTF8("key.forward")] = String::fromUTF8("Forward");
	elements[String::fromUTF8("key.left")] = String::fromUTF8("Left");
	elements[String::fromUTF8("key.back")] = String::fromUTF8("Back");
	elements[String::fromUTF8("key.right")] = String::fromUTF8("Right");
	elements[String::fromUTF8("key.jump")] = String::fromUTF8("Jump");
	elements[String::fromUTF8("key.inventory")] = String::fromUTF8("Inventory");
	elements[String::fromUTF8("key.drop")] = String::fromUTF8("Drop");
	elements[String::fromUTF8("key.chat")] = String::fromUTF8("Chat");
	elements[String::fromUTF8("key.fog")] = String::fromUTF8("Toggle Fog");
	elements[String::fromUTF8("key.sneak")] = String::fromUTF8("Sneak");
	elements[String::fromUTF8("key.playerlist")] = String::fromUTF8("List players");
	
	// Texture Pack
	elements[String::fromUTF8("texturePack.openFolder")] = String::fromUTF8("Open texture pack folder");
	elements[String::fromUTF8("texturePack.title")] = String::fromUTF8("Select Texture Pack");
	elements[String::fromUTF8("texturePack.folderInfo")] = String::fromUTF8("(Place texture pack files here)");
	
	// Tiles - Common blocks
	elements[String::fromUTF8("tile.stone.name")] = String::fromUTF8("Stone");
	elements[String::fromUTF8("tile.grass.name")] = String::fromUTF8("Grass");
	elements[String::fromUTF8("tile.dirt.name")] = String::fromUTF8("Dirt");
	elements[String::fromUTF8("tile.stonebrick.name")] = String::fromUTF8("Cobblestone");
	elements[String::fromUTF8("tile.wood.name")] = String::fromUTF8("Wooden Planks");
	elements[String::fromUTF8("tile.sapling.name")] = String::fromUTF8("Sapling");
	elements[String::fromUTF8("tile.bedrock.name")] = String::fromUTF8("Bedrock");
	elements[String::fromUTF8("tile.water.name")] = String::fromUTF8("Water");
	elements[String::fromUTF8("tile.lava.name")] = String::fromUTF8("Lava");
	elements[String::fromUTF8("tile.sand.name")] = String::fromUTF8("Sand");
	elements[String::fromUTF8("tile.sandStone.name")] = String::fromUTF8("Sandstone");
	elements[String::fromUTF8("tile.gravel.name")] = String::fromUTF8("Gravel");
	elements[String::fromUTF8("tile.oreGold.name")] = String::fromUTF8("Gold Ore");
	elements[String::fromUTF8("tile.oreIron.name")] = String::fromUTF8("Iron Ore");
	elements[String::fromUTF8("tile.oreCoal.name")] = String::fromUTF8("Coal Ore");
	elements[String::fromUTF8("tile.log.name")] = String::fromUTF8("Wood");
	elements[String::fromUTF8("tile.leaves.name")] = String::fromUTF8("Leaves");
	elements[String::fromUTF8("tile.sponge.name")] = String::fromUTF8("Sponge");
	elements[String::fromUTF8("tile.glass.name")] = String::fromUTF8("Glass");
	elements[String::fromUTF8("tile.cloth.name")] = String::fromUTF8("Wool");
	elements[String::fromUTF8("tile.flower.name")] = String::fromUTF8("Flower");
	elements[String::fromUTF8("tile.rose.name")] = String::fromUTF8("Rose");
	elements[String::fromUTF8("tile.mushroom.name")] = String::fromUTF8("Mushroom");
	elements[String::fromUTF8("tile.blockGold.name")] = String::fromUTF8("Block of Gold");
	elements[String::fromUTF8("tile.blockIron.name")] = String::fromUTF8("Block of Iron");
	elements[String::fromUTF8("tile.stoneSlab.name")] = String::fromUTF8("Stone Slab");
	elements[String::fromUTF8("tile.brick.name")] = String::fromUTF8("Bricks");
	elements[String::fromUTF8("tile.tnt.name")] = String::fromUTF8("TNT");
	elements[String::fromUTF8("tile.bookshelf.name")] = String::fromUTF8("Bookshelf");
	elements[String::fromUTF8("tile.stoneMoss.name")] = String::fromUTF8("Moss Stone");
	elements[String::fromUTF8("tile.obsidian.name")] = String::fromUTF8("Obsidian");
	elements[String::fromUTF8("tile.torch.name")] = String::fromUTF8("Torch");
	elements[String::fromUTF8("tile.fire.name")] = String::fromUTF8("Fire");
	elements[String::fromUTF8("tile.mobSpawner.name")] = String::fromUTF8("Monster Spawner");
	elements[String::fromUTF8("tile.stairsWood.name")] = String::fromUTF8("Wooden Stairs");
	elements[String::fromUTF8("tile.chest.name")] = String::fromUTF8("Chest");
	elements[String::fromUTF8("tile.redstoneDust.name")] = String::fromUTF8("Redstone Dust");
	elements[String::fromUTF8("tile.oreDiamond.name")] = String::fromUTF8("Diamond Ore");
	elements[String::fromUTF8("tile.blockDiamond.name")] = String::fromUTF8("Block of Diamond");
	elements[String::fromUTF8("tile.workbench.name")] = String::fromUTF8("Crafting Table");
	elements[String::fromUTF8("tile.crops.name")] = String::fromUTF8("Crops");
	elements[String::fromUTF8("tile.farmland.name")] = String::fromUTF8("Farmland");
	elements[String::fromUTF8("tile.furnace.name")] = String::fromUTF8("Furnace");
	elements[String::fromUTF8("tile.sign.name")] = String::fromUTF8("Sign");
	elements[String::fromUTF8("tile.doorWood.name")] = String::fromUTF8("Wooden Door");
	elements[String::fromUTF8("tile.ladder.name")] = String::fromUTF8("Ladder");
	elements[String::fromUTF8("tile.rail.name")] = String::fromUTF8("Rail");
	elements[String::fromUTF8("tile.stairsStone.name")] = String::fromUTF8("Stone Stairs");
	elements[String::fromUTF8("tile.lever.name")] = String::fromUTF8("Lever");
	elements[String::fromUTF8("tile.pressurePlate.name")] = String::fromUTF8("Pressure Plate");
	elements[String::fromUTF8("tile.doorIron.name")] = String::fromUTF8("Iron Door");
	elements[String::fromUTF8("tile.oreRedstone.name")] = String::fromUTF8("Redstone Ore");
	elements[String::fromUTF8("tile.notGate.name")] = String::fromUTF8("Redstone Torch");
	elements[String::fromUTF8("tile.button.name")] = String::fromUTF8("Button");
	elements[String::fromUTF8("tile.snow.name")] = String::fromUTF8("Snow");
	elements[String::fromUTF8("tile.ice.name")] = String::fromUTF8("Ice");
	elements[String::fromUTF8("tile.cactus.name")] = String::fromUTF8("Cactus");
	elements[String::fromUTF8("tile.clay.name")] = String::fromUTF8("Clay");
	elements[String::fromUTF8("tile.reeds.name")] = String::fromUTF8("Reeds");
	elements[String::fromUTF8("tile.jukebox.name")] = String::fromUTF8("Jukebox");
	elements[String::fromUTF8("tile.fence.name")] = String::fromUTF8("Fence");
	elements[String::fromUTF8("tile.pumpkin.name")] = String::fromUTF8("Pumpkin");
	elements[String::fromUTF8("tile.litpumpkin.name")] = String::fromUTF8("Jack 'o' Lantern");
	elements[String::fromUTF8("tile.hellrock.name")] = String::fromUTF8("Netherrack");
	elements[String::fromUTF8("tile.hellsand.name")] = String::fromUTF8("Soul Sand");
	elements[String::fromUTF8("tile.lightgem.name")] = String::fromUTF8("Glowstone");
	elements[String::fromUTF8("tile.portal.name")] = String::fromUTF8("Portal");
	elements[String::fromUTF8("tile.oreLapis.name")] = String::fromUTF8("Lapis Lazuli Ore");
	elements[String::fromUTF8("tile.blockLapis.name")] = String::fromUTF8("Lapis Lazuli Block");
	elements[String::fromUTF8("tile.dispenser.name")] = String::fromUTF8("Dispenser");
	elements[String::fromUTF8("tile.musicBlock.name")] = String::fromUTF8("Note Block");
	elements[String::fromUTF8("tile.cake.name")] = String::fromUTF8("Cake");
	
	// Colored wool
	elements[String::fromUTF8("tile.cloth.black.name")] = String::fromUTF8("Black Wool");
	elements[String::fromUTF8("tile.cloth.red.name")] = String::fromUTF8("Red Wool");
	elements[String::fromUTF8("tile.cloth.green.name")] = String::fromUTF8("Green Wool");
	elements[String::fromUTF8("tile.cloth.brown.name")] = String::fromUTF8("Brown Wool");
	elements[String::fromUTF8("tile.cloth.blue.name")] = String::fromUTF8("Blue Wool");
	elements[String::fromUTF8("tile.cloth.purple.name")] = String::fromUTF8("Purple Wool");
	elements[String::fromUTF8("tile.cloth.cyan.name")] = String::fromUTF8("Cyan Wool");
	elements[String::fromUTF8("tile.cloth.silver.name")] = String::fromUTF8("Light Gray Wool");
	elements[String::fromUTF8("tile.cloth.gray.name")] = String::fromUTF8("Gray Wool");
	elements[String::fromUTF8("tile.cloth.pink.name")] = String::fromUTF8("Pink Wool");
	elements[String::fromUTF8("tile.cloth.lime.name")] = String::fromUTF8("Lime Wool");
	elements[String::fromUTF8("tile.cloth.yellow.name")] = String::fromUTF8("Yellow Wool");
	elements[String::fromUTF8("tile.cloth.lightBlue.name")] = String::fromUTF8("Light Blue Wool");
	elements[String::fromUTF8("tile.cloth.magenta.name")] = String::fromUTF8("Magenta Wool");
	elements[String::fromUTF8("tile.cloth.orange.name")] = String::fromUTF8("Orange Wool");
	elements[String::fromUTF8("tile.cloth.white.name")] = String::fromUTF8("Wool");
	
	// Items - Tools
	elements[String::fromUTF8("item.shovelIron.name")] = String::fromUTF8("Iron Shovel");
	elements[String::fromUTF8("item.pickaxeIron.name")] = String::fromUTF8("Iron Pickaxe");
	elements[String::fromUTF8("item.hatchetIron.name")] = String::fromUTF8("Iron Axe");
	elements[String::fromUTF8("item.flintAndSteel.name")] = String::fromUTF8("Flint and Steel");
	elements[String::fromUTF8("item.apple.name")] = String::fromUTF8("Apple");
	elements[String::fromUTF8("item.bow.name")] = String::fromUTF8("Bow");
	elements[String::fromUTF8("item.arrow.name")] = String::fromUTF8("Arrow");
	elements[String::fromUTF8("item.coal.name")] = String::fromUTF8("Coal");
	elements[String::fromUTF8("item.charcoal.name")] = String::fromUTF8("Charcoal");
	elements[String::fromUTF8("item.emerald.name")] = String::fromUTF8("Diamond");
	elements[String::fromUTF8("item.ingotIron.name")] = String::fromUTF8("Iron Ingot");
	elements[String::fromUTF8("item.ingotGold.name")] = String::fromUTF8("Gold Ingot");
	elements[String::fromUTF8("item.swordIron.name")] = String::fromUTF8("Iron Sword");
	elements[String::fromUTF8("item.swordWood.name")] = String::fromUTF8("Wooden Sword");
	elements[String::fromUTF8("item.shovelWood.name")] = String::fromUTF8("Wooden Shovel");
	elements[String::fromUTF8("item.pickaxeWood.name")] = String::fromUTF8("Wooden Pickaxe");
	elements[String::fromUTF8("item.hatchetWood.name")] = String::fromUTF8("Wooden Axe");
	elements[String::fromUTF8("item.swordStone.name")] = String::fromUTF8("Stone Sword");
	elements[String::fromUTF8("item.shovelStone.name")] = String::fromUTF8("Stone Shovel");
	elements[String::fromUTF8("item.pickaxeStone.name")] = String::fromUTF8("Stone Pickaxe");
	elements[String::fromUTF8("item.hatchetStone.name")] = String::fromUTF8("Stone Axe");
	elements[String::fromUTF8("item.swordDiamond.name")] = String::fromUTF8("Diamond Sword");
	elements[String::fromUTF8("item.shovelDiamond.name")] = String::fromUTF8("Diamond Shovel");
	elements[String::fromUTF8("item.pickaxeDiamond.name")] = String::fromUTF8("Diamond Pickaxe");
	elements[String::fromUTF8("item.hatchetDiamond.name")] = String::fromUTF8("Diamond Axe");
	elements[String::fromUTF8("item.stick.name")] = String::fromUTF8("Stick");
	elements[String::fromUTF8("item.bowl.name")] = String::fromUTF8("Bowl");
	elements[String::fromUTF8("item.mushroomStew.name")] = String::fromUTF8("Mushroom Stew");
	elements[String::fromUTF8("item.swordGold.name")] = String::fromUTF8("Golden Sword");
	elements[String::fromUTF8("item.shovelGold.name")] = String::fromUTF8("Golden Shovel");
	elements[String::fromUTF8("item.pickaxeGold.name")] = String::fromUTF8("Golden Pickaxe");
	elements[String::fromUTF8("item.hatchetGold.name")] = String::fromUTF8("Golden Axe");
	elements[String::fromUTF8("item.string.name")] = String::fromUTF8("String");
	elements[String::fromUTF8("item.feather.name")] = String::fromUTF8("Feather");
	elements[String::fromUTF8("item.sulphur.name")] = String::fromUTF8("Sulphur");
	elements[String::fromUTF8("item.hoeWood.name")] = String::fromUTF8("Wooden Hoe");
	elements[String::fromUTF8("item.hoeStone.name")] = String::fromUTF8("Stone Hoe");
	elements[String::fromUTF8("item.hoeIron.name")] = String::fromUTF8("Iron Hoe");
	elements[String::fromUTF8("item.hoeDiamond.name")] = String::fromUTF8("Diamond Hoe");
	elements[String::fromUTF8("item.hoeGold.name")] = String::fromUTF8("Golden Hoe");
	elements[String::fromUTF8("item.seeds.name")] = String::fromUTF8("Seeds");
	elements[String::fromUTF8("item.wheat.name")] = String::fromUTF8("Wheat");
	elements[String::fromUTF8("item.bread.name")] = String::fromUTF8("Bread");
	
	// Armor
	elements[String::fromUTF8("item.helmetCloth.name")] = String::fromUTF8("Leather Cap");
	elements[String::fromUTF8("item.chestplateCloth.name")] = String::fromUTF8("Leather Tunic");
	elements[String::fromUTF8("item.leggingsCloth.name")] = String::fromUTF8("Leather Pants");
	elements[String::fromUTF8("item.bootsCloth.name")] = String::fromUTF8("Leather Boots");
	elements[String::fromUTF8("item.helmetChain.name")] = String::fromUTF8("Chain Helmet");
	elements[String::fromUTF8("item.chestplateChain.name")] = String::fromUTF8("Chain Chestplate");
	elements[String::fromUTF8("item.leggingsChain.name")] = String::fromUTF8("Chain Leggings");
	elements[String::fromUTF8("item.bootsChain.name")] = String::fromUTF8("Chain Boots");
	elements[String::fromUTF8("item.helmetIron.name")] = String::fromUTF8("Iron Helmet");
	elements[String::fromUTF8("item.chestplateIron.name")] = String::fromUTF8("Iron Chestplate");
	elements[String::fromUTF8("item.leggingsIron.name")] = String::fromUTF8("Iron Leggings");
	elements[String::fromUTF8("item.bootsIron.name")] = String::fromUTF8("Iron Boots");
	elements[String::fromUTF8("item.helmetDiamond.name")] = String::fromUTF8("Diamond Helmet");
	elements[String::fromUTF8("item.chestplateDiamond.name")] = String::fromUTF8("Diamond Chestplate");
	elements[String::fromUTF8("item.leggingsDiamond.name")] = String::fromUTF8("Diamond Leggings");
	elements[String::fromUTF8("item.bootsDiamond.name")] = String::fromUTF8("Diamond Boots");
	elements[String::fromUTF8("item.helmetGold.name")] = String::fromUTF8("Golden Helmet");
	elements[String::fromUTF8("item.chestplateGold.name")] = String::fromUTF8("Golden Chestplate");
	elements[String::fromUTF8("item.leggingsGold.name")] = String::fromUTF8("Golden Leggings");
	elements[String::fromUTF8("item.bootsGold.name")] = String::fromUTF8("Golden boots");
	
	// Other items
	elements[String::fromUTF8("item.flint.name")] = String::fromUTF8("Flint");
	elements[String::fromUTF8("item.porkchopRaw.name")] = String::fromUTF8("Raw Porkchop");
	elements[String::fromUTF8("item.porkchopCooked.name")] = String::fromUTF8("Cooked Porkchop");
	elements[String::fromUTF8("item.painting.name")] = String::fromUTF8("Painting");
	elements[String::fromUTF8("item.appleGold.name")] = String::fromUTF8("Golden Apple");
	elements[String::fromUTF8("item.sign.name")] = String::fromUTF8("Sign");
	elements[String::fromUTF8("item.doorWood.name")] = String::fromUTF8("Wooden Door");
	elements[String::fromUTF8("item.bucket.name")] = String::fromUTF8("Bucket");
	elements[String::fromUTF8("item.bucketWater.name")] = String::fromUTF8("Water Bucket");
	elements[String::fromUTF8("item.bucketLava.name")] = String::fromUTF8("Lava bucket");
	elements[String::fromUTF8("item.minecart.name")] = String::fromUTF8("Minecart");
	elements[String::fromUTF8("item.saddle.name")] = String::fromUTF8("Saddle");
	elements[String::fromUTF8("item.doorIron.name")] = String::fromUTF8("Iron Door");
	elements[String::fromUTF8("item.redstone.name")] = String::fromUTF8("Redstone");
	elements[String::fromUTF8("item.snowball.name")] = String::fromUTF8("Snowball");
	elements[String::fromUTF8("item.boat.name")] = String::fromUTF8("Boat");
	elements[String::fromUTF8("item.leather.name")] = String::fromUTF8("Leather");
	elements[String::fromUTF8("item.milk.name")] = String::fromUTF8("Milk");
	elements[String::fromUTF8("item.brick.name")] = String::fromUTF8("Brick");
	elements[String::fromUTF8("item.clay.name")] = String::fromUTF8("Clay");
	elements[String::fromUTF8("item.reeds.name")] = String::fromUTF8("Sugar Canes");
	elements[String::fromUTF8("item.paper.name")] = String::fromUTF8("Paper");
	elements[String::fromUTF8("item.book.name")] = String::fromUTF8("Book");
	elements[String::fromUTF8("item.slimeball.name")] = String::fromUTF8("Slimeball");
	elements[String::fromUTF8("item.minecartChest.name")] = String::fromUTF8("Minecart with Chest");
	elements[String::fromUTF8("item.minecartFurnace.name")] = String::fromUTF8("Minecart with Furnace");
	elements[String::fromUTF8("item.egg.name")] = String::fromUTF8("Egg");
	elements[String::fromUTF8("item.compass.name")] = String::fromUTF8("Compass");
	elements[String::fromUTF8("item.fishingRod.name")] = String::fromUTF8("Fishing Rod");
	elements[String::fromUTF8("item.clock.name")] = String::fromUTF8("Clock");
	elements[String::fromUTF8("item.yellowDust.name")] = String::fromUTF8("Glowstone Dust");
	elements[String::fromUTF8("item.fishRaw.name")] = String::fromUTF8("Raw Fish");
	elements[String::fromUTF8("item.fishCooked.name")] = String::fromUTF8("Cooked Fish");
	elements[String::fromUTF8("item.record.name")] = String::fromUTF8("Music Disc");
	elements[String::fromUTF8("item.bone.name")] = String::fromUTF8("Bone");
	
	// Dyes
	elements[String::fromUTF8("item.dyePowder.black.name")] = String::fromUTF8("Ink Sac");
	elements[String::fromUTF8("item.dyePowder.red.name")] = String::fromUTF8("Rose Red");
	elements[String::fromUTF8("item.dyePowder.green.name")] = String::fromUTF8("Cactus Green");
	elements[String::fromUTF8("item.dyePowder.brown.name")] = String::fromUTF8("Cocoa Beans");
	elements[String::fromUTF8("item.dyePowder.blue.name")] = String::fromUTF8("Lapis Lazuli");
	elements[String::fromUTF8("item.dyePowder.purple.name")] = String::fromUTF8("Purple Dye");
	elements[String::fromUTF8("item.dyePowder.cyan.name")] = String::fromUTF8("Cyan Dye");
	elements[String::fromUTF8("item.dyePowder.silver.name")] = String::fromUTF8("Light Gray Dye");
	elements[String::fromUTF8("item.dyePowder.gray.name")] = String::fromUTF8("Gray Dye");
	elements[String::fromUTF8("item.dyePowder.pink.name")] = String::fromUTF8("Pink Dye");
	elements[String::fromUTF8("item.dyePowder.lime.name")] = String::fromUTF8("Lime Dye");
	elements[String::fromUTF8("item.dyePowder.yellow.name")] = String::fromUTF8("Dandelion Yellow");
	elements[String::fromUTF8("item.dyePowder.lightBlue.name")] = String::fromUTF8("Light Blue Dye");
	elements[String::fromUTF8("item.dyePowder.magenta.name")] = String::fromUTF8("Magenta Dye");
	elements[String::fromUTF8("item.dyePowder.orange.name")] = String::fromUTF8("Orange Dye");
	elements[String::fromUTF8("item.dyePowder.white.name")] = String::fromUTF8("Bone Meal");
	
	elements[String::fromUTF8("item.sugar.name")] = String::fromUTF8("Sugar");
	elements[String::fromUTF8("item.cake.name")] = String::fromUTF8("Cake");
}

Language &Language::getInstance()
{
	return singleton;
}

jstring Language::getElement(const jstring &elementId) const
{
	auto it = elements.find(elementId);
	if (it != elements.end())
		return it->second;
	return u"";
}

jstring Language::getElementName(const jstring &elementId) const
{
	return getElement(elementId + u".name");
}

jstring Language::getElementDesc(const jstring &elementId) const
{
	return getElement(elementId + u".desc");
}
