// Focused Alpha dimension-transfer and portal regressions.
//
// Alpha reference:
//   Minecraft.java:998-1026  keeps the same player object, rebinds worldObj,
//                            and runs Teleporter after the destination loads it
//   Teleporter.java:23-79    selects the nearest portal and resets velocity
//   Teleporter.java:81-229   performs the two-pass site search and four-pass
//                            4x5 frame construction

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/Teleporter.h"
#include "world/level/chunk/ChunkSource.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/ObsidianTile.h"
#include "world/level/tile/PortalTile.h"
#include "world/level/tile/Tile.h"

class PortalTestChunk : public LevelChunk
{
public:
	PortalTestChunk(Level &level, int_t x, int_t z) : LevelChunk(level, x, z)
	{
	}

	bool setTileAndData(int_t x, int_t y, int_t z, int_t tile, int_t dataValue) override
	{
		int_t index = (x * (16 * 128)) | (z * 128) | y;
		bool changed = blocks[static_cast<size_t>(index)] != tile || data.get(x, y, z) != dataValue;
		blocks[static_cast<size_t>(index)] = static_cast<ubyte_t>(tile);
		data.set(x, y, z, static_cast<ubyte_t>(dataValue));
		return changed;
	}

	bool setTile(int_t x, int_t y, int_t z, int_t tile) override
	{
		return setTileAndData(x, y, z, tile, 0);
	}
};

class PortalTestChunkSource : public ChunkSource
{
public:
	explicit PortalTestChunkSource(Level &level) : level(level)
	{
	}

	bool hasChunk(int_t x, int_t z) override
	{
		return true;
	}

	std::shared_ptr<LevelChunk> getChunk(int_t x, int_t z) override
	{
		long_t key = chunkKey(x, z);
		auto found = chunks.find(key);
		if (found != chunks.end())
			return found->second;

		std::shared_ptr<LevelChunk> chunk = std::make_shared<PortalTestChunk>(level, x, z);
		chunks.emplace(key, chunk);
		return chunk;
	}

	void postProcess(ChunkSource &, int_t, int_t) override
	{
	}

	bool save(bool, std::shared_ptr<ProgressListener>) override
	{
		return true;
	}

	bool tick() override
	{
		return false;
	}

	bool shouldSave() override
	{
		return false;
	}

	jstring gatherStats() override
	{
		return u"PortalTestChunkSource";
	}

private:
	static long_t chunkKey(int_t x, int_t z)
	{
		uint64_t ux = static_cast<uint32_t>(x);
		uint64_t uz = static_cast<uint32_t>(z);
		return static_cast<long_t>((ux << 32) | uz);
	}

	Level &level;
	std::unordered_map<long_t, std::shared_ptr<LevelChunk>> chunks;
};

class PortalTestLevel : public Level
{
public:
	explicit PortalTestLevel(const jstring &name, int_t dimension = Dimension::Id_Normal)
		: Level(name, dimension, 0x126LL)
	{
		chunkSource = std::make_shared<PortalTestChunkSource>(*this);
	}
};

static void placePortalRectangle(Level &level, int_t interiorX, int_t interiorY, int_t interiorZ, bool spansX)
{
	int_t xStep = spansX ? 1 : 0;
	int_t zStep = spansX ? 0 : 1;
	for (int_t across = 0; across < 4; across++)
	{
		for (int_t up = -1; up < 4; up++)
		{
			int_t x = interiorX + (across - 1) * xStep;
			int_t z = interiorZ + (across - 1) * zStep;
			bool frame = across == 0 || across == 3 || up == -1 || up == 3;
			level.setTileNoUpdate(x, interiorY + up, z,
				frame ? Tile::obsidian.id : Tile::portalTile.id);
		}
	}
}

static bool verifyPortalRectangle(headless::TestContext &ctx, Level &level,
	int_t interiorX, int_t interiorY, int_t interiorZ, bool spansX)
{
	int_t xStep = spansX ? 1 : 0;
	int_t zStep = spansX ? 0 : 1;
	for (int_t across = 0; across < 4; across++)
	{
		for (int_t up = -1; up < 4; up++)
		{
			int_t x = interiorX + (across - 1) * xStep;
			int_t z = interiorZ + (across - 1) * zStep;
			bool frame = across == 0 || across == 3 || up == -1 || up == 3;
			int_t expected = frame ? Tile::obsidian.id : Tile::portalTile.id;
			if (!ctx.checkEqual(level.getTile(x, interiorY + up, z), expected,
				"portal rectangle block across=" + std::to_string(across) +
				" up=" + std::to_string(up)))
				return false;
		}
	}
	return true;
}

// This state deliberately includes fields omitted by Entity/Player NBT. A
// recreation-based dimension transfer therefore cannot satisfy this test.
HEADLESS_TEST(nether, player_reparent_preserves_identity_and_live_state)
{
	headless::initGameRegistries();
	PortalTestLevel overworld(u"reparent-overworld");
	PortalTestLevel nether(u"reparent-nether", Dimension::Id_Hell);
	std::shared_ptr<Player> player = std::make_shared<Player>(overworld);
	Player *identity = player.get();
	int_t entityId = player->entityId;

	player->health = 7;
	player->hurtTime = 6;
	player->attackTime = 5;
	player->tickCount = 404;
	player->score = 1234;
	player->swinging = true;
	player->swingTime = 3;
	player->horizontalCollision = true;
	player->hurtMarked = true;
	player->walkAnimPos = 9.25f;
	player->attackAnim = 0.75f;
	player->customTextureUrl = u"identity-state";
	player->moveTo(10.25, 72.5, -3.75, 91.0f, -14.0f);

	overworld.addEntity(player);
	overworld.removeEntity(player);
	player->removed = false;
	player->setLevel(nether);
	nether.addEntity(player);

	ctx.check(player.get() == identity, "dimension transfer keeps the same player address");
	ctx.check(&player->getLevel() == &nether, "player is rebound to the destination level");
	ctx.checkEqual(player->entityId, entityId, "entity id survives reparenting");
	ctx.checkEqual(player->health, 7, "health survives reparenting");
	ctx.checkEqual(player->hurtTime, 6, "hurt timer survives reparenting");
	ctx.checkEqual(player->attackTime, 5, "attack timer survives reparenting");
	ctx.checkEqual(player->tickCount, 404, "tick counter survives reparenting");
	ctx.checkEqual(player->score, 1234, "score survives reparenting");
	ctx.check(player->swinging && player->swingTime == 3, "swing state survives reparenting");
	ctx.check(player->horizontalCollision && player->hurtMarked, "collision and hurt flags survive reparenting");
	ctx.checkEqualBits(player->walkAnimPos, 9.25f, "walk animation survives reparenting");
	ctx.checkEqualBits(player->attackAnim, 0.75f, "attack animation survives reparenting");
	ctx.checkEqual(player->customTextureUrl, jstring(u"identity-state"), "custom texture state survives reparenting");
	ctx.checkEqual(static_cast<long long>(overworld.players.size()), 0, "old level no longer owns the player as a player");
	ctx.checkEqual(static_cast<long long>(nether.players.size()), 1, "new level owns one player");
	ctx.check(nether.players[0].get() == identity, "new level owns the original player object");
}

HEADLESS_TEST(nether, nearest_existing_portal_places_entity_like_alpha)
{
	headless::initGameRegistries();
	PortalTestLevel level(u"nearest-portal");
	placePortalRectangle(level, 4, 70, 2, true);
	placePortalRectangle(level, 50, 70, 50, false);

	Entity entity(level);
	entity.moveTo(4.2, 72.0, 2.2, 137.0f, 22.0f);
	entity.setVelocity(1.5, -0.25, 0.75);

	Teleporter teleporter;
	ctx.check(teleporter.findPortal(level, entity), "an existing portal is found");
	ctx.checkEqualBits(entity.x, 5.0, "entity is centred across the two portal columns");
	ctx.checkEqualBits(entity.y, 70.5, "entity uses the bottom portal block plus one half");
	ctx.checkEqualBits(entity.z, 2.5, "entity is centred on the portal plane");
	ctx.checkEqualBits(entity.yRot, 137.0f, "portal lookup keeps yaw");
	ctx.checkEqualBits(entity.xRot, 0.0f, "portal lookup resets pitch");
	ctx.checkEqualBits(entity.xd, 0.0, "portal lookup clears x velocity");
	ctx.checkEqualBits(entity.yd, 0.0, "portal lookup clears y velocity");
	ctx.checkEqualBits(entity.zd, 0.0, "portal lookup clears z velocity");
}

HEADLESS_TEST(nether, no_site_fallback_builds_complete_portal_and_teleports)
{
	headless::initGameRegistries();
	PortalTestLevel level(u"fallback-portal");
	Entity entity(level);
	entity.moveTo(0.25, 40.0, 0.25, 33.0f, 18.0f);
	entity.setVelocity(-0.5, 1.25, 0.75);

	Teleporter teleporter;
	teleporter.teleport(level, entity);

	std::vector<std::array<int_t, 3>> portalBlocks;
	for (int_t x = -4; x <= 4; x++)
	{
		for (int_t z = -4; z <= 4; z++)
		{
			for (int_t y = 68; y <= 74; y++)
			{
				if (level.getTile(x, y, z) == Tile::portalTile.id)
					portalBlocks.push_back({ x, y, z });
			}
		}
	}

	if (!ctx.checkEqual(static_cast<long long>(portalBlocks.size()), 6,
		"fallback creates six portal interior blocks"))
		return;

	std::set<int_t> xs;
	std::set<int_t> ys;
	std::set<int_t> zs;
	for (const std::array<int_t, 3> &block : portalBlocks)
	{
		xs.insert(block[0]);
		ys.insert(block[1]);
		zs.insert(block[2]);
	}

	if (!ctx.checkEqual(static_cast<long long>(ys.size()), 3, "portal interior is three blocks high"))
		return;
	int_t interiorY = *ys.begin();
	bool spansX = xs.size() == 2 && zs.size() == 1;
	bool spansZ = xs.size() == 1 && zs.size() == 2;
	if (!ctx.check(spansX || spansZ, "portal interior is two-by-three on one horizontal axis"))
		return;

	int_t interiorX = *xs.begin();
	int_t interiorZ = *zs.begin();
	if (!verifyPortalRectangle(ctx, level, interiorX, interiorY, interiorZ, spansX))
		return;

	ctx.check(!level.noNeighborUpdate, "portal build restores neighbour updates");
	double expectedX = spansX ? static_cast<double>(interiorX) + 1.0
		: static_cast<double>(interiorX) + 0.5;
	double expectedZ = spansX ? static_cast<double>(interiorZ) + 0.5
		: static_cast<double>(interiorZ) + 1.0;
	ctx.checkEqualBits(entity.x, expectedX, "fallback centres the entity across the portal interior");
	ctx.checkEqualBits(entity.y, static_cast<double>(interiorY) + 0.5,
		"fallback teleports to the created portal floor");
	ctx.checkEqualBits(entity.z, expectedZ, "fallback centres the entity on the portal plane");
	ctx.checkEqualBits(entity.yRot, 33.0f, "fallback transfer keeps yaw");
	ctx.checkEqualBits(entity.xRot, 0.0f, "fallback transfer resets pitch");
	ctx.checkEqualBits(entity.xd, 0.0, "fallback transfer clears x velocity");
	ctx.checkEqualBits(entity.yd, 0.0, "fallback transfer clears y velocity");
	ctx.checkEqualBits(entity.zd, 0.0, "fallback transfer clears z velocity");
}

// Alpha places a portal arrival with setLocationAndAngles, so posY carries the
// player's 1.62 height offset and the bounding box bottom lands on the reported
// coordinate (Teleporter.java:72, Entity.java:538-545). The no-offset setter
// left the box a whole height offset lower, inside the obsidian floor.
HEADLESS_TEST(nether, portal_arrival_keeps_player_height_offset)
{
	headless::initGameRegistries();
	PortalTestLevel level(u"portal-arrival-offset");
	placePortalRectangle(level, 4, 70, 2, true);

	std::shared_ptr<Player> player = std::make_shared<Player>(level);
	player->absMoveTo(4.5, 76.0, 2.5, 12.0f, 5.0f);
	ctx.checkEqualBits(player->heightOffset, 1.62f, "player keeps Alpha's height offset");

	Teleporter teleporter;
	if (!ctx.check(teleporter.findPortal(level, *player), "the placed portal is found"))
		return;

	const double portalFloor = 70.5;
	ctx.checkEqualBits(player->x, 5.0, "arrival is centred across the portal columns");
	ctx.checkEqualBits(player->z, 2.5, "arrival is centred on the portal plane");
	ctx.checkEqualBits(player->y, portalFloor + static_cast<double>(player->heightOffset),
		"arrival position carries the height offset");
	ctx.checkEqualBits(player->yo, player->y, "the previous position matches the arrival");
	ctx.check(std::fabs(player->bb.y0 - portalFloor) < 1.0e-6,
		"the collision box rests at the portal floor instead of below it");
}
