#include "client/spc/SPCCommand.h"

#include <initializer_list>
#include <limits>
#include <memory>
#include <string>

#include "util/Mth.h"
#include "world/entity/Entity.h"
#include "world/entity/EntityIO.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/player/Player.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"
#include "world/level/Level.h"
#include "world/level/tile/CobblestoneTile.h"
#include "world/level/tile/Tile.h"

std::map<jstring, SPCCommand::Waypoint> SPCCommand::waypoints;
double SPCCommand::previousX = 0.0;
double SPCCommand::previousY = 0.0;
double SPCCommand::previousZ = 0.0;
bool SPCCommand::hasPreviousPosition = false;
jstring SPCCommand::lastCommand;

static jstring lowerCommandAscii(const jstring &value)
{
	jstring result = value;
	for (char16_t &character : result)
	{
		if (character >= u'A' && character <= u'Z')
			character = static_cast<char16_t>(character - u'A' + u'a');
	}
	return result;
}

static std::vector<jstring> splitCommand(const jstring &value)
{
	std::vector<jstring> parts;
	jstring current;
	for (char16_t character : value)
	{
		if (character == u' ' || character == u'\t' || character == u'\r' || character == u'\n')
		{
			if (!current.empty())
			{
				parts.push_back(current);
				current.clear();
			}
		}
		else
		{
			current.push_back(character);
		}
	}
	if (!current.empty())
		parts.push_back(current);
	return parts;
}

static bool parseCommandLong(const jstring &text, long_t &value)
{
	try
	{
		const std::string utf8 = String::toUTF8(text);
		size_t used = 0;
		value = static_cast<long_t>(std::stoll(utf8, &used, 10));
		return used == utf8.size();
	}
	catch (const std::exception &)
	{
		return false;
	}
}

static bool parseCommandDouble(const jstring &text, double &value)
{
	try
	{
		const std::string utf8 = String::toUTF8(text);
		size_t used = 0;
		value = std::stod(utf8, &used);
		return used == utf8.size();
	}
	catch (const std::exception &)
	{
		return false;
	}
}

static SPCCommand::Result commandResult(bool success, const jstring &message)
{
	SPCCommand::Result result;
	result.handled = true;
	result.success = success;
	if (!message.empty())
		result.messages.push_back(message);
	return result;
}

// Alpha's Item.java and Block.java carry no name identifiers, so the beta
// port's automatic descriptionId and Language lookup cannot be reproduced
// here. This table spells out the same vocabulary the beta port accepts,
// restricted to blocks and items that exist in Alpha 1.2.6 (Item.java ends at
// id 94 plus the two records; Block.java leaves ids 21-34 and 36 null and ends
// at 91).
static bool commandNameHasPrefix(const jstring &value, const jstring &prefix)
{
	return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

static jstring normalizeCommandName(const jstring &value)
{
	jstring lowered = lowerCommandAscii(value);
	if (commandNameHasPrefix(lowered, u"item."))
		lowered = lowered.substr(5);
	else if (commandNameHasPrefix(lowered, u"tile."))
		lowered = lowered.substr(5);

	jstring result;
	result.reserve(lowered.size());
	for (char16_t character : lowered)
	{
		if ((character >= u'a' && character <= u'z') || (character >= u'0' && character <= u'9'))
			result.push_back(character);
	}
	return result;
}

static void addCommandAlias(std::map<jstring, int_t> &aliases, const jstring &name, int_t id)
{
	const jstring normalized = normalizeCommandName(name);
	if (!normalized.empty())
		aliases[normalized] = id;
}

static void addCommandItemAliases(std::map<jstring, int_t> &aliases, Item *item,
	std::initializer_list<const char16_t *> names)
{
	if (item == nullptr)
		return;
	for (const char16_t *name : names)
		addCommandAlias(aliases, jstring(name), item->getShiftedIndex());
}

// Tiles are registered after items, so a shared name resolves to the block,
// which is the order the beta port uses.
static void addCommandTileAliases(std::map<jstring, int_t> &aliases, int_t tileId,
	std::initializer_list<const char16_t *> names)
{
	if (tileId <= 0 || tileId >= 256 || Tile::tiles[static_cast<size_t>(tileId)] == nullptr)
		return;
	for (const char16_t *name : names)
		addCommandAlias(aliases, jstring(name), tileId);
}

static const std::map<jstring, int_t> &commandItemAliases()
{
	static std::map<jstring, int_t> aliases;
	static bool initialized = false;
	if (initialized)
		return aliases;
	// A command can only run with a live level, so the registries are built by
	// then; refuse to cache an empty table if that ever stops holding.
	if (Items::stick == nullptr)
		return aliases;
	initialized = true;

	addCommandItemAliases(aliases, Items::flintAndSteel, { u"flintandsteel", u"flintsteel" });
	addCommandItemAliases(aliases, Items::ingotIron, { u"ingotiron", u"iron", u"ironingot" });
	addCommandItemAliases(aliases, Items::ingotGold, { u"ingotgold", u"gold", u"goldingot" });
	addCommandItemAliases(aliases, Items::stick, { u"stick", u"sticks" });
	addCommandItemAliases(aliases, Items::seeds, { u"seeds", u"seed" });
	addCommandItemAliases(aliases, Items::wheat, { u"wheat" });
	addCommandItemAliases(aliases, Items::bread, { u"bread" });
	addCommandItemAliases(aliases, Items::reed, { u"reeds", u"sugarcane" });
	addCommandItemAliases(aliases, Items::coal, { u"coal" });
	// Alpha calls this diamond where beta's name id says emerald.
	addCommandItemAliases(aliases, Items::diamond, { u"emerald", u"diamond" });
	addCommandItemAliases(aliases, Items::redstone, { u"redstone", u"reddust" });
	addCommandItemAliases(aliases, Items::flint, { u"flint" });
	addCommandItemAliases(aliases, Items::leather, { u"leather" });
	addCommandItemAliases(aliases, Items::silk, { u"string", u"silk" });
	addCommandItemAliases(aliases, Items::feather, { u"feather" });
	addCommandItemAliases(aliases, Items::gunpowder, { u"sulphur", u"gunpowder" });
	addCommandItemAliases(aliases, Items::bowlEmpty, { u"bowl" });
	addCommandItemAliases(aliases, Items::brick, { u"brick", u"brickitem" });
	addCommandItemAliases(aliases, Items::clay, { u"clay" });
	addCommandItemAliases(aliases, Items::paper, { u"paper" });
	addCommandItemAliases(aliases, Items::book, { u"book" });
	addCommandItemAliases(aliases, Items::compass, { u"compass" });
	addCommandItemAliases(aliases, Items::pocketSundial, { u"clock", u"watch", u"pocketsundial" });

	addCommandItemAliases(aliases, Items::swordWood, { u"swordwood", u"woodsword", u"woodensword" });
	addCommandItemAliases(aliases, Items::shovelWood, { u"shovelwood", u"woodshovel", u"woodenspade" });
	addCommandItemAliases(aliases, Items::pickaxeWood, { u"pickaxewood", u"woodpickaxe", u"woodenpickaxe" });
	addCommandItemAliases(aliases, Items::axeWood, { u"hatchetwood", u"woodaxe", u"woodenaxe" });
	addCommandItemAliases(aliases, Items::hoeWood, { u"hoewood", u"woodhoe", u"woodenhoe" });
	addCommandItemAliases(aliases, Items::swordStone, { u"swordstone", u"stonesword" });
	addCommandItemAliases(aliases, Items::shovelStone, { u"shovelstone", u"stoneshovel", u"stonespade" });
	addCommandItemAliases(aliases, Items::pickaxeStone, { u"pickaxestone", u"stonepickaxe" });
	addCommandItemAliases(aliases, Items::axeStone, { u"hatchetstone", u"stoneaxe" });
	addCommandItemAliases(aliases, Items::hoeStone, { u"hoestone", u"stonehoe" });
	// Alpha names the iron tier "steel".
	addCommandItemAliases(aliases, Items::swordSteel, { u"swordiron", u"ironsword" });
	addCommandItemAliases(aliases, Items::shovelSteel, { u"shoveliron", u"ironshovel", u"ironspade" });
	addCommandItemAliases(aliases, Items::pickaxeSteel, { u"pickaxeiron", u"ironpickaxe" });
	addCommandItemAliases(aliases, Items::axeSteel, { u"hatchetiron", u"ironaxe" });
	addCommandItemAliases(aliases, Items::hoeSteel, { u"hoeiron", u"ironhoe" });
	addCommandItemAliases(aliases, Items::swordDiamond, { u"sworddiamond", u"diamondsword" });
	addCommandItemAliases(aliases, Items::shovelDiamond, { u"shoveldiamond", u"diamondshovel", u"diamondspade" });
	addCommandItemAliases(aliases, Items::pickaxeDiamond, { u"pickaxediamond", u"diamondpickaxe" });
	addCommandItemAliases(aliases, Items::axeDiamond, { u"hatchetdiamond", u"diamondaxe" });
	addCommandItemAliases(aliases, Items::hoeDiamond, { u"hoediamond", u"diamondhoe" });
	addCommandItemAliases(aliases, Items::swordGold, { u"swordgold", u"goldsword", u"goldensword" });
	addCommandItemAliases(aliases, Items::shovelGold, { u"shovelgold", u"goldshovel", u"goldenspade" });
	addCommandItemAliases(aliases, Items::pickaxeGold, { u"pickaxegold", u"goldpickaxe", u"goldenpickaxe" });
	addCommandItemAliases(aliases, Items::axeGold, { u"hatchetgold", u"goldaxe", u"goldenaxe" });
	addCommandItemAliases(aliases, Items::hoeGold, { u"hoegold", u"goldhoe", u"goldenhoe" });

	addCommandItemAliases(aliases, Items::appleRed, { u"apple" });
	addCommandItemAliases(aliases, Items::appleGold, { u"applegold", u"goldapple", u"goldenapple" });
	addCommandItemAliases(aliases, Items::bow, { u"bow" });
	addCommandItemAliases(aliases, Items::arrow, { u"arrow", u"arrows" });
	addCommandItemAliases(aliases, Items::porkRaw, { u"porkchopraw", u"pork", u"rawpork", u"porkchop", u"rawporkchop" });
	addCommandItemAliases(aliases, Items::porkCooked, { u"porkchopcooked", u"cookedpork", u"grilledpork", u"cookedporkchop" });
	addCommandItemAliases(aliases, Items::fishRaw, { u"fishraw", u"fish", u"rawfish" });
	addCommandItemAliases(aliases, Items::fishCooked, { u"fishcooked", u"cookedfish" });
	addCommandItemAliases(aliases, Items::bowlSoup, { u"mushroomstew", u"soup", u"mushroomsoup", u"stew" });
	addCommandItemAliases(aliases, Items::egg, { u"egg", u"eggs" });
	addCommandItemAliases(aliases, Items::painting, { u"painting" });
	addCommandItemAliases(aliases, Items::sign, { u"sign" });
	addCommandItemAliases(aliases, Items::doorWood, { u"doorwood", u"door", u"woodendoor", u"wooddoor" });
	addCommandItemAliases(aliases, Items::doorSteel, { u"dooriron", u"irondoor" });
	addCommandItemAliases(aliases, Items::minecartEmpty, { u"minecart", u"cart" });
	addCommandItemAliases(aliases, Items::minecartPowered, { u"minecartfurnace", u"poweredminecart", u"furnacecart", u"furnaceminecart" });
	addCommandItemAliases(aliases, Items::minecartCrate, { u"minecartchest", u"chestcart", u"chestminecart", u"storageminecart" });
	addCommandItemAliases(aliases, Items::boat, { u"boat" });
	addCommandItemAliases(aliases, Items::saddle, { u"saddle" });
	addCommandItemAliases(aliases, Items::fishingRod, { u"fishingrod", u"rod", u"fishingpole" });
	addCommandItemAliases(aliases, Items::snowball, { u"snowball", u"snowballs" });
	addCommandItemAliases(aliases, Items::slimeBall, { u"slimeball", u"slime" });
	addCommandItemAliases(aliases, Items::lightStoneDust, { u"yellowdust", u"glowstonedust", u"glowdust", u"lightstonedust" });
	addCommandItemAliases(aliases, Items::bucketEmpty, { u"bucket" });
	addCommandItemAliases(aliases, Items::bucketWater, { u"bucketwater", u"waterbucket" });
	addCommandItemAliases(aliases, Items::bucketLava, { u"bucketlava", u"lavabucket" });
	addCommandItemAliases(aliases, Items::bucketMilk, { u"milk", u"milkbucket" });
	addCommandItemAliases(aliases, Items::record13, { u"record", u"gold13record", u"goldrecord" });
	addCommandItemAliases(aliases, Items::recordCat, { u"catrecord", u"greenrecord" });

	addCommandItemAliases(aliases, Items::helmetLeather, { u"helmetcloth", u"leatherhelmet", u"leathercap" });
	addCommandItemAliases(aliases, Items::plateLeather, { u"chestplatecloth", u"leatherchestplate", u"leathertunic" });
	addCommandItemAliases(aliases, Items::legsLeather, { u"leggingscloth", u"leatherleggings", u"leatherpants" });
	addCommandItemAliases(aliases, Items::bootsLeather, { u"bootscloth", u"leatherboots" });
	addCommandItemAliases(aliases, Items::helmetChain, { u"helmetchain", u"chainhelmet", u"chainmailhelmet" });
	addCommandItemAliases(aliases, Items::plateChain, { u"chestplatechain", u"chainchestplate", u"chainmailchestplate" });
	addCommandItemAliases(aliases, Items::legsChain, { u"leggingschain", u"chainleggings", u"chainmailleggings" });
	addCommandItemAliases(aliases, Items::bootsChain, { u"bootschain", u"chainboots", u"chainmailboots" });
	addCommandItemAliases(aliases, Items::helmetSteel, { u"helmetiron", u"ironhelmet" });
	addCommandItemAliases(aliases, Items::plateSteel, { u"chestplateiron", u"ironchestplate", u"ironplate" });
	addCommandItemAliases(aliases, Items::legsSteel, { u"leggingsiron", u"ironleggings", u"ironpants" });
	addCommandItemAliases(aliases, Items::bootsSteel, { u"bootsiron", u"ironboots" });
	addCommandItemAliases(aliases, Items::helmetDiamond, { u"helmetdiamond", u"diamondhelmet" });
	addCommandItemAliases(aliases, Items::plateDiamond, { u"chestplatediamond", u"diamondchestplate", u"diamondplate" });
	addCommandItemAliases(aliases, Items::legsDiamond, { u"leggingsdiamond", u"diamondleggings", u"diamondpants" });
	addCommandItemAliases(aliases, Items::bootsDiamond, { u"bootsdiamond", u"diamondboots" });
	addCommandItemAliases(aliases, Items::helmetGold, { u"helmetgold", u"goldhelmet", u"goldenhelmet" });
	addCommandItemAliases(aliases, Items::plateGold, { u"chestplategold", u"goldchestplate", u"goldenchestplate" });
	addCommandItemAliases(aliases, Items::legsGold, { u"leggingsgold", u"goldleggings", u"goldenleggings" });
	addCommandItemAliases(aliases, Items::bootsGold, { u"bootsgold", u"goldboots", u"goldenboots" });

	addCommandTileAliases(aliases, 1, { u"stone" });
	addCommandTileAliases(aliases, 2, { u"grass" });
	addCommandTileAliases(aliases, 3, { u"dirt" });
	addCommandTileAliases(aliases, 5, { u"wood", u"planks", u"plank" });
	addCommandTileAliases(aliases, 12, { u"sand" });
	addCommandTileAliases(aliases, 13, { u"gravel" });
	addCommandTileAliases(aliases, 17, { u"log", u"tree", u"trunk" });
	addCommandTileAliases(aliases, 18, { u"leaf", u"leaves" });
	addCommandTileAliases(aliases, 37, { u"yellowflower", u"flower" });
	addCommandTileAliases(aliases, 38, { u"rose", u"redflower" });
	addCommandTileAliases(aliases, 39, { u"brownmushroom" });
	addCommandTileAliases(aliases, 40, { u"redmushroom" });
	addCommandTileAliases(aliases, 4, { u"cobble", u"cobblestone" });
	addCommandTileAliases(aliases, 7, { u"bedrock" });
	addCommandTileAliases(aliases, 8, { u"water" });
	addCommandTileAliases(aliases, 10, { u"lava" });
	addCommandTileAliases(aliases, 14, { u"goldore" });
	addCommandTileAliases(aliases, 15, { u"ironore" });
	addCommandTileAliases(aliases, 16, { u"coalore" });
	addCommandTileAliases(aliases, 48, { u"mossycobblestone", u"mossstone" });
	addCommandTileAliases(aliases, 49, { u"obsidian" });
	addCommandTileAliases(aliases, 43, { u"doubleslab" });
	addCommandTileAliases(aliases, 44, { u"slab", u"halfslab" });
	addCommandTileAliases(aliases, 58, { u"workbench", u"craftingtable" });
	addCommandTileAliases(aliases, 59, { u"crop", u"crops", u"wheatcrop" });
	addCommandTileAliases(aliases, 60, { u"farmland", u"soil" });
	addCommandTileAliases(aliases, 61, { u"furnace" });
	addCommandTileAliases(aliases, 56, { u"diamondore" });
	addCommandTileAliases(aliases, 73, { u"redstoneore" });
	addCommandTileAliases(aliases, 78, { u"snow" });
	addCommandTileAliases(aliases, 79, { u"ice" });
	addCommandTileAliases(aliases, 81, { u"cactus" });
	addCommandTileAliases(aliases, 82, { u"clayblock" });
	addCommandTileAliases(aliases, 83, { u"reedblock" });
	addCommandTileAliases(aliases, 86, { u"pumpkin" });
	addCommandTileAliases(aliases, 50, { u"torch" });
	addCommandTileAliases(aliases, 6, { u"sapling" });
	addCommandTileAliases(aliases, 19, { u"sponge" });
	addCommandTileAliases(aliases, 20, { u"glass" });
	addCommandTileAliases(aliases, 35, { u"wool", u"cloth", u"whitewool" });
	addCommandTileAliases(aliases, 41, { u"goldblock", u"blockofgold" });
	addCommandTileAliases(aliases, 42, { u"ironblock", u"blockofiron" });
	addCommandTileAliases(aliases, 45, { u"brick", u"bricks", u"brickblock" });
	addCommandTileAliases(aliases, 46, { u"tnt" });
	addCommandTileAliases(aliases, 47, { u"bookshelf", u"bookcase" });
	addCommandTileAliases(aliases, 52, { u"mobspawner", u"spawner" });
	addCommandTileAliases(aliases, 53, { u"woodstairs", u"woodenstairs", u"stairs" });
	addCommandTileAliases(aliases, 54, { u"chest" });
	addCommandTileAliases(aliases, 57, { u"diamondblock", u"blockofdiamond" });
	addCommandTileAliases(aliases, 65, { u"ladder" });
	addCommandTileAliases(aliases, 66, { u"rail", u"rails", u"track", u"tracks", u"minecarttrack" });
	addCommandTileAliases(aliases, 67, { u"cobblestairs", u"cobblestonestairs", u"stonestairs" });
	addCommandTileAliases(aliases, 69, { u"lever", u"switch" });
	addCommandTileAliases(aliases, 70, { u"stoneplate", u"stonepressureplate", u"pressureplate" });
	addCommandTileAliases(aliases, 72, { u"woodplate", u"woodenpressureplate", u"woodpressureplate" });
	addCommandTileAliases(aliases, 76, { u"redstonetorch", u"redtorch" });
	addCommandTileAliases(aliases, 77, { u"button", u"stonebutton" });
	addCommandTileAliases(aliases, 80, { u"snowblock" });
	addCommandTileAliases(aliases, 84, { u"jukebox" });
	addCommandTileAliases(aliases, 85, { u"fence", u"woodfence", u"woodenfence" });
	addCommandTileAliases(aliases, 87, { u"netherrack", u"netherstone", u"hellrock", u"bloodstone" });
	addCommandTileAliases(aliases, 88, { u"soulsand", u"slowsand" });
	addCommandTileAliases(aliases, 89, { u"glowstone", u"lightstone", u"glowstoneblock" });
	addCommandTileAliases(aliases, 91, { u"jackolantern", u"pumpkinlantern" });

	// Kept from the previous local table; the beta port never listed fire.
	addCommandTileAliases(aliases, 51, { u"fire" });
	return aliases;
}

static int_t commandItemId(const jstring &input)
{
	long_t numeric = 0;
	if (parseCommandLong(input, numeric)
		&& numeric > 0 && numeric < static_cast<long_t>(Item::itemsList.size()))
	{
		const int_t id = static_cast<int_t>(numeric);
		if ((id < 256 && Tile::tiles[id] != nullptr)
			|| Item::itemsList[static_cast<size_t>(id)] != nullptr)
			return id;
		return -1;
	}

	const std::map<jstring, int_t> &aliases = commandItemAliases();
	const auto found = aliases.find(normalizeCommandName(input));
	if (found != aliases.end())
		return found->second;
	return -1;
}

static jstring commandEntityId(const jstring &input)
{
	const jstring name = lowerCommandAscii(input);
	if (name == u"pig") return u"Pig";
	if (name == u"sheep") return u"Sheep";
	if (name == u"cow") return u"Cow";
	if (name == u"chicken") return u"Chicken";
	if (name == u"zombie") return u"Zombie";
	if (name == u"skeleton") return u"Skeleton";
	if (name == u"spider") return u"Spider";
	if (name == u"creeper") return u"Creeper";

	if (name == u"slime") return u"Slime";
	if (name == u"pigzombie" || name == u"zombiepigman") return u"PigZombie";
	if (name == u"ghast") return u"Ghast";
	if (name == u"giant") return u"Giant";
	return u"";
}

bool SPCCommand::shouldOpenChat(bool playerLoaded, int_t eventKey, int_t chatKey)
{
	return playerLoaded && eventKey == chatKey;
}

void SPCCommand::resetState()
{
	waypoints.clear();
	previousX = previousY = previousZ = 0.0;
	hasPreviousPosition = false;
	lastCommand.clear();
}

SPCCommand::Result SPCCommand::execute(Level &level, Player &player, const jstring &input)
{
	if (input.empty() || input[0] != u'/')
		return Result();
	if (level.isOnline)
		return commandResult(false, u"Singleplayer commands are disabled in multiplayer");

	jstring commandLine = input.substr(1);
	std::vector<jstring> parts = splitCommand(commandLine);
	if (parts.empty())
		return commandResult(false, u"Empty command");
	const jstring command = lowerCommandAscii(parts[0]);

	if (command == u"repeat")
	{
		if (lastCommand.empty())
			return commandResult(false, u"No command to repeat");
		const jstring repeated = lastCommand;
		return execute(level, player, u"/" + repeated);
	}
	lastCommand = commandLine;

	if (command == u"help" || command == u"?")
	{
		Result result = commandResult(true, u"Commands: give, tp, pos, set/goto/rem/listwaypoints, home, return, time, seed, difficulty, spawn, setspawn, health/heal/hurt/kill, setblock, platform, ascend, descend, removedrops, clearinventory, save, repeat");
		return result;
	}

	if (command == u"give" || command == u"i" || command == u"item")
	{
		if (parts.size() < 2)
			return commandResult(false, u"Usage: give <id|name> [count] [damage]");
		const int_t id = commandItemId(parts[1]);
		if (id < 0)
			return commandResult(false, u"Unknown item: " + parts[1]);
		long_t count = 1;
		long_t damage = 0;
		if (parts.size() >= 3 && (!parseCommandLong(parts[2], count) || count <= 0
			|| count > (std::numeric_limits<int_t>::max)()))
			return commandResult(false, u"Invalid item count");
		if (parts.size() >= 4 && (!parseCommandLong(parts[3], damage)
			|| damage < 0 || damage > (std::numeric_limits<int_t>::max)()))
			return commandResult(false, u"Invalid item damage");
		ItemStack stack(id, static_cast<int_t>(count), static_cast<int_t>(damage));
		player.inventory.addItemStackToInventory(stack);
		if (stack.stackSize > 0)
			return commandResult(false, u"Inventory full; " + String::toString(stack.stackSize) + u" not added");
		return commandResult(true, u"Given " + String::toString(count) + u" of id " + String::toString(id));
	}

	if (command == u"tp" || command == u"tele" || command == u"teleport" || command == u"t")
	{
		if (parts.size() < 4)
			return commandResult(false, u"Usage: tp <x> <y> <z>");
		double x = 0.0, y = 0.0, z = 0.0;
		if (!parseCommandDouble(parts[1], x) || !parseCommandDouble(parts[2], y)
			|| !parseCommandDouble(parts[3], z))
			return commandResult(false, u"Invalid coordinates");
		previousX = player.x; previousY = player.y; previousZ = player.z;
		hasPreviousPosition = true;
		player.setPos(x, y, z);
		return commandResult(true, u"Teleported to " + String::toString(x) + u", " + String::toString(y) + u", " + String::toString(z));
	}

	if (command == u"pos" || command == u"p")
		return commandResult(true, u"Position: " + String::toString(player.x) + u", " + String::toString(player.y) + u", " + String::toString(player.z));

	if (command == u"set" || command == u"s")
	{
		if (parts.size() < 2)
			return commandResult(false, u"Usage: set <waypoint>");
		const jstring name = lowerCommandAscii(parts[1]);
		waypoints[name] = { player.x, player.y, player.z };
		return commandResult(true, u"Waypoint set: " + name);
	}

	if (command == u"goto")
	{
		if (parts.size() < 2)
			return commandResult(false, u"Usage: goto <waypoint>");
		const jstring name = lowerCommandAscii(parts[1]);
		auto found = waypoints.find(name);
		if (found == waypoints.end())
			return commandResult(false, u"Unknown waypoint: " + name);
		previousX = player.x; previousY = player.y; previousZ = player.z;
		hasPreviousPosition = true;
		player.setPos(found->second.x, found->second.y, found->second.z);
		return commandResult(true, u"Warped to " + name);
	}

	if (command == u"rem")
	{
		if (parts.size() < 2)
			return commandResult(false, u"Usage: rem <waypoint>");
		const jstring name = lowerCommandAscii(parts[1]);
		if (waypoints.erase(name) == 0)
			return commandResult(false, u"Unknown waypoint: " + name);
		return commandResult(true, u"Removed waypoint: " + name);
	}

	if (command == u"listwaypoints" || command == u"l")
	{
		Result result = commandResult(true, waypoints.empty() ? u"No waypoints set" : u"Waypoints:");
		for (const auto &entry : waypoints)
			result.messages.push_back(entry.first + u" = " + String::toString(entry.second.x) + u", " + String::toString(entry.second.y) + u", " + String::toString(entry.second.z));
		return result;
	}

	if (command == u"home")
	{
		previousX = player.x; previousY = player.y; previousZ = player.z;
		hasPreviousPosition = true;
		player.setPos(level.xSpawn + 0.5, level.ySpawn + 1.0, level.zSpawn + 0.5);
		return commandResult(true, u"Teleported home");
	}

	if (command == u"return")
	{
		if (!hasPreviousPosition)
			return commandResult(false, u"No previous position saved");
		const double x = previousX, y = previousY, z = previousZ;
		previousX = player.x; previousY = player.y; previousZ = player.z;
		player.setPos(x, y, z);
		return commandResult(true, u"Returned to previous position");
	}

	if (command == u"kill")
	{
		player.health = 0;
		player.die(nullptr);
		return commandResult(true, u"Killed player");
	}

	if (command == u"heal")
	{
		long_t amount = Player::MAX_HEALTH;
		if (parts.size() >= 2 && (!parseCommandLong(parts[1], amount) || amount <= 0))
			return commandResult(false, u"Invalid heal amount");
		player.heal(static_cast<int_t>(amount));
		return commandResult(true, u"Healed " + String::toString(amount));
	}

	if (command == u"hurt")
	{
		long_t amount = 0;
		if (parts.size() < 2 || !parseCommandLong(parts[1], amount) || amount <= 0)
			return commandResult(false, u"Usage: hurt <positive amount>");
		player.hurt(nullptr, static_cast<int_t>(amount));
		return commandResult(true, u"Applied " + String::toString(amount) + u" damage");
	}

	if (command == u"health")
	{
		if (parts.size() == 1)
			return commandResult(true, u"Health: " + String::toString(player.health));
		const jstring value = lowerCommandAscii(parts[1]);
		if (value == u"max") player.health = Player::MAX_HEALTH;
		else if (value == u"min") player.health = 1;
		else
		{
			long_t amount = 0;
			if (!parseCommandLong(parts[1], amount))
				return commandResult(false, u"Usage: health <max|min|value>");
			player.health = static_cast<int_t>(amount);
		}
		return commandResult(true, u"Health set to " + String::toString(player.health));
	}

	if (command == u"time")
	{
		if (parts.size() == 1 || lowerCommandAscii(parts[1]) == u"get")
			return commandResult(true, u"Time: " + String::toString(level.time));
		const jstring mode = lowerCommandAscii(parts[1]);
		const long_t day = level.time / Level::TICKS_PER_DAY;
		if (mode == u"day") level.setTime(day * Level::TICKS_PER_DAY);
		else if (mode == u"night") level.setTime(day * Level::TICKS_PER_DAY + 13000);
		else if (mode == u"set" || mode == u"add")
		{
			long_t ticks = 0;
			if (parts.size() < 3 || !parseCommandLong(parts[2], ticks))
				return commandResult(false, u"Usage: time set|add <ticks>");
			level.setTime(mode == u"set" ? ticks : level.time + ticks);
		}
		else return commandResult(false, u"Usage: time [get|day|night|set|add]");
		return commandResult(true, u"Time: " + String::toString(level.time));
	}

	if (command == u"seed")
		return commandResult(true, u"Seed: " + String::toString(level.seed));

	if (command == u"difficulty" || command == u"diff")
	{
		if (parts.size() == 1)
			return commandResult(true, u"Difficulty: " + String::toString(level.difficulty));
		long_t value = 0;
		if (!parseCommandLong(parts[1], value) || value < 0 || value > 3)
			return commandResult(false, u"Difficulty must be 0-3");
		level.difficulty = static_cast<int_t>(value);
		return commandResult(true, u"Difficulty: " + String::toString(level.difficulty));
	}

	if (command == u"spawn")
	{
		if (parts.size() < 2)
			return commandResult(false, u"Usage: spawn <mob> [count]");
		const jstring entityId = commandEntityId(parts[1]);
		if (entityId.empty())
			return commandResult(false, u"Unknown mob: " + parts[1]);
		long_t count = 1;
		if (parts.size() >= 3 && (!parseCommandLong(parts[2], count) || count <= 0 || count > 1000))
			return commandResult(false, u"Invalid spawn count (1-1000)");
		int_t spawned = 0;
		for (long_t i = 0; i < count; ++i)
		{
			std::shared_ptr<Entity> entity = EntityIO::newEntity(entityId, level);
			if (entity == nullptr)
				return commandResult(false, u"Entity registry rejected " + entityId);
			entity->moveTo(player.x + level.random.nextInt(5), player.y,
				player.z + level.random.nextInt(5), player.yRot, 0.0f);
			if (level.addEntity(entity))
				++spawned;
		}
		return commandResult(true, u"Spawned " + String::toString(spawned) + u" " + entityId);
	}

	if (command == u"setspawn" || command == u"spawnpoint")
	{
		if (parts.size() >= 4)
		{
			long_t x = 0, y = 0, z = 0;
			if (!parseCommandLong(parts[1], x) || !parseCommandLong(parts[2], y)
				|| !parseCommandLong(parts[3], z))
				return commandResult(false, u"Invalid spawn coordinates");
			level.xSpawn = static_cast<int_t>(x);
			level.ySpawn = static_cast<int_t>(y);
			level.zSpawn = static_cast<int_t>(z);
		}
		else
		{
			level.xSpawn = Mth::floor(player.x);
			level.ySpawn = Mth::floor(player.y);
			level.zSpawn = Mth::floor(player.z);
		}
		return commandResult(true, u"Spawn point updated");
	}

	if (command == u"setblock")
	{
		if (parts.size() < 5)
			return commandResult(false, u"Usage: setblock <x> <y> <z> <id|name> [data]");
		long_t x = 0, y = 0, z = 0, data = 0;
		if (!parseCommandLong(parts[1], x) || !parseCommandLong(parts[2], y)
			|| !parseCommandLong(parts[3], z))
			return commandResult(false, u"Invalid block coordinates");
		const int_t id = commandItemId(parts[4]);
		if (id < 0 || id >= 256 || Tile::tiles[id] == nullptr)
			return commandResult(false, u"Unknown block: " + parts[4]);
		if (parts.size() >= 6 && (!parseCommandLong(parts[5], data) || data < 0 || data > 15))
			return commandResult(false, u"Block data must be 0-15");
		if (!level.setTileAndData(static_cast<int_t>(x), static_cast<int_t>(y),
			static_cast<int_t>(z), id, static_cast<int_t>(data)))
			return commandResult(false, u"Block was already in that state");
		return commandResult(true, u"Block set");
	}

	if (command == u"platform")
	{
		level.setTile(Mth::floor(player.x), Mth::floor(player.y) - 1,
			Mth::floor(player.z), Tile::cobblestone.id);
		return commandResult(true, u"Placed cobblestone beneath player");
	}

	if (command == u"ascend" || command == u"descend")
	{
		const int_t x = Mth::floor(player.x);
		const int_t z = Mth::floor(player.z);
		const int_t start = Mth::floor(player.y);
		const int_t direction = command == u"ascend" ? 1 : -1;
		for (int_t y = start + direction; y >= 1 && y < Level::DEPTH - 2; y += direction)
		{
			if (level.isSolidTile(x, y, z) && !level.isSolidTile(x, y + 1, z)
				&& !level.isSolidTile(x, y + 2, z))
			{
				previousX = player.x; previousY = player.y; previousZ = player.z;
				hasPreviousPosition = true;
				player.setPos(x + 0.5, y + 1.0, z + 0.5);
				return commandResult(true, command == u"ascend" ? u"Ascended" : u"Descended");
			}
		}
		return commandResult(false, u"No platform found");
	}

	if (command == u"removedrops")
	{
		std::vector<std::shared_ptr<Entity>> drops;
		for (const std::shared_ptr<Entity> &entity : level.entities)
		{
			if (dynamic_cast<EntityItem *>(entity.get()) != nullptr)
				drops.push_back(entity);
		}
		for (const std::shared_ptr<Entity> &drop : drops)
			level.removeEntity(drop);
		return commandResult(true, u"Removed " + String::toString(static_cast<int_t>(drops.size())) + u" dropped items");
	}

	if (command == u"clearinventory")
	{
		for (ItemStack &stack : player.inventory.mainInventory) stack = ItemStack();
		for (ItemStack &stack : player.inventory.armorInventory) stack = ItemStack();
		return commandResult(true, u"Inventory cleared");
	}

	if (command == u"save")
	{
		level.save(true, nullptr);
		return commandResult(true, u"World saved");
	}

	if (command == u"extinguish" || command == u"ext")
	{
		player.onFire = 0;
		return commandResult(true, u"Player extinguished");
	}

	return commandResult(false, u"Command not found: " + parts[0]);
}
