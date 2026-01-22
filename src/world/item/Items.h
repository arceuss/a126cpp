#pragma once

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
#include "world/item/ItemBucket.h"
#include "world/item/ItemMinecart.h"
#include "world/item/ItemSaddle.h"
#include "world/item/ItemRedstone.h"
#include "world/item/ItemSnowball.h"
#include "world/item/ItemBoat.h"
#include "world/item/ItemReed.h"
#include "world/item/ItemFishingRod.h"
#include "world/item/ItemRecord.h"

// Alpha 1.2.6 Item registry initialization
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/Item.java
// Note: Alpha uses "diamond" naming (Beta uses "emerald" internally)
namespace Items
{
	// Tools - Steel/Iron tier (tier 2)
	extern ItemSpade *shovelSteel;         // ID 0 -> shiftedIndex 256 (Item.java:8)
	extern ItemPickaxe *pickaxeSteel;      // ID 1 -> shiftedIndex 257 (Item.java:9)
	extern ItemAxe *axeSteel;              // ID 2 -> shiftedIndex 258 (Item.java:10)
	
	// Special tools
	extern ItemFlintAndSteel *flintAndSteel;  // ID 3 -> shiftedIndex 259 (Item.java:11)
	
	// Food
	extern ItemFood *appleRed;             // ID 4 -> shiftedIndex 260 (Item.java:12)
	
	// Combat
	extern ItemBow *bow;                   // ID 5 -> shiftedIndex 261 (Item.java:13)
	extern Item *arrow;                    // ID 6 -> shiftedIndex 262 (Item.java:14)
	
	// Ore drops
	extern Item *coal;                     // ID 7 -> shiftedIndex 263 (Item.java:15)
	extern Item *diamond;                  // ID 8 -> shiftedIndex 264 (Item.java:16) - Alpha uses "diamond" not "emerald"
	extern Item *ingotIron;                // ID 9 -> shiftedIndex 265 (Item.java:17)
	extern Item *ingotGold;                // ID 10 -> shiftedIndex 266 (Item.java:18)
	
	// Swords
	extern ItemSword *swordSteel;          // ID 11 -> shiftedIndex 267 (Item.java:19)
	extern ItemSword *swordWood;           // ID 12 -> shiftedIndex 268 (Item.java:20)
	
	// Tools - Wood tier (tier 0)
	extern ItemSpade *shovelWood;          // ID 13 -> shiftedIndex 269 (Item.java:21)
	extern ItemPickaxe *pickaxeWood;       // ID 14 -> shiftedIndex 270 (Item.java:22)
	extern ItemAxe *axeWood;               // ID 15 -> shiftedIndex 271 (Item.java:23)
	
	// Tools - Stone tier (tier 1)
	extern ItemSword *swordStone;          // ID 16 -> shiftedIndex 272 (Item.java:24)
	extern ItemSpade *shovelStone;         // ID 17 -> shiftedIndex 273 (Item.java:25)
	extern ItemPickaxe *pickaxeStone;      // ID 18 -> shiftedIndex 274 (Item.java:26)
	extern ItemAxe *axeStone;              // ID 19 -> shiftedIndex 275 (Item.java:27)
	
	// Tools - Diamond tier (tier 3)
	extern ItemSword *swordDiamond;        // ID 20 -> shiftedIndex 276 (Item.java:28)
	extern ItemSpade *shovelDiamond;       // ID 21 -> shiftedIndex 277 (Item.java:29)
	extern ItemPickaxe *pickaxeDiamond;    // ID 22 -> shiftedIndex 278 (Item.java:30)
	extern ItemAxe *axeDiamond;            // ID 23 -> shiftedIndex 279 (Item.java:31)
	
	// Basic items
	extern Item *stick;                    // ID 24 -> shiftedIndex 280 (Item.java:32)
	extern Item *bowlEmpty;                // ID 25 -> shiftedIndex 281 (Item.java:33)
	extern Item *bowlSoup;                 // ID 26 -> shiftedIndex 282 (Item.java:34) - ItemSoup
	
	// Tools - Gold tier (tier 0, but different stats)
	extern ItemSword *swordGold;           // ID 27 -> shiftedIndex 283 (Item.java:35)
	extern ItemSpade *shovelGold;          // ID 28 -> shiftedIndex 284 (Item.java:36)
	extern ItemPickaxe *pickaxeGold;       // ID 29 -> shiftedIndex 285 (Item.java:37)
	extern ItemAxe *axeGold;               // ID 30 -> shiftedIndex 286 (Item.java:38)
	
	// Materials
	extern Item *silk;                     // ID 31 -> shiftedIndex 287 (Item.java:39) - Alpha name, Beta calls it "string"
	extern Item *feather;                  // ID 32 -> shiftedIndex 288 (Item.java:40)
	extern Item *gunpowder;                 // ID 33 -> shiftedIndex 289 (Item.java:41) - Alpha name, Beta calls it "sulphur"
	
	// Hoes (tier 0-3)
	extern ItemHoe *hoeWood;               // ID 34 -> shiftedIndex 290 (Item.java:42)
	extern ItemHoe *hoeStone;               // ID 35 -> shiftedIndex 291 (Item.java:43)
	extern ItemHoe *hoeSteel;               // ID 36 -> shiftedIndex 292 (Item.java:44)
	extern ItemHoe *hoeDiamond;            // ID 37 -> shiftedIndex 293 (Item.java:45)
	extern ItemHoe *hoeGold;               // ID 38 -> shiftedIndex 294 (Item.java:46)
	
	// Farming
	extern ItemSeeds *seeds;               // ID 39 -> shiftedIndex 295 (Item.java:47)
	extern Item *wheat;                    // ID 40 -> shiftedIndex 296 (Item.java:48)
	extern ItemFood *bread;                 // ID 41 -> shiftedIndex 297 (Item.java:49)
	
	// Armor - Leather (tier 0)
	extern ItemArmor *helmetLeather;       // ID 42 -> shiftedIndex 298 (Item.java:50)
	extern ItemArmor *plateLeather;        // ID 43 -> shiftedIndex 299 (Item.java:51)
	extern ItemArmor *legsLeather;         // ID 44 -> shiftedIndex 300 (Item.java:52)
	extern ItemArmor *bootsLeather;        // ID 45 -> shiftedIndex 301 (Item.java:53)
	
	// Armor - Chain (tier 0, different icons)
	extern ItemArmor *helmetChain;         // ID 46 -> shiftedIndex 302 (Item.java:54)
	extern ItemArmor *plateChain;          // ID 47 -> shiftedIndex 303 (Item.java:55)
	extern ItemArmor *legsChain;           // ID 48 -> shiftedIndex 304 (Item.java:56)
	extern ItemArmor *bootsChain;          // ID 49 -> shiftedIndex 305 (Item.java:57)
	
	// Armor - Steel/Iron (tier 2)
	extern ItemArmor *helmetSteel;         // ID 50 -> shiftedIndex 306 (Item.java:58)
	extern ItemArmor *plateSteel;          // ID 51 -> shiftedIndex 307 (Item.java:59)
	extern ItemArmor *legsSteel;           // ID 52 -> shiftedIndex 308 (Item.java:60)
	extern ItemArmor *bootsSteel;          // ID 53 -> shiftedIndex 309 (Item.java:61)
	
	// Armor - Diamond (tier 3)
	extern ItemArmor *helmetDiamond;       // ID 54 -> shiftedIndex 310 (Item.java:62)
	extern ItemArmor *plateDiamond;        // ID 55 -> shiftedIndex 311 (Item.java:63)
	extern ItemArmor *legsDiamond;         // ID 56 -> shiftedIndex 312 (Item.java:64)
	extern ItemArmor *bootsDiamond;        // ID 57 -> shiftedIndex 313 (Item.java:65)
	
	// Armor - Gold (tier 0, different icons)
	extern ItemArmor *helmetGold;          // ID 58 -> shiftedIndex 314 (Item.java:66)
	extern ItemArmor *plateGold;           // ID 59 -> shiftedIndex 315 (Item.java:67)
	extern ItemArmor *legsGold;            // ID 60 -> shiftedIndex 316 (Item.java:68)
	extern ItemArmor *bootsGold;           // ID 61 -> shiftedIndex 317 (Item.java:69)
	
	// Misc items
	extern Item *flint;                    // ID 62 -> shiftedIndex 318 (Item.java:70)
	extern ItemFood *porkRaw;              // ID 63 -> shiftedIndex 319 (Item.java:71)
	extern ItemFood *porkCooked;           // ID 64 -> shiftedIndex 320 (Item.java:72)
	extern ItemPainting *painting;         // ID 65 -> shiftedIndex 321 (Item.java:73)
	extern ItemFood *appleGold;            // ID 66 -> shiftedIndex 322 (Item.java:74)
	extern ItemSign *sign;                 // ID 67 -> shiftedIndex 323 (Item.java:75)
	extern ItemDoor *doorWood;             // ID 68 -> shiftedIndex 324 (Item.java:76)
	extern ItemBucket *bucketEmpty;        // ID 69 -> shiftedIndex 325 (Item.java:77)
	extern ItemBucket *bucketWater;        // ID 70 -> shiftedIndex 326 (Item.java:78)
	extern ItemBucket *bucketLava;         // ID 71 -> shiftedIndex 327 (Item.java:79)
	extern ItemMinecart *minecartEmpty;    // ID 72 -> shiftedIndex 328 (Item.java:80)
	extern ItemSaddle *saddle;            // ID 73 -> shiftedIndex 329 (Item.java:81)
	extern ItemDoor *doorSteel;           // ID 74 -> shiftedIndex 330 (Item.java:82)
	extern ItemRedstone *redstone;        // ID 75 -> shiftedIndex 331 (Item.java:83)
	extern ItemSnowball *snowball;        // ID 76 -> shiftedIndex 332 (Item.java:84)
	extern ItemBoat *boat;                 // ID 77 -> shiftedIndex 333 (Item.java:85)
	extern Item *leather;                  // ID 78 -> shiftedIndex 334 (Item.java:86)
	extern ItemBucket *bucketMilk;         // ID 79 -> shiftedIndex 335 (Item.java:87)
	extern Item *brick;                    // ID 80 -> shiftedIndex 336 (Item.java:88)
	extern Item *clay;                     // ID 81 -> shiftedIndex 337 (Item.java:89)
	extern ItemReed *reed;                 // ID 82 -> shiftedIndex 338 (Item.java:90)
	extern Item *paper;                    // ID 83 -> shiftedIndex 339 (Item.java:91)
	extern Item *book;                     // ID 84 -> shiftedIndex 340 (Item.java:92)
	extern Item *slimeBall;                // ID 85 -> shiftedIndex 341 (Item.java:93)
	extern ItemMinecart *minecartCrate;    // ID 86 -> shiftedIndex 342 (Item.java:94)
	extern ItemMinecart *minecartPowered; // ID 87 -> shiftedIndex 343 (Item.java:95)
	extern Item *egg;                      // ID 88 -> shiftedIndex 344 (Item.java:96)
	extern Item *compass;                  // ID 89 -> shiftedIndex 345 (Item.java:97)
	extern ItemFishingRod *fishingRod;     // ID 90 -> shiftedIndex 346 (Item.java:98)
	extern Item *pocketSundial;            // ID 91 -> shiftedIndex 347 (Item.java:99)
	extern Item *lightStoneDust;           // ID 92 -> shiftedIndex 348 (Item.java:100)
	extern ItemFood *fishRaw;               // ID 93 -> shiftedIndex 349 (Item.java:101)
	extern ItemFood *fishCooked;           // ID 94 -> shiftedIndex 350 (Item.java:102)
	
	// Records
	extern ItemRecord *record13;           // ID 2000 -> shiftedIndex 2256 (Item.java:103)
	extern ItemRecord *recordCat;          // ID 2001 -> shiftedIndex 2257 (Item.java:104)
	
	// Additional items (Beta 1.2+)
	extern Item *bone;                     // ID 96 -> shiftedIndex 352 (Item.java:116)
	
	// Backwards compatibility aliases (Beta names -> Alpha names)
	// These allow existing code using Beta names to continue working
	extern Item *ironIngot;   // Beta name -> Alpha: ingotIron
	extern Item *redStone;    // Beta name -> Alpha: redstone
	extern Item *string;      // Beta name -> Alpha: silk
	extern Item *sulphur;     // Beta name -> Alpha: gunpowder
	extern Item *record01;    // Beta name -> Alpha: record13
	extern Item *record02;    // Beta name -> Alpha: recordCat
	extern Item *yellowDust;  // Beta name -> Alpha: lightStoneDust
	extern ItemSword *sword_gold;  // Beta name -> Alpha: swordGold
	extern ItemFood *porkChop_cooked;  // Beta name -> Alpha: porkCooked
	
	// Initialize all items (call once during game startup)
	void initItems();
}
