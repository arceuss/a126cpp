#include <memory>

#include "client/spc/SPCCommand.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/player/Player.h"
#include "world/item/Items.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/Tile.h"

static void resetCommands()
{
	SPCCommand::resetState();
}

HEADLESS_TEST(commands, non_commands_and_multiplayer_are_not_executed)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-routing", Dimension::Id_Normal, 6001LL);
	Player player(level);

	SPCCommand::Result chat = SPCCommand::execute(level, player, u"hello");
	ctx.check(!chat.handled, "plain chat is not a command");

	level.isOnline = true;
	const long_t originalTime = level.time;
	SPCCommand::Result online = SPCCommand::execute(level, player, u"/time set 500");
	ctx.check(online.handled && !online.success, "online level rejects local commands");
	ctx.checkEqual(level.time, originalTime, "rejected multiplayer command cannot mutate level");
}

HEADLESS_TEST(commands, give_supports_names_counts_and_damage)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-give", Dimension::Id_Normal, 6002LL);
	Player player(level);

	SPCCommand::Result result = SPCCommand::execute(level, player, u"/give stone 3 2");
	ctx.check(result.handled && result.success, "give command succeeds");
	ItemStack *stack = player.inventory.getCurrentItem();
	if (!ctx.check(stack != nullptr, "given stack enters inventory"))
		return;
	ctx.checkEqual(stack->itemID, Tile::rock.id, "given block id");
	ctx.checkEqual(stack->stackSize, 3, "given count");
	ctx.checkEqual(stack->itemDamage, 2, "given damage/data");
}

HEADLESS_TEST(commands, teleport_waypoint_home_and_return)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-teleport", Dimension::Id_Normal, 6003LL);
	level.xSpawn = 10; level.ySpawn = 70; level.zSpawn = -20;
	Player player(level);
	player.setPos(1.0, 65.0, 2.0);

	ctx.check(SPCCommand::execute(level, player, u"/set test").success, "set waypoint");
	ctx.check(SPCCommand::execute(level, player, u"/tp 20 80 30").success, "teleport command");
	ctx.checkEqualBits(player.x, 20.0, "teleport x");
	ctx.check(SPCCommand::execute(level, player, u"/goto test").success, "goto waypoint");
	ctx.checkEqualBits(player.x, 1.0, "waypoint x");
	ctx.check(SPCCommand::execute(level, player, u"/return").success, "return command");
	ctx.checkEqualBits(player.x, 20.0, "returned x");
	ctx.check(SPCCommand::execute(level, player, u"/home").success, "home command");
	ctx.checkEqualBits(player.x, 10.5, "home x");
	ctx.checkEqualBits(player.y, 71.0, "home y");
}

HEADLESS_TEST(commands, time_difficulty_seed_and_spawnpoint)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-world-state", Dimension::Id_Normal, 6004LL);
	Player player(level);
	player.setPos(4.8, 75.2, -6.1);

	ctx.check(SPCCommand::execute(level, player, u"/time set 1200").success, "time set");
	ctx.check(SPCCommand::execute(level, player, u"/time add 300").success, "time add");
	ctx.checkEqual(level.time, 1500LL, "command time result");
	ctx.check(SPCCommand::execute(level, player, u"/difficulty 3").success, "difficulty command");
	ctx.checkEqual(level.difficulty, 3, "difficulty result");
	ctx.check(SPCCommand::execute(level, player, u"/setspawn").success, "current spawnpoint command");
	ctx.checkEqual(level.xSpawn, 4, "spawn x floor");
	ctx.checkEqual(level.ySpawn, 75, "spawn y floor");
	ctx.checkEqual(level.zSpawn, -7, "spawn z floor");
	SPCCommand::Result seed = SPCCommand::execute(level, player, u"/seed");
	ctx.check(seed.success && !seed.messages.empty(), "seed command produces output");
}

HEADLESS_TEST(commands, spawn_and_setblock_mutate_production_world)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-production", Dimension::Id_Normal, 6005LL);
	Player player(level);
	player.setPos(8.5, 100.0, 8.5);

	SPCCommand::Result spawn = SPCCommand::execute(level, player, u"/spawn pig 2");
	ctx.check(spawn.success, "spawn command succeeds");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 2, "two production Pig entities");
	for (const std::shared_ptr<Entity> &entity : level.entities)
		ctx.checkEqual(entity->getEncodeId(), jstring(u"Pig"), "spawned command entity type");

	SPCCommand::Result block = SPCCommand::execute(level, player, u"/setblock 1 100 1 stone");
	ctx.check(block.success, "setblock command succeeds");
	ctx.checkEqual(level.getTile(1, 100, 1), Tile::rock.id, "setblock production state");
}

HEADLESS_TEST(commands, health_kill_and_inventory_commands)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-player-state", Dimension::Id_Normal, 6006LL);
	Player player(level);
	player.setPos(0.5, 65.0, 0.5);

	ctx.check(SPCCommand::execute(level, player, u"/health 5").success, "health set command");
	ctx.check(SPCCommand::execute(level, player, u"/heal 3").success, "heal command");
	ctx.checkEqual(player.health, 8, "healed health");
	ctx.check(SPCCommand::execute(level, player, u"/hurt 2").success, "hurt command");
	ctx.checkEqual(player.health, 6, "hurt health");

	player.inventory.mainInventory[0] = ItemStack(Items::coal->getShiftedIndex(), 2);
	ctx.check(SPCCommand::execute(level, player, u"/clearinventory").success,
		"clearinventory command");
	ctx.check(player.inventory.mainInventory[0].isEmpty(), "inventory cleared");

	player.inventory.mainInventory[0] = ItemStack(Items::coal->getShiftedIndex(), 2);
	ctx.check(SPCCommand::execute(level, player, u"/kill").success, "kill command");
	ctx.checkEqual(player.health, 0, "kill health");
	ctx.check(player.inventory.mainInventory[0].isEmpty(), "kill dispatches Player death drops");
}

HEADLESS_TEST(commands, repeat_and_error_paths_are_deterministic)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-errors", Dimension::Id_Normal, 6007LL);
	Player player(level);

	SPCCommand::Result emptyRepeat = SPCCommand::execute(level, player, u"/repeat");
	ctx.check(!emptyRepeat.success, "repeat without history fails");
	ctx.check(SPCCommand::execute(level, player, u"/time set 77").success, "seed repeat history");
	level.time = 0;
	ctx.check(SPCCommand::execute(level, player, u"/repeat").success, "repeat succeeds");
	ctx.checkEqual(level.time, 77LL, "repeated command effect");
	ctx.check(!SPCCommand::execute(level, player, u"/give unknown_item").success,
		"unknown item rejected");
	ctx.check(!SPCCommand::execute(level, player, u"/doesnotexist").success,
		"unknown command rejected");
}

HEADLESS_TEST(commands, item_names_match_beta_vocabulary_within_alpha_content)
{
	headless::initGameRegistries();
	resetCommands();
	Level level(u"commands-item-names", Dimension::Id_Normal, 6008LL);
	Player player(level);

	auto giveResolves = [&](const jstring &name, int_t expectedId, const char *label)
	{
		SPCCommand::execute(level, player, u"/clearinventory");
		if (!ctx.check(SPCCommand::execute(level, player, u"/give " + name).success, label))
			return;
		ItemStack *stack = player.inventory.getCurrentItem();
		if (!ctx.check(stack != nullptr, label))
			return;
		ctx.checkEqual(stack->itemID, expectedId, label);
	};

	// Spellings the beta port accepts: descriptionId form, its aliases, and
	// the "item."/"tile." prefixed and mixed-case forms normalizeName strips.
	giveResolves(u"hatchetIron", Items::axeSteel->getShiftedIndex(), "beta name id for the iron axe");
	giveResolves(u"item.mushroomStew", Items::bowlSoup->getShiftedIndex(), "prefixed name id");
	giveResolves(u"GoldenApple", Items::appleGold->getShiftedIndex(), "mixed-case alias");
	giveResolves(u"emerald", Items::diamond->getShiftedIndex(), "beta emerald name id maps to Alpha diamond");
	giveResolves(u"lightstonedust", Items::lightStoneDust->getShiftedIndex(), "glowstone dust alias");
	giveResolves(u"chainmailboots", Items::bootsChain->getShiftedIndex(), "armor alias");
	giveResolves(u"mossstone", 48, "mossy cobblestone alias");
	giveResolves(u"jackolantern", 91, "lit pumpkin alias");

	// A shared name resolves to the block, matching the beta port's ordering.
	giveResolves(u"brick", 45, "brick prefers the block");
	giveResolves(u"brickitem", Items::brick->getShiftedIndex(), "brick item keeps its own alias");

	// Content that does not exist in Alpha 1.2.6 stays unknown.
	for (const jstring &absent : { jstring(u"cake"), jstring(u"shears"), jstring(u"bed"),
		jstring(u"sugar"), jstring(u"cookie"), jstring(u"map"), jstring(u"repeater"),
		jstring(u"lapislazuli"), jstring(u"sandstone"), jstring(u"piston"),
		jstring(u"noteblock"), jstring(u"dispenser"), jstring(u"cobweb"),
		jstring(u"trapdoor"), jstring(u"bone") })
	{
		ctx.check(!SPCCommand::execute(level, player, u"/give " + absent).success,
			"non-Alpha item name is rejected");
	}
}

HEADLESS_TEST(commands, chat_key_opens_with_any_loaded_player)
{
	ctx.check(SPCCommand::shouldOpenChat(true, 20, 20),
		"singleplayer with a loaded player opens chat");
	ctx.check(!SPCCommand::shouldOpenChat(false, 20, 20),
		"chat stays closed without a player");
	ctx.check(!SPCCommand::shouldOpenChat(true, 23, 20),
		"unrelated key does not open chat");
}
