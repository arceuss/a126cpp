// Ordinary Alpha gameplay behavior that can be exercised without rendering.

#include <memory>
#include <vector>
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/Entity.h"
#include "world/entity/Painting.h"
#include "world/entity/PrimedTnt.h"
#include "world/entity/MobCategory.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/player/Player.h"
#include "world/entity/monster/Ghast.h"
#include "world/entity/monster/PigZombie.h"
#include "world/entity/projectile/Fireball.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"
#include "world/level/Level.h"
#include "world/level/Explosion.h"
#include "world/level/levelgen/feature/TreeFeature.h"
#include "world/level/LevelListener.h"
#include "world/level/biome/BiomeMobs.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "client/renderer/entity/FireballRenderer.h"
#include "world/level/material/Material.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/FarmTile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/RailTile.h"
#include "world/level/tile/FireTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/tile/HellStoneTile.h"
#include "world/level/tile/ObsidianTile.h"
#include "world/level/tile/MobSpawnerTile.h"
#include "world/level/tile/SandTile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/Tile.h"

#include "world/level/tile/SaplingTile.h"
#include "world/level/tile/TreeTile.h"
#include "world/level/tile/entity/MobSpawnerTileEntity.h"
#include "world/level/tile/TntTile.h"
static void loadSandTestChunks(Level &level)
{
	// Sand at block (8,8) schedules with Alpha's eight-block existence radius,
	// which intersects chunk coordinates 0 and 1 on each axis.
	for (int_t x = 0; x <= 1; ++x)
	{
		for (int_t z = 0; z <= 1; ++z)
			level.getChunk(x, z);
	}
}

class GameplayCaptureListener : public LevelListener
{
public:
	std::vector<jstring> sounds;
	std::vector<jstring> particles;

	void tileChanged(int_t, int_t, int_t) override {}
	void setTilesDirty(int_t, int_t, int_t, int_t, int_t, int_t) override {}
	void allChanged() override {}
	void playSound(const jstring &name, double, double, double, float, float) override
	{
		sounds.push_back(name);
	}
	void addParticle(const jstring &name, double, double, double, double, double, double) override
	{
		particles.push_back(name);
	}
	void playMusic(const jstring &, double, double, double, float) override {}
	void entityAdded(std::shared_ptr<Entity>) override {}
	void entityRemoved(std::shared_ptr<Entity>) override {}
	void skyColorChanged() override {}
	void playStreamingMusic(const jstring &, int_t, int_t, int_t) override {}
	void tileEntityChanged(int_t, int_t, int_t, std::shared_ptr<TileEntity>) override {}
};

class GameplaySignalTile : public Tile
{
public:
	GameplaySignalTile() : Tile(200, Material::stone)
	{
	}

	bool isSignalSource() override { return true; }
	bool isSolidRender() override { return false; }
	bool getSignal(LevelSource &, int_t, int_t, int_t, int_t) override { return true; }
};

static GameplaySignalTile &gameplaySignalTile()
{
	static GameplaySignalTile tile;
	return tile;
}

HEADLESS_TEST(gameplay, unsupported_sand_spawns_falling_entity)
{
	headless::initGameRegistries();
	Level level(u"gameplay-falling-sand", Dimension::Id_Normal, 2026LL);
	loadSandTestChunks(level);
	level.instaTick = true;

	ctx.check(level.setTile(8, 100, 8, Tile::sand.id), "placing sand in air changes the block");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1,
		"unsupported sand immediately joins the world as one entity");
	if (level.entities.empty())
		return;

	std::shared_ptr<Entity> falling = *level.entities.begin();
	ctx.checkEqual(falling->getEncodeId(), jstring(u"FallingSand"), "falling entity identity");
	ctx.checkEqual(level.getTile(8, 100, 8), Tile::sand.id,
		"Alpha keeps the source block until the entity's first tick");

	const double initialY = falling->y;
	level.tickEntities();
	ctx.checkEqual(level.getTile(8, 100, 8), 0,
		"falling entity removes the source block on its first tick");
	ctx.check(falling->y < initialY, "falling entity moves down");
}

HEADLESS_TEST(gameplay, removing_support_makes_sand_fall)
{
	headless::initGameRegistries();
	Level level(u"gameplay-sand-neighbor", Dimension::Id_Normal, 2027LL);
	loadSandTestChunks(level);
	level.instaTick = true;

	ctx.check(level.setTile(8, 90, 8, Tile::rock.id), "place temporary support");
	ctx.check(level.setTile(8, 91, 8, Tile::sand.id), "place supported sand");
	ctx.check(level.entities.empty(), "supported sand must stay a block");

	ctx.check(level.setTile(8, 90, 8, 0), "remove temporary support");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1,
		"neighbor update must create one FallingSand entity");
	if (!level.entities.empty())
		ctx.checkEqual((*level.entities.begin())->getEncodeId(), jstring(u"FallingSand"),
			"neighbor-triggered falling entity identity");
}

HEADLESS_TEST(gameplay, bow_consumes_arrow_and_spawns_projectile)
{
	headless::initGameRegistries();
	Level level(u"gameplay-bow", Dimension::Id_Normal, 3001LL);
	Player player(level);
	player.setPos(8.5, 100.0, 8.5);
	player.inventory.mainInventory[0] = ItemStack(Items::arrow->getShiftedIndex(), 2);

	ItemStack bow(Items::bow->getShiftedIndex(), 1);
	bow.use(level, player);
	ctx.checkEqual(player.inventory.mainInventory[0].stackSize, 1, "one arrow consumed");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "one arrow entity spawned");
	if (!level.entities.empty())
		ctx.checkEqual((*level.entities.begin())->getEncodeId(), jstring(u"Arrow"), "spawned arrow identity");
}

HEADLESS_TEST(gameplay, snowball_spawns_projectile_only_locally)
{
	headless::initGameRegistries();
	Level level(u"gameplay-snowball", Dimension::Id_Normal, 3002LL);
	Player player(level);
	ItemStack snowballs(Items::snowball->getShiftedIndex(), 2);
	ItemStack result = snowballs.use(level, player);

	ctx.checkEqual(result.stackSize, 1, "one snowball consumed");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "one snowball entity spawned");
	if (!level.entities.empty())
		ctx.checkEqual((*level.entities.begin())->getEncodeId(), jstring(u"Snowball"),
			"spawned snowball identity");
}

HEADLESS_TEST(gameplay, saddle_then_mount_pig)
{
	headless::initGameRegistries();
	Level level(u"gameplay-saddle-pig", Dimension::Id_Normal, 3003LL);
	level.getChunk(0, 0);
	std::shared_ptr<Player> player = std::make_shared<Player>(level);
	player->setPos(0.5, 65.0, 0.5);
	std::shared_ptr<Pig> pig = std::make_shared<Pig>(level);
	pig->setPos(1.5, 65.0, 0.5);
	level.addEntity(pig);

	player->inventory.mainInventory[0] = ItemStack(Items::saddle->getShiftedIndex(), 1);
	player->interact(pig);
	ctx.check(pig->hasSaddle(), "item-on-mob interaction saddles the pig");
	ctx.check(player->inventory.getCurrentItem() == nullptr, "saddle stack consumed");

	player->interact(pig);
	ctx.check(player->riding != nullptr && player->riding.get() == pig.get(),
		"second interaction mounts the saddled pig");
	ctx.check(pig->rider != nullptr && pig->rider.get() == player.get(),
		"pig records the mounted player");
}

HEADLESS_TEST(gameplay, hoe_tills_grass_and_drops_seeds)
{
	headless::initGameRegistries();
	Level level(u"gameplay-hoe", Dimension::Id_Normal, 3004LL);
	loadSandTestChunks(level);
	ctx.check(level.setTile(8, 100, 8, Tile::grass.id), "place grass test block");

	long_t seed = 0;
	for (;; ++seed)
	{
		Random probe(seed);
		if (probe.nextInt(8) == 0)
			break;
	}
	level.random.setSeed(seed);

	Player player(level);
	ItemStack hoe(Items::hoeSteel->getShiftedIndex(), 1);
	ctx.check(Items::hoeSteel->useOn(hoe, player, level, 8, 100, 8, Facing::UP),
		"hoe accepts grass");
	ctx.checkEqual(level.getTile(8, 100, 8), Tile::farmland.id, "grass becomes farmland");
	ctx.checkEqual(hoe.itemDamage, 1, "hoe loses one durability");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "one seed drop spawned");
	if (!level.entities.empty())
	{
		std::shared_ptr<EntityItem> seeds = std::dynamic_pointer_cast<EntityItem>(*level.entities.begin());
		if (ctx.check(seeds != nullptr, "seed drop is an EntityItem"))
		{
			ctx.checkEqual(seeds->item.itemID, Items::seeds->getShiftedIndex(), "seed item id");
			ctx.checkEqual(seeds->throwTime, 10, "seed pickup delay");
		}
	}
}

HEADLESS_TEST(gameplay, minecart_item_spawns_on_rail)
{
	headless::initGameRegistries();
	Level level(u"gameplay-minecart", Dimension::Id_Normal, 3005LL);
	loadSandTestChunks(level);
	ctx.check(level.setTile(8, 100, 8, Tile::rail.id), "place rail test block");
	Player player(level);
	ItemStack minecart(Items::minecartEmpty->getShiftedIndex(), 1);

	ctx.check(Items::minecartEmpty->useOn(minecart, player, level, 8, 100, 8, Facing::UP),
		"minecart item accepts rail");
	ctx.checkEqual(minecart.stackSize, 0, "minecart item consumed");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "minecart entity spawned");
	if (!level.entities.empty())
		ctx.checkEqual((*level.entities.begin())->getEncodeId(), jstring(u"Minecart"),
			"spawned minecart identity");
}

HEADLESS_TEST(gameplay, painting_only_consumes_when_valid)
{
	headless::initGameRegistries();
	Level level(u"gameplay-painting", Dimension::Id_Normal, 3006LL);
	loadSandTestChunks(level);
	for (int_t y = 88; y <= 96; ++y)
	{
		for (int_t z = 6; z <= 14; ++z)
			level.setTile(8, y, z, Tile::rock.id);
	}
	Player player(level);

	ItemStack valid(Items::painting->getShiftedIndex(), 1);
	ctx.check(Items::painting->useOn(valid, player, level, 8, 92, 10, Facing::WEST),
		"valid wall face handled");
	ctx.checkEqual(valid.stackSize, 0, "valid painting consumed");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "painting entity spawned");
	if (!level.entities.empty())
		ctx.checkEqual((*level.entities.begin())->getEncodeId(), jstring(u"Painting"),
			"spawned painting identity");

	ItemStack invalid(Items::painting->getShiftedIndex(), 1);
	ctx.check(!Items::painting->useOn(invalid, player, level, 8, 92, 10, Facing::UP),
		"ceiling face rejected");
	ctx.checkEqual(invalid.stackSize, 1, "invalid painting retained");
}

HEADLESS_TEST(gameplay, mushroom_stew_returns_empty_bowl)
{
	headless::initGameRegistries();
	Level level(u"gameplay-soup", Dimension::Id_Normal, 3007LL);
	Player player(level);
	player.health = 10;
	ItemStack soup(Items::bowlSoup->getShiftedIndex(), 1);
	ItemStack result = soup.use(level, player);

	ctx.checkEqual(result.itemID, Items::bowlEmpty->getShiftedIndex(), "soup returns bowl item");
	ctx.checkEqual(result.stackSize, 1, "returned bowl count");
	ctx.checkEqual(player.health, 20, "soup healing");
}

HEADLESS_TEST(gameplay, lethal_player_damage_dispatches_player_death)
{
	headless::initGameRegistries();
	Level level(u"gameplay-player-death", Dimension::Id_Normal, 3008LL);
	Player player(level);
	player.setPos(0.5, 65.0, 0.5);
	player.inventory.mainInventory[0] = ItemStack(Items::coal->getShiftedIndex(), 3);

	Mob *asMob = &player;
	ctx.check(asMob->hurt(nullptr, 100), "lethal damage accepted");
	ctx.check(player.inventory.mainInventory[0].isEmpty(), "Player::die drops inventory");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1,
		"player death creates the inventory drop entity");
	ctx.checkEqualBits(player.heightOffset, 0.1f, "Player::die posture applied");
}

HEADLESS_TEST(gameplay, destroyed_tnt_primes_locally)
{
	headless::initGameRegistries();
	Level level(u"gameplay-tnt-destroy", Dimension::Id_Normal, 4001LL);
	loadSandTestChunks(level);
	ctx.check(level.setTile(8, 100, 8, Tile::tnt.id), "place TNT");
	level.setTile(8, 100, 8, 0);
	Tile::tnt.destroy(level, 8, 100, 8, 0);

	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "one primed TNT entity");
	if (!level.entities.empty())
	{
		std::shared_ptr<PrimedTnt> primed = std::dynamic_pointer_cast<PrimedTnt>(*level.entities.begin());
		if (ctx.check(primed != nullptr, "destroyed TNT creates PrimedTnt"))
			ctx.checkEqual(primed->life, 80, "direct-prime fuse");
	}
}

HEADLESS_TEST(gameplay, explosion_invokes_tnt_chain_callback)
{
	headless::initGameRegistries();
	Level level(u"gameplay-tnt-chain", Dimension::Id_Normal, 4002LL);
	loadSandTestChunks(level);
	ctx.check(level.setTile(8, 100, 8, Tile::tnt.id), "place chained TNT");

	Explosion explosion(level, nullptr, 8.5, 100.5, 8.5, 4.0f);
	explosion.toBlow.insert(TilePos(8, 100, 8));
	explosion.addParticles();

	ctx.checkEqual(level.getTile(8, 100, 8), 0, "explosion clears TNT block");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1,
		"explosion callback creates one primed TNT");
	if (!level.entities.empty())
	{
		std::shared_ptr<PrimedTnt> primed = std::dynamic_pointer_cast<PrimedTnt>(*level.entities.begin());
		if (ctx.check(primed != nullptr, "chain reaction entity type"))
			ctx.check(primed->life >= 10 && primed->life <= 29,
				"Alpha shortened chain-reaction fuse is in [10,29]");
	}
}

HEADLESS_TEST(gameplay, block_hit_extinguishes_adjacent_fire)
{
	headless::initGameRegistries();
	Level level(u"gameplay-extinguish", Dimension::Id_Normal, 4003LL);
	loadSandTestChunks(level);
	GameplayCaptureListener listener;
	level.addListener(listener);
	ctx.check(level.setTile(8, 100, 8, Tile::rock.id), "place clicked block");
	ctx.check(level.setTile(9, 99, 8, Tile::rock.id), "place fire support");
	ctx.check(level.setTile(9, 100, 8, Tile::fire.id), "place adjacent fire");

	level.extinguishFire(8, 100, 8, Facing::EAST);
	ctx.checkEqual(level.getTile(9, 100, 8), 0, "adjacent fire removed");
	ctx.check(!listener.sounds.empty() && listener.sounds.back() == u"random.fizz",
		"local extinguish emits random.fizz");
}

HEADLESS_TEST(gameplay, fire_on_hellrock_is_permanent)
{
	headless::initGameRegistries();
	Level level(u"gameplay-permanent-fire", Dimension::Id_Normal, 4004LL);
	loadSandTestChunks(level);
	ctx.check(level.setTile(8, 99, 8, Tile::hellRock.id), "place hellrock");
	ctx.check(level.setTile(8, 100, 8, Tile::fire.id), "place fire");
	level.setData(8, 100, 8, 15);

	long_t seed = 0;
	for (;; ++seed)
	{
		Random probe(seed);
		if (probe.nextInt(4) == 0)
			break;
	}
	Random random(seed);
	Tile::fire.tick(level, 8, 100, 8, random);
	ctx.checkEqual(level.getTile(8, 100, 8), Tile::fire.id,
		"age-15 fire survives the extinction draw on hellrock");
}

HEADLESS_TEST(gameplay, lava_hardening_emits_fizz)
{
	headless::initGameRegistries();
	Level level(u"gameplay-fluid-fizz", Dimension::Id_Normal, 4005LL);
	loadSandTestChunks(level);
	GameplayCaptureListener listener;
	level.addListener(listener);
	level.setTileAndData(9, 100, 8, Tile::water.id, 0);
	level.setTileAndData(8, 100, 8, Tile::calmLava.id, 0);

	ctx.checkEqual(level.getTile(8, 100, 8), Tile::obsidian.id,
		"lava source hardens to obsidian");
	ctx.check(!listener.sounds.empty() && listener.sounds.back() == u"random.fizz",
		"hardening emits random.fizz");
	int_t smoke = 0;
	for (const jstring &particle : listener.particles)
	{
		if (particle == u"largesmoke")
			++smoke;
	}
	ctx.checkEqual(smoke, 8, "hardening emits eight largesmoke particles");
}

static long_t saplingGrowthSeed()
{
	for (long_t seed = 0;; ++seed)
	{
		Random probe(seed);
		if (probe.nextInt(5) == 0 && probe.nextInt(10) != 0)
			return seed;
	}
}

HEADLESS_TEST(gameplay, mature_sapling_grows_tree)
{
	headless::initGameRegistries();
	Level level(u"gameplay-sapling-grow", Dimension::Id_Normal, 5001LL);
	loadSandTestChunks(level);
	level.setTile(8, 99, 8, Tile::dirt.id);
	level.setTile(8, 100, 8, Tile::sapling.id);
	level.setData(8, 100, 8, 15);
	Random random(saplingGrowthSeed());

	Tile::sapling.tick(level, 8, 100, 8, random);
	ctx.checkEqual(level.getTile(8, 100, 8), Tile::treeTrunk.id,
		"successful sapling growth places trunk at base");
}

HEADLESS_TEST(gameplay, obstructed_sapling_is_restored)
{
	headless::initGameRegistries();
	Level level(u"gameplay-sapling-rollback", Dimension::Id_Normal, 5002LL);
	loadSandTestChunks(level);
	level.setTile(8, 99, 8, Tile::dirt.id);
	level.setTile(8, 100, 8, Tile::sapling.id);
	level.setData(8, 100, 8, 15);
	ctx.check(level.setTile(9, 101, 8, Tile::rock.id), "place tree obstruction");
	Random random(saplingGrowthSeed());
	Random sequenceCheck(saplingGrowthSeed());
	ctx.checkEqual(sequenceCheck.nextInt(5), 0, "sapling growth gate seed");
	ctx.check(sequenceCheck.nextInt(10) != 0, "sapling chooses WorldGenTrees");

	Tile::sapling.tick(level, 8, 100, 8, random);
	ctx.checkEqual(level.getTile(8, 100, 8), Tile::sapling.id,
		"failed generator restores the sapling");
}

HEADLESS_TEST(gameplay, hostile_natural_spawn_factories_exist)
{
	headless::initGameRegistries();
	const std::vector<BiomeMobs::EntityFactory> &factories =
		BiomeMobs::getMobs(BiomeType::FOREST, MobCategory::monster);
	ctx.checkEqual(static_cast<long long>(factories.size()), 4,
		"Alpha hostile factory count");

	Level level(u"gameplay-hostile-factories", Dimension::Id_Normal, 5003LL);
	const jstring expected[] = { u"Spider", u"Zombie", u"Skeleton", u"Creeper" };
	for (int_t i = 0; i < 4; ++i)
		ctx.checkEqual(factories[static_cast<size_t>(i)](level)->getEncodeId(),
			expected[i], "hostile factory order");
}

// Alpha: MobSpawnerHell replaces both spawn lists, so the nether gets ghasts and
// pig zombies and no animals (MobSpawnerHell.java:12-15).
HEADLESS_TEST(gameplay, hell_spawns_only_nether_mobs)
{
	headless::initGameRegistries();
	Level level(u"gameplay-hell-factories", Dimension::Id_Hell, 5013LL);

	ctx.checkEqual(static_cast<long long>(static_cast<int_t>(level.getBiomeSource().getBiomeAt(1234, -4321))),
		static_cast<long long>(static_cast<int_t>(BiomeType::HELL)),
		"the nether reports the hell biome everywhere");

	const std::vector<BiomeMobs::EntityFactory> &monsters =
		BiomeMobs::getMobs(BiomeType::HELL, MobCategory::monster);
	if (!ctx.checkEqual(static_cast<long long>(monsters.size()), 2, "hell hostile factory count"))
		return;

	const jstring expected[] = { u"Ghast", u"PigZombie" };
	for (int_t i = 0; i < 2; ++i)
		ctx.checkEqual(monsters[static_cast<size_t>(i)](level)->getEncodeId(),
			expected[i], "hell factory order");

	ctx.check(BiomeMobs::getMobs(BiomeType::HELL, MobCategory::creature).empty(),
		"the nether spawns no animals");
}

HEADLESS_TEST(gameplay, nether_entities_have_their_alpha_render_data)
{
	headless::initGameRegistries();
	Level level(u"gameplay-nether-render-data", Dimension::Id_Hell, 5014LL);

	PigZombie pigZombie(level);
	ItemStack *heldItem = pigZombie.getCarriedItem();
	if (ctx.check(heldItem != nullptr, "pig zombies carry a golden sword"))
		ctx.checkEqual(heldItem->itemID, Items::swordGold->getShiftedIndex(),
			"pig zombie held-item id");

	Fireball fireball(level);
	EntityRenderer *renderer = EntityRenderDispatcher::instance.getRenderer(fireball);
	ctx.check(renderer != nullptr, "ghast fireballs have a registered renderer");
	ctx.check(dynamic_cast<FireballRenderer *>(renderer) != nullptr,
		"ghast fireballs use RenderFireball rather than the player fallback");
}

HEADLESS_TEST(gameplay, reflected_fireball_can_hit_its_ghast)
{
	headless::initGameRegistries();
	Level level(u"gameplay-returned-fireball", Dimension::Id_Normal, 5015LL);
	level.getChunk(0, 0);
	for (int_t z = 4; z <= 8; ++z)
		ctx.checkEqual(level.getTile(8, 100, z), 0,
			"the reflection test path is clear");

	std::shared_ptr<Ghast> ghast = std::make_shared<Ghast>(level);
	ghast->moveTo(8.0, 100.0, 8.0, 0.0f, 0.0f);
	level.addEntity(ghast);

	Player deflector(level);
	deflector.moveTo(8.0, 100.0, 1.0, 0.0f, 0.0f);

	Fireball fireball(level, *ghast, 0.0, 0.0, 1.0);
	fireball.moveTo(8.0, 100.0, 4.0, 0.0f, 0.0f);
	ctx.check(fireball.hurt(&deflector, 1), "the player reflects the fireball");
	ctx.check(fireball.zd > 0.99, "reflection points back toward the Ghast");
	AABB *searchBox = fireball.bb.expand(0.0, 0.0, 4.0)->grow(1.0, 1.0, 1.0);
	const std::vector<std::shared_ptr<Entity>> &nearby =
		level.getEntities(&fireball, *searchBox);
	ctx.check(!nearby.empty(), "the Ghast is present in the projectile query");
	Vec3 *from = Vec3::newTemp(fireball.x, fireball.y, fireball.z);
	Vec3 *to = Vec3::newTemp(fireball.x, fireball.y, fireball.z + 4.0);
	HitResult directHit = ghast->bb.grow(0.3, 0.3, 0.3)->clip(*from, *to);
	ctx.check(directHit.type != HitResult::Type::NONE,
		"the reflected segment intersects the Ghast");
	// Cross the Ghast's whole collision box in one tick. With the stale Ghast
	// owner, the 25-tick immunity still skips this otherwise-certain hit.
	fireball.zd = 4.0;
	fireball.tick();

	ctx.check(fireball.removed,
		"the reflected fireball collides with its former Ghast owner immediately");
}

HEADLESS_TEST(gameplay, dungeon_spawner_activates_near_player)
{
	headless::initGameRegistries();
	Level level(u"gameplay-dungeon-spawner", Dimension::Id_Normal, 5004LL);
	loadSandTestChunks(level);
	for (int_t x = 4; x <= 12; ++x)
	{
		for (int_t z = 4; z <= 12; ++z)
			level.setTile(x, 63, z, Tile::grass.id);
	}

	std::shared_ptr<Player> player = std::make_shared<Player>(level);
	player->setPos(8.5, 64.0, 0.5);
	level.addEntity(player);

	level.setTile(8, 64, 8, Tile::mobSpawner.id);
	std::shared_ptr<MobSpawnerTileEntity> spawner =
		std::make_shared<MobSpawnerTileEntity>();
	spawner->x = 8;
	spawner->y = 64;
	spawner->z = 8;
	spawner->setEntityId(u"Mob");
	spawner->spawnDelay = 0;
	level.setTileEntity(8, 64, 8, spawner);
	ctx.check(spawner->isNearPlayer(), "spawner detects nearby player");

	long_t seed = 0;
	for (;; ++seed)
	{
		Random probe(seed);
		probe.nextFloat();
		probe.nextFloat();
		probe.nextFloat();
		probe.nextDouble();
		probe.nextDouble();
		if (probe.nextInt(3) == 1)
			break;
	}
	level.random.setSeed(seed);
	spawner->tick();

	int_t spawned = 0;
	for (const std::shared_ptr<Entity> &entity : level.entities)
	{
		if (entity->getEncodeId() == u"Mob")
			++spawned;
	}
	ctx.check(spawned > 0, "active dungeon spawner creates at least one Mob");
	ctx.check(spawner->spawnDelay >= 200 && spawner->spawnDelay <= 799,
		"spawner resets delay to Alpha range");
}

HEADLESS_TEST(gameplay, tree_feature_rejects_obstruction)
{
	headless::initGameRegistries();
	Level level(u"gameplay-tree-feature-obstruction", Dimension::Id_Normal, 5005LL);
	loadSandTestChunks(level);
	level.setTile(8, 99, 8, Tile::dirt.id);
	level.setTile(8, 105, 8, Tile::rock.id);
	Random random(saplingGrowthSeed());
	random.nextInt(5);
	random.nextInt(10);
	TreeFeature tree;
	ctx.check(!tree.place(level, random, 8, 100, 8),
		"WorldGenTrees rejects the obstruction");
}

HEADLESS_TEST(gameplay, redstone_signal_primes_tnt)
{
	headless::initGameRegistries();
	Level level(u"gameplay-tnt-redstone", Dimension::Id_Normal, 5006LL);
	loadSandTestChunks(level);
	GameplaySignalTile &signal = gameplaySignalTile();
	ctx.check(level.setTile(8, 100, 8, Tile::tnt.id), "place redstone-triggered TNT");
	ctx.check(level.setTile(9, 100, 8, signal.id), "place powered signal source");

	ctx.checkEqual(level.getTile(8, 100, 8), 0, "powered TNT block removed");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1,
		"powered TNT creates one PrimedTnt");
	if (!level.entities.empty())
		ctx.check(std::dynamic_pointer_cast<PrimedTnt>(*level.entities.begin()) != nullptr,
			"redstone-prime entity type");
}

HEADLESS_TEST(gameplay, boat_item_spawns_locally)
{
	headless::initGameRegistries();
	Level level(u"gameplay-boat", Dimension::Id_Normal, 5007LL);
	loadSandTestChunks(level);
	level.setTile(8, 99, 11, Tile::rock.id);

	Player player(level);
	player.setPos(8.5, 100.0, 8.5);
	player.xo = player.x;
	player.yo = player.y;
	player.zo = player.z;
	player.yRotO = player.yRot = 0.0f;
	player.xRotO = player.xRot = 20.0f;
	ItemStack boat(Items::boat->getShiftedIndex(), 1);
	ItemStack result = boat.use(level, player);

	ctx.checkEqual(result.stackSize, 0, "boat item consumed after block hit");
	ctx.checkEqual(static_cast<long long>(level.entities.size()), 1, "boat entity spawned");
	if (!level.entities.empty())
		ctx.checkEqual((*level.entities.begin())->getEncodeId(), jstring(u"Boat"),
			"spawned boat identity");
}
