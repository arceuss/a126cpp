#include "world/item/Items.h"
#include "world/item/Item.h"
#include "world/item/ItemPickaxe.h"
#include "world/item/ItemSpade.h"
#include "world/item/ItemAxe.h"
#include "world/item/ItemSword.h"
#include "world/item/ItemFood.h"
#include "world/item/ItemFlintAndSteel.h"
#include "world/item/ItemBow.h"
#include "world/item/ItemHoe.h"
#include "world/item/ItemSeeds.h"
#include "world/item/ItemArmor.h"
#include "world/item/ItemPainting.h"
#include "world/item/ItemSign.h"
#include "world/item/ItemDoor.h"
#include "world/level/tile/CropTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/tile/ReedTile.h"
#include "world/item/ItemBucket.h"
#include "world/item/ItemMinecart.h"
#include "world/item/ItemSaddle.h"
#include "world/item/ItemRedstone.h"
#include "world/item/ItemSnowball.h"
#include "world/item/ItemBoat.h"
#include "world/item/ItemReed.h"
#include "world/item/ItemFishingRod.h"
#include "world/item/ItemRecord.h"
#include "world/level/tile/Tile.h"
#include "world/level/material/Material.h"

namespace Items
{
	// Tools - Steel/Iron tier (tier 2)
	ItemSpade *shovelSteel = nullptr;
	ItemPickaxe *pickaxeSteel = nullptr;
	ItemAxe *axeSteel = nullptr;
	
	// Special tools
	ItemFlintAndSteel *flintAndSteel = nullptr;
	
	// Food
	ItemFood *appleRed = nullptr;
	
	// Combat
	ItemBow *bow = nullptr;
	Item *arrow = nullptr;
	
	// Ore drops
	Item *coal = nullptr;
	Item *diamond = nullptr;
	Item *ingotIron = nullptr;
	Item *ingotGold = nullptr;
	
	// Swords
	ItemSword *swordSteel = nullptr;
	ItemSword *swordWood = nullptr;
	
	// Tools - Wood tier (tier 0)
	ItemSpade *shovelWood = nullptr;
	ItemPickaxe *pickaxeWood = nullptr;
	ItemAxe *axeWood = nullptr;
	
	// Tools - Stone tier (tier 1)
	ItemSword *swordStone = nullptr;
	ItemSpade *shovelStone = nullptr;
	ItemPickaxe *pickaxeStone = nullptr;
	ItemAxe *axeStone = nullptr;
	
	// Tools - Diamond tier (tier 3)
	ItemSword *swordDiamond = nullptr;
	ItemSpade *shovelDiamond = nullptr;
	ItemPickaxe *pickaxeDiamond = nullptr;
	ItemAxe *axeDiamond = nullptr;
	
	// Basic items
	Item *stick = nullptr;
	Item *bowlEmpty = nullptr;
	Item *bowlSoup = nullptr;
	
	// Tools - Gold tier (tier 0, but different stats)
	ItemSword *swordGold = nullptr;
	ItemSpade *shovelGold = nullptr;
	ItemPickaxe *pickaxeGold = nullptr;
	ItemAxe *axeGold = nullptr;
	
	// Materials
	Item *silk = nullptr;
	Item *feather = nullptr;
	Item *gunpowder = nullptr;
	
	// Hoes (tier 0-3)
	ItemHoe *hoeWood = nullptr;
	ItemHoe *hoeStone = nullptr;
	ItemHoe *hoeSteel = nullptr;
	ItemHoe *hoeDiamond = nullptr;
	ItemHoe *hoeGold = nullptr;
	
	// Farming
	ItemSeeds *seeds = nullptr;
	Item *wheat = nullptr;
	ItemFood *bread = nullptr;
	
	// Armor - Leather (tier 0)
	ItemArmor *helmetLeather = nullptr;
	ItemArmor *plateLeather = nullptr;
	ItemArmor *legsLeather = nullptr;
	ItemArmor *bootsLeather = nullptr;
	
	// Armor - Chain (tier 0, different icons)
	ItemArmor *helmetChain = nullptr;
	ItemArmor *plateChain = nullptr;
	ItemArmor *legsChain = nullptr;
	ItemArmor *bootsChain = nullptr;
	
	// Armor - Steel/Iron (tier 2)
	ItemArmor *helmetSteel = nullptr;
	ItemArmor *plateSteel = nullptr;
	ItemArmor *legsSteel = nullptr;
	ItemArmor *bootsSteel = nullptr;
	
	// Armor - Diamond (tier 3)
	ItemArmor *helmetDiamond = nullptr;
	ItemArmor *plateDiamond = nullptr;
	ItemArmor *legsDiamond = nullptr;
	ItemArmor *bootsDiamond = nullptr;
	
	// Armor - Gold (tier 0, different icons)
	ItemArmor *helmetGold = nullptr;
	ItemArmor *plateGold = nullptr;
	ItemArmor *legsGold = nullptr;
	ItemArmor *bootsGold = nullptr;
	
	// Misc items
	Item *flint = nullptr;
	ItemFood *porkRaw = nullptr;
	ItemFood *porkCooked = nullptr;
	ItemPainting *painting = nullptr;
	ItemFood *appleGold = nullptr;
	ItemSign *sign = nullptr;
	ItemDoor *doorWood = nullptr;
	ItemBucket *bucketEmpty = nullptr;
	ItemBucket *bucketWater = nullptr;
	ItemBucket *bucketLava = nullptr;
	ItemMinecart *minecartEmpty = nullptr;
	ItemSaddle *saddle = nullptr;
	ItemDoor *doorSteel = nullptr;
	ItemRedstone *redstone = nullptr;
	ItemSnowball *snowball = nullptr;
	ItemBoat *boat = nullptr;
	Item *leather = nullptr;
	ItemBucket *bucketMilk = nullptr;
	Item *brick = nullptr;
	Item *clay = nullptr;
	ItemReed *reed = nullptr;
	Item *paper = nullptr;
	Item *book = nullptr;
	Item *slimeBall = nullptr;
	ItemMinecart *minecartCrate = nullptr;
	ItemMinecart *minecartPowered = nullptr;
	Item *egg = nullptr;
	Item *compass = nullptr;
	ItemFishingRod *fishingRod = nullptr;
	Item *pocketSundial = nullptr;
	Item *lightStoneDust = nullptr;
	ItemFood *fishRaw = nullptr;
	ItemFood *fishCooked = nullptr;
	
	// Records
	ItemRecord *record13 = nullptr;
	ItemRecord *recordCat = nullptr;
	
	// Backwards compatibility aliases (Beta names -> Alpha names)
	Item *ironIngot = nullptr;
	Item *redStone = nullptr;
	Item *string = nullptr;
	Item *sulphur = nullptr;
	Item *record01 = nullptr;
	Item *record02 = nullptr;
	Item *yellowDust = nullptr;
	Item *bone = nullptr;  // Beta: Item.java:116 - bone (ID 96)
	
	void initItems()
	{
		// Alpha: Item.java:8-10 - Steel/Iron tier tools (tier 2)
		shovelSteel = new ItemSpade(0, 2);
		shovelSteel->setIconIndex(82);  // Alpha: Item.java:8
		pickaxeSteel = new ItemPickaxe(1, 2);
		pickaxeSteel->setIconIndex(98);  // Alpha: Item.java:9
		axeSteel = new ItemAxe(2, 2);
		axeSteel->setIconIndex(114);  // Alpha: Item.java:10
		
		// Alpha: Item.java:11 - flintAndSteel (ItemFlintAndSteel)
		flintAndSteel = new ItemFlintAndSteel(3);
		flintAndSteel->setIconIndex(5);  // Alpha: Item.java:11
		
		// Alpha: Item.java:12 - appleRed (FoodItem)
		appleRed = new ItemFood(4, 4);
		appleRed->setIconIndex(10);  // Alpha: Item.java:12
		
		// Alpha: Item.java:13-14 - bow and arrow
		bow = new ItemBow(5);
		bow->setIconIndex(21);  // Alpha: Item.java:13
		arrow = new Item(6);
		arrow->setMaxStackSize(64);
		arrow->setIconIndex(37);  // Alpha: Item.java:14
		
		// Alpha: Item.java:15-18 - ore drops and ingots
		coal = new Item(7);
		coal->setMaxStackSize(64);
		coal->setIconIndex(7);  // Alpha: Item.java:15
		diamond = new Item(8);
		diamond->setMaxStackSize(64);
		diamond->setIconIndex(55);  // Alpha: Item.java:16
		ingotIron = new Item(9);
		ingotIron->setMaxStackSize(64);
		ingotIron->setIconIndex(23);  // Alpha: Item.java:17
		ingotGold = new Item(10);
		ingotGold->setMaxStackSize(64);
		ingotGold->setIconIndex(39);  // Alpha: Item.java:18
		
		// Alpha: Item.java:19-20 - swords
		swordSteel = new ItemSword(11, 2);
		swordSteel->setIconIndex(66);  // Alpha: Item.java:19
		swordWood = new ItemSword(12, 0);
		swordWood->setIconIndex(64);  // Alpha: Item.java:20
		
		// Alpha: Item.java:21-23 - Wood tier tools (tier 0)
		shovelWood = new ItemSpade(13, 0);
		shovelWood->setIconIndex(80);  // Alpha: Item.java:21
		pickaxeWood = new ItemPickaxe(14, 0);
		pickaxeWood->setIconIndex(96);  // Alpha: Item.java:22
		axeWood = new ItemAxe(15, 0);
		axeWood->setIconIndex(112);  // Alpha: Item.java:23
		
		// Alpha: Item.java:24-27 - Stone tier tools (tier 1)
		swordStone = new ItemSword(16, 1);
		swordStone->setIconIndex(65);  // Alpha: Item.java:24
		shovelStone = new ItemSpade(17, 1);
		shovelStone->setIconIndex(81);  // Alpha: Item.java:25
		pickaxeStone = new ItemPickaxe(18, 1);
		pickaxeStone->setIconIndex(97);  // Alpha: Item.java:26
		axeStone = new ItemAxe(19, 1);
		axeStone->setIconIndex(113);  // Alpha: Item.java:27
		
		// Alpha: Item.java:28-31 - Diamond tier tools (tier 3)
		swordDiamond = new ItemSword(20, 3);
		swordDiamond->setIconIndex(67);  // Alpha: Item.java:28
		shovelDiamond = new ItemSpade(21, 3);
		shovelDiamond->setIconIndex(83);  // Alpha: Item.java:29
		pickaxeDiamond = new ItemPickaxe(22, 3);
		pickaxeDiamond->setIconIndex(99);  // Alpha: Item.java:30
		axeDiamond = new ItemAxe(23, 3);
		axeDiamond->setIconIndex(115);  // Alpha: Item.java:31
		
		// Alpha: Item.java:32-34 - basic items
		stick = new Item(24);
		stick->setMaxStackSize(64);
		stick->setIconIndex(53);  // Alpha: Item.java:32
		stick->setFull3D();  // Alpha: Item.java:32 - stick.setFull3D()
		bowlEmpty = new Item(25);
		bowlEmpty->setMaxStackSize(64);
		bowlEmpty->setIconIndex(71);  // Alpha: Item.java:33
		bowlSoup = new ItemFood(26, 10);  // ItemSoup extends ItemFood
		bowlSoup->setMaxStackSize(1);
		bowlSoup->setIconIndex(72);  // Alpha: Item.java:34
		
		// Alpha: Item.java:35-38 - Gold tier tools (tier 0, but different stats)
		swordGold = new ItemSword(27, 0);
		swordGold->setIconIndex(68);  // Alpha: Item.java:35
		shovelGold = new ItemSpade(28, 0);
		shovelGold->setIconIndex(84);  // Alpha: Item.java:36
		pickaxeGold = new ItemPickaxe(29, 0);
		pickaxeGold->setIconIndex(100);  // Alpha: Item.java:37
		axeGold = new ItemAxe(30, 0);
		axeGold->setIconIndex(116);  // Alpha: Item.java:38
		
		// Alpha: Item.java:39-41 - materials
		silk = new Item(31);
		silk->setMaxStackSize(64);
		silk->setIconIndex(8);  // Alpha: Item.java:39
		feather = new Item(32);
		feather->setMaxStackSize(64);
		feather->setIconIndex(24);  // Alpha: Item.java:40
		gunpowder = new Item(33);
		gunpowder->setMaxStackSize(64);
		gunpowder->setIconIndex(40);  // Alpha: Item.java:41
		
		// Alpha: Item.java:42-46 - hoes (ItemHoe)
		hoeWood = new ItemHoe(34, 0);
		hoeWood->setIconIndex(128);  // Alpha: Item.java:42
		hoeStone = new ItemHoe(35, 1);
		hoeStone->setIconIndex(129);  // Alpha: Item.java:43
		hoeSteel = new ItemHoe(36, 2);
		hoeSteel->setIconIndex(130);  // Alpha: Item.java:44
		hoeDiamond = new ItemHoe(37, 3);
		hoeDiamond->setIconIndex(131);  // Alpha: Item.java:45
		hoeGold = new ItemHoe(38, 0);
		hoeGold->setIconIndex(132);  // Alpha: Item.java:46
		
		// Alpha: Item.java:47-49 - farming
		// Alpha: ItemSeeds places Tile.crops (wheat) - need to check tile ID
		seeds = new ItemSeeds(39, Tile::crops.id);  // ItemSeeds
		seeds->setIconIndex(9);  // Alpha: Item.java:47
		wheat = new Item(40);
		wheat->setMaxStackSize(64);
		wheat->setIconIndex(25);  // Alpha: Item.java:48
		bread = new ItemFood(41, 5);
		// Note: Alpha FoodItem constructor sets maxStackSize to 1, but bread needs to stack
		// We'll override it after construction
		bread->setMaxStackSize(64);
		bread->setIconIndex(41);  // Alpha: Item.java:49
		
		// Alpha: Item.java:50-53 - Leather armor (ItemArmor, tier 0)
		// Alpha: renderIndex (3rd param) = 0 for cloth/leather, used for texture selection
		// Alpha: iconIndex (setIconIndex) is different - 0, 16, 32, 48 for helmet/plate/legs/boots
		helmetLeather = new ItemArmor(42, 0, 0, 0);  // tier 0, renderIndex 0 (cloth), slot 0 (helmet)
		helmetLeather->setIconIndex(0);  // Alpha: setIconIndex(0) (Item.java:50)
		plateLeather = new ItemArmor(43, 0, 0, 1);   // tier 0, renderIndex 0 (cloth), slot 1 (chestplate)
		plateLeather->setIconIndex(16);  // Alpha: setIconIndex(16) (Item.java:51)
		legsLeather = new ItemArmor(44, 0, 0, 2);    // tier 0, renderIndex 0 (cloth), slot 2 (leggings)
		legsLeather->setIconIndex(32);  // Alpha: setIconIndex(32) (Item.java:52)
		bootsLeather = new ItemArmor(45, 0, 0, 3);   // tier 0, renderIndex 0 (cloth), slot 3 (boots)
		bootsLeather->setIconIndex(48);  // Alpha: setIconIndex(48) (Item.java:53)
		
		// Alpha: Item.java:54-57 - Chain armor (ItemArmor, tier 1, renderIndex 1)
		helmetChain = new ItemArmor(46, 1, 1, 0);  // tier 1, renderIndex 1 (chain), slot 0
		helmetChain->setIconIndex(1);  // Alpha: setIconIndex(1) (Item.java:54)
		plateChain = new ItemArmor(47, 1, 1, 1);   // tier 1, renderIndex 1 (chain), slot 1
		plateChain->setIconIndex(17);  // Alpha: setIconIndex(17) (Item.java:55)
		legsChain = new ItemArmor(48, 1, 1, 2);    // tier 1, renderIndex 1 (chain), slot 2
		legsChain->setIconIndex(33);  // Alpha: setIconIndex(33) (Item.java:56)
		bootsChain = new ItemArmor(49, 1, 1, 3);   // tier 1, renderIndex 1 (chain), slot 3
		bootsChain->setIconIndex(49);  // Alpha: setIconIndex(49) (Item.java:57)
		
		// Alpha: Item.java:58-61 - Steel/Iron armor (ItemArmor, tier 2, renderIndex 2)
		helmetSteel = new ItemArmor(50, 2, 2, 0);  // tier 2, renderIndex 2 (iron), slot 0
		helmetSteel->setIconIndex(2);  // Alpha: setIconIndex(2) (Item.java:58)
		plateSteel = new ItemArmor(51, 2, 2, 1);   // tier 2, renderIndex 2 (iron), slot 1
		plateSteel->setIconIndex(18);  // Alpha: setIconIndex(18) (Item.java:59)
		legsSteel = new ItemArmor(52, 2, 2, 2);    // tier 2, renderIndex 2 (iron), slot 2
		legsSteel->setIconIndex(34);  // Alpha: setIconIndex(34) (Item.java:60)
		bootsSteel = new ItemArmor(53, 2, 2, 3);   // tier 2, renderIndex 2 (iron), slot 3
		bootsSteel->setIconIndex(50);  // Alpha: setIconIndex(50) (Item.java:61)
		
		// Alpha: Item.java:62-65 - Diamond armor (ItemArmor, tier 3, renderIndex 3)
		helmetDiamond = new ItemArmor(54, 3, 3, 0);  // tier 3, renderIndex 3 (diamond), slot 0
		helmetDiamond->setIconIndex(3);  // Alpha: setIconIndex(3) (Item.java:62)
		plateDiamond = new ItemArmor(55, 3, 3, 1);   // tier 3, renderIndex 3 (diamond), slot 1
		plateDiamond->setIconIndex(19);  // Alpha: setIconIndex(19) (Item.java:63)
		legsDiamond = new ItemArmor(56, 3, 3, 2);    // tier 3, renderIndex 3 (diamond), slot 2
		legsDiamond->setIconIndex(35);  // Alpha: setIconIndex(35) (Item.java:64)
		bootsDiamond = new ItemArmor(57, 3, 3, 3);   // tier 3, renderIndex 3 (diamond), slot 3
		bootsDiamond->setIconIndex(51);  // Alpha: setIconIndex(51) (Item.java:65)
		
		// Alpha: Item.java:66-69 - Gold armor (ItemArmor, tier 1, renderIndex 4)
		helmetGold = new ItemArmor(58, 1, 4, 0);  // tier 1, renderIndex 4 (gold), slot 0
		helmetGold->setIconIndex(4);  // Alpha: setIconIndex(4) (Item.java:66)
		plateGold = new ItemArmor(59, 1, 4, 1);   // tier 1, renderIndex 4 (gold), slot 1
		plateGold->setIconIndex(20);  // Alpha: setIconIndex(20) (Item.java:67)
		legsGold = new ItemArmor(60, 1, 4, 2);    // tier 1, renderIndex 4 (gold), slot 2
		legsGold->setIconIndex(36);  // Alpha: setIconIndex(36) (Item.java:68)
		bootsGold = new ItemArmor(61, 1, 4, 3);   // tier 1, renderIndex 4 (gold), slot 3
		bootsGold->setIconIndex(52);  // Alpha: setIconIndex(52) (Item.java:69)
		
		// Alpha: Item.java:70-72 - misc items
		flint = new Item(62);
		flint->setMaxStackSize(64);
		flint->setIconIndex(6);  // Alpha: Item.java:70
		porkRaw = new ItemFood(63, 3);
		porkRaw->setIconIndex(87);  // Alpha: Item.java:71
		porkCooked = new ItemFood(64, 8);
		porkCooked->setIconIndex(88);  // Alpha: Item.java:72
		
		// Alpha: Item.java:73-76 - placeable items
		painting = new ItemPainting(65);
		painting->setIconIndex(26);  // Alpha: Item.java:73
		appleGold = new ItemFood(66, 42);
		appleGold->setIconIndex(11);  // Alpha: Item.java:74
		sign = new ItemSign(67);
		sign->setIconIndex(42);  // Alpha: Item.java:75
		doorWood = new ItemDoor(68, Material::wood);
		doorWood->setIconIndex(43);  // Alpha: Item.java:76
		
		// Alpha: Item.java:77-79 - buckets (ItemBucket)
		// Beta: Buckets place the flowing/source block (ID 8 for water, ID 10 for lava), not the stationary block
		bucketEmpty = new ItemBucket(69, 0);   // content 0 = empty
		bucketEmpty->setIconIndex(74);  // Alpha: Item.java:77
		bucketWater = new ItemBucket(70, Tile::water.id);  // content = flowing water block ID (ID 8, source block)
		bucketWater->setIconIndex(75);  // Alpha: Item.java:78
		bucketLava = new ItemBucket(71, Tile::lava.id);    // content = flowing lava block ID (ID 10, source block)
		bucketLava->setIconIndex(76);  // Alpha: Item.java:79
		
		// Alpha: Item.java:80-82 - minecarts and doors
		minecartEmpty = new ItemMinecart(72, 0);  // type 0 = empty
		minecartEmpty->setIconIndex(135);  // Alpha: Item.java:80
		saddle = new ItemSaddle(73);
		saddle->setIconIndex(104);  // Alpha: Item.java:81
		doorSteel = new ItemDoor(74, Material::metal);
		doorSteel->setIconIndex(44);  // Alpha: Item.java:82
		
		// Alpha: Item.java:83-86 - special items
		redstone = new ItemRedstone(75);
		redstone->setIconIndex(56);  // Alpha: Item.java:83
		snowball = new ItemSnowball(76);
		snowball->setIconIndex(14);  // Alpha: Item.java:84
		boat = new ItemBoat(77);
		boat->setIconIndex(136);  // Alpha: Item.java:85
		leather = new Item(78);
		leather->setMaxStackSize(64);
		leather->setIconIndex(103);  // Alpha: Item.java:86
		
		// Alpha: Item.java:87-93 - more items
		bucketMilk = new ItemBucket(79, -1);  // content -1 = milk (special)
		bucketMilk->setIconIndex(77);  // Alpha: Item.java:87
		brick = new Item(80);
		brick->setMaxStackSize(64);
		brick->setIconIndex(22);  // Alpha: Item.java:88
		clay = new Item(81);
		clay->setMaxStackSize(64);
		clay->setIconIndex(57);  // Alpha: Item.java:89
		reed = new ItemReed(82, Tile::reed.id);  // ItemReed
		reed->setIconIndex(27);  // Alpha: Item.java:90
		paper = new Item(83);
		paper->setMaxStackSize(64);
		paper->setIconIndex(58);  // Alpha: Item.java:91
		book = new Item(84);
		book->setMaxStackSize(64);
		book->setIconIndex(59);  // Alpha: Item.java:92
		slimeBall = new Item(85);
		slimeBall->setMaxStackSize(64);
		slimeBall->setIconIndex(30);  // Alpha: Item.java:93
		
		// Alpha: Item.java:94-99 - minecarts and misc
		minecartCrate = new ItemMinecart(86, 1);  // type 1 = chest
		minecartCrate->setIconIndex(151);  // Alpha: Item.java:94
		minecartPowered = new ItemMinecart(87, 2);  // type 2 = powered
		minecartPowered->setIconIndex(167);  // Alpha: Item.java:95
		egg = new Item(88);
		egg->setMaxStackSize(16);
		egg->setIconIndex(12);  // Alpha: Item.java:96
		compass = new Item(89);
		compass->setMaxStackSize(64);
		compass->setIconIndex(54);  // Alpha: Item.java:97
		fishingRod = new ItemFishingRod(90);
		fishingRod->setIconIndex(69);  // Alpha: Item.java:98
		pocketSundial = new Item(91);
		pocketSundial->setMaxStackSize(64);
		pocketSundial->setIconIndex(70);  // Alpha: Item.java:99
		
		// Alpha: Item.java:100-102 - light stone and fish
		lightStoneDust = new Item(92);
		lightStoneDust->setMaxStackSize(64);
		lightStoneDust->setIconIndex(73);  // Alpha: Item.java:100
		fishRaw = new ItemFood(93, 2);
		fishRaw->setIconIndex(89);  // Alpha: Item.java:101
		fishCooked = new ItemFood(94, 5);
		fishCooked->setIconIndex(90);  // Alpha: Item.java:102
		
		// Alpha: Item.java:103-104 - records (ItemRecord)
		record13 = new ItemRecord(2000, u"13");  // record name "13"
		record13->setIconIndex(240);  // Alpha: Item.java:103
		recordCat = new ItemRecord(2001, u"cat");  // record name "cat"
		recordCat->setIconIndex(241);  // Alpha: Item.java:104
		
		// Beta: Item.java:116 - bone (ID 96)
		bone = new Item(96);
		bone->setMaxStackSize(64);
		bone->setIconIndex(193);  // Icon index 12,1 = 12*16+1 = 193
		
		// Backwards compatibility aliases (Beta names -> Alpha names)
		// These point to the Alpha-named items for code that still uses Beta names
		ironIngot = ingotIron;      // Beta: ironIngot -> Alpha: ingotIron
		redStone = redstone;         // Beta: redStone -> Alpha: redstone
		string = silk;               // Beta: string -> Alpha: silk
		sulphur = gunpowder;         // Beta: sulphur -> Alpha: gunpowder
		record01 = record13;         // Beta: record01 -> Alpha: record13
		record02 = recordCat;        // Beta: record02 -> Alpha: recordCat
		yellowDust = lightStoneDust; // Beta: yellowDust -> Alpha: lightStoneDust
	}
}
