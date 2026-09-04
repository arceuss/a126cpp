// Minecraft.h reaches windows.h; keep std::min/max usable below.
#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif

#include "tools/stress/StressHarness.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "client/Minecraft.h"
#include "client/player/LocalPlayer.h"
#include "client/player/Input.h"
#include "client/multiplayer/MultiPlayerLevel.h"
#include "java/String.h"
#include "network/NetClientHandler.h"
#include "network/Packet3Chat.h"
#include "java/Random.h"
#include "util/Mth.h"
#include "world/entity/Mob.h"
#include "world/entity/Painting.h"
#include "world/entity/PrimedTnt.h"
#include "world/entity/animal/Chicken.h"
#include "world/entity/animal/Cow.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/animal/Sheep.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/item/Minecart.h"
#include "world/entity/monster/Creeper.h"
#include "world/entity/monster/Skeleton.h"
#include "world/entity/monster/Spider.h"
#include "world/entity/monster/Zombie.h"
#include "world/entity/projectile/Arrow.h"
#include "world/item/ItemStack.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/CobblestoneTile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/GlassTile.h"
#include "world/level/tile/StoneTile.h"
#include "world/level/tile/TorchTile.h"
#include "world/level/tile/WoodTile.h"

// Scenarios are worst cases the profiling sessions in the audit could not
// isolate: each one stresses a single subsystem so the log attributes cost to
// it. Every block placement goes through `Level::setTile`, the same path the
// game uses, so light scheduling and dirty marking match play.

namespace stress
{

// Alpha's world edge: the terrain noise loses precision at this x or z and
// produces the Far Lands (net.minecraft.src.NoiseGeneratorOctaves overflow).
static const int_t FAR_LANDS = 12550821;

// The player floats above the terrain so no scenario depends on the surface
// shape at its position; chunks below still load, light, and render.
static double hoverY(Level &level, int_t x, int_t z)
{
	return static_cast<double>(level.getHeightmap(x, z)) + 4.0;
}

// Against a server the player must stand where the server put it: hovering
// is flight, which the server rejects, and the spawn column is authoritative.
static double restingY(World &world)
{
	if (world.online)
		return world.player.y;
	return hoverY(world.level, Mth::floor(world.player.x), Mth::floor(world.player.z));
}

// Stand at spawn and do nothing: the control every other scenario is compared
// against. Whatever churn appears here is the port's idle cost.
class IdleScenario : public Scenario
{
	double x = 0.0, y = 0.0, z = 0.0;

public:
	const char *name() const override { return "idle"; }
	int defaultFrames() const override { return 600; }
	bool supportsMultiplayer() const override { return true; }

	void setup(World &world, const Params &) override
	{
		x = world.player.x;
		z = world.player.z;
		y = restingY(world);
		pinPlayer(world.player, x, y, z, 0.0f, 20.0f);
	}

	void onTick(World &world, long_t) override
	{
		pinPlayer(world.player, x, y, z, 0.0f, 20.0f);
	}
};

// Turn in place: the view direction drives frustum culling, the distance
// resort, and the dirty re-add scan, none of which a fixed camera exercises.
// `--rate` degrees per tick.
class SpinScenario : public Scenario
{
	double x = 0.0, y = 0.0, z = 0.0;
	float rate = 3.0f;

public:
	const char *name() const override { return "spin"; }
	int defaultFrames() const override { return 900; }
	bool supportsMultiplayer() const override { return true; }

	void setup(World &world, const Params &params) override
	{
		rate = static_cast<float>(params.doubleOr("rate", 3.0));
		x = world.player.x;
		z = world.player.z;
		y = restingY(world);
		pinPlayer(world.player, x, y, z, 0.0f, 10.0f);
	}

	void onTick(World &world, long_t tick) override
	{
		const float yaw = std::fmod(rate * static_cast<float>(tick), 360.0f);
		const float pitch = 10.0f * std::sin(static_cast<float>(tick) * 0.05f);
		pinPlayer(world.player, x, y, z, yaw, pitch);
	}
};

// Run the clock: every sky-darken step dirties all sections at once and the
// sky light path recomputes through the whole loaded set. `--step` ticks of
// world time per game tick (24000 = one day).
class DayCycleScenario : public Scenario
{
	double x = 0.0, y = 0.0, z = 0.0;
	long_t step = 50;

public:
	const char *name() const override { return "daycycle"; }
	int defaultFrames() const override { return 1500; }

	void setup(World &world, const Params &params) override
	{
		step = static_cast<long_t>(params.intOr("step", 50));
		x = world.player.x;
		z = world.player.z;
		y = restingY(world);
		pinPlayer(world.player, x, y, z, 0.0f, 20.0f);
	}

	void onTick(World &world, long_t) override
	{
		pinPlayer(world.player, x, y, z, 0.0f, 20.0f);
		world.level.setTime(world.level.time + step);
	}

	void report(World &world, std::vector<std::string> &lines) override
	{
		lines.push_back("daycycle_end_time " + std::to_string(world.level.time));
	}
};

// Straight-line travel over new terrain: chunk generation, lighting of fresh
// chunks, section rebuilds, and cache turnover at a fixed blocks-per-tick speed.
// `--distance` blocks, `--speed` blocks per tick.
class TravelScenario : public Scenario
{
	double startX = 0.0, z = 0.0;
	double speed = 8.0;
	double distance = 10000.0;
	double y = 96.0;

public:
	const char *name() const override { return "travel"; }
	// Set by setup: enough frames to cover the distance at the tick cadence.
	int defaultFrames() const override { return frames; }
	int frames = 0;

	void setup(World &world, const Params &params) override
	{
		speed = params.doubleOr("speed", 8.0);
		distance = params.doubleOr("distance", 10000.0);
		startX = world.player.x;
		z = world.player.z;
		pinPlayer(world.player, startX, y, z, 90.0f, 10.0f);
		frames = static_cast<int>(std::ceil(distance / speed)) * world.tickInterval;
	}

	void onTick(World &world, long_t tick) override
	{
		const double x = startX + std::min(distance, speed * static_cast<double>(tick));
		pinPlayer(world.player, x, y, z, 90.0f, 10.0f);
	}

	void report(World &world, std::vector<std::string> &lines) override
	{
		lines.push_back("travel_end_x " + std::to_string(world.player.x));
	}
};

// Holds the forward key and jumps when the player runs into a block: the
// same input the keyboard produces, so the client's own physics moves the
// player at Alpha's walking speed and every position it sends is one the
// server can reproduce.
class ScriptedInput final : public Input
{
public:
	void tick(Player &player) override
	{
		xa = 0.0f;
		ya = 1.0f;
		jumping = player.horizontalCollision && (player.onGround || player.isInWater());
		sneaking = false;
	}

	void releaseAllKeys() override {}
	void setKey(int_t, bool) override {}
};

// Walk a circle around spawn under real player physics, steering the view
// toward the next point on the circle. Teleports and flight get a player
// kicked, so against a server this is the movement that keeps chunks
// streaming in and out at the view edge the way play does. `--radius` blocks.
class WalkScenario : public Scenario
{
	double cx = 0.0, cz = 0.0;
	double radius = 48.0;
	double lastX = 0.0, lastZ = 0.0;
	double walked = 0.0;
	long_t jumps = 0;

public:
	const char *name() const override { return "walk"; }
	int defaultFrames() const override { return 3000; }
	bool supportsMultiplayer() const override { return true; }

	void setup(World &world, const Params &params) override
	{
		radius = params.doubleOr("radius", 48.0);
		cx = world.player.x - radius;
		cz = world.player.z;
		lastX = world.player.x;
		lastZ = world.player.z;
		world.player.input = std::make_unique<ScriptedInput>();
		steer(world);
	}

	void onTick(World &world, long_t) override
	{
		const double dx = world.player.x - lastX;
		const double dz = world.player.z - lastZ;
		walked += std::sqrt(dx * dx + dz * dz);
		lastX = world.player.x;
		lastZ = world.player.z;
		if (world.player.input->jumping)
			jumps++;
		steer(world);
	}

	void report(World &world, std::vector<std::string> &lines) override
	{
		const double dx = world.player.x - cx;
		const double dz = world.player.z - cz;
		lines.push_back("walk_distance " + std::to_string(walked));
		lines.push_back("walk_jumps " + std::to_string(jumps));
		lines.push_back("walk_end_radius " + std::to_string(std::sqrt(dx * dx + dz * dz)));
	}

private:
	// Aim at the circle point a few degrees ahead of the player's own angle,
	// so terrain detours converge back onto the ring instead of spiralling.
	void steer(World &world)
	{
		const double angle = std::atan2(world.player.z - cz, world.player.x - cx) + 8.0 * 3.141592653589793 / 180.0;
		const double tx = cx + std::cos(angle) * radius;
		const double tz = cz + std::sin(angle) * radius;
		const double dx = tx - world.player.x;
		const double dz = tz - world.player.z;
		// Game convention: yaw = atan2(dz, dx) - 90 (Mob.cpp lookAt).
		const float yaw = static_cast<float>(std::atan2(dz, dx) * 180.0 / 3.141592653589793 - 90.0);
		world.player.yRot = yaw;
		world.player.yRotO = yaw;
		world.player.xRot = 10.0f;
		world.player.xRotO = 10.0f;
	}
};

// Teleport to the Far Lands edge and walk along it. Generation there is the
// pathological noise case, and every renderer offset sits at the extreme of
// float precision. `--axis x|z`, `--sign +|-`, `--distance`, `--speed`.
class FarLandsScenario : public Scenario
{
	bool alongX = true;
	int_t sign = 1;
	double speed = 4.0;
	double distance = 512.0;
	double baseX = 0.0, baseZ = 0.0;
	double y = 96.0;

public:
	const char *name() const override { return "farlands"; }
	int defaultFrames() const override { return 900; }

	void setup(World &world, const Params &params) override
	{
		alongX = params.stringOr("axis", "x") != "z";
		sign = params.stringOr("sign", "+") == "-" ? -1 : 1;
		speed = params.doubleOr("speed", 4.0);
		distance = params.doubleOr("distance", 512.0);
		baseX = alongX ? static_cast<double>(FAR_LANDS * sign) : 0.0;
		baseZ = alongX ? 0.0 : static_cast<double>(FAR_LANDS * sign);
		pinPlayer(world.player, baseX, y, baseZ, alongX ? 0.0f : 90.0f, 10.0f);
	}

	void onTick(World &world, long_t tick) override
	{
		// Walk perpendicular to the edge axis so the edge stays in view.
		const double offset = std::min(distance, speed * static_cast<double>(tick));
		const double x = alongX ? baseX : baseX + offset;
		const double z = alongX ? baseZ + offset : baseZ;
		pinPlayer(world.player, x, y, z, alongX ? 0.0f : 90.0f, 10.0f);
	}

	void report(World &world, std::vector<std::string> &lines) override
	{
		const int_t px = Mth::floor(world.player.x);
		const int_t pz = Mth::floor(world.player.z);
		lines.push_back("farlands_x " + std::to_string(px));
		lines.push_back("farlands_z " + std::to_string(pz));
		lines.push_back("farlands_heightmap " + std::to_string(world.level.getHeightmap(px, pz)));
	}
};

// A large multi-storey building placed through setTile: the bulk-edit case
// (light scheduling per placement, then the drain, then the rebuild of every
// section it touches). `--size` footprint, `--floors`, `--torches` per floor.
class BuildingScenario : public Scenario
{
	int_t placed = 0;
	int_t size = 128;
	int_t floors = 8;
	double camX = 0.0, camY = 0.0, camZ = 0.0;

public:
	const char *name() const override { return "building"; }
	int defaultFrames() const override { return 600; }

	void setup(World &world, const Params &params) override
	{
		size = static_cast<int_t>(params.intOr("size", 128));
		floors = static_cast<int_t>(params.intOr("floors", 8));
		const int_t torchSpacing = static_cast<int_t>(params.intOr("torch_spacing", 8));
		const int_t floorHeight = 6;
		Level &level = world.level;
		const int_t x0 = Mth::floor(world.player.x) - size / 2;
		const int_t z0 = Mth::floor(world.player.z) - size / 2;
		const int_t y0 = level.getHeightmap(x0 + size / 2, z0 + size / 2);
		const int_t height = floors * floorHeight;

		auto put = [&](int_t x, int_t y, int_t z, int_t id)
		{
			if (y < 1 || y > 126)
				return;
			if (level.setTile(x, y, z, id))
				placed++;
		};

		for (int_t f = 0; f <= floors; f++)
		{
			const int_t y = y0 + f * floorHeight;
			for (int_t x = x0; x < x0 + size; x++)
				for (int_t z = z0; z < z0 + size; z++)
					put(x, y, z, f == 0 ? Tile::cobblestone.id : Tile::wood.id);
		}
		for (int_t y = y0 + 1; y < y0 + height; y++)
		{
			const bool windowRow = (y - y0) % floorHeight == 3;
			for (int_t x = x0; x < x0 + size; x++)
			{
				const int_t id = windowRow && (x % 2 == 0) ? Tile::glass.id : Tile::cobblestone.id;
				put(x, y, z0, id);
				put(x, y, z0 + size - 1, id);
			}
			for (int_t z = z0 + 1; z < z0 + size - 1; z++)
			{
				const int_t id = windowRow && (z % 2 == 0) ? Tile::glass.id : Tile::cobblestone.id;
				put(x0, y, z, id);
				put(x0 + size - 1, y, z, id);
			}
			// Interior partition walls every 16 blocks with doorways.
			for (int_t x = x0 + 16; x < x0 + size - 1; x += 16)
				for (int_t z = z0 + 1; z < z0 + size - 1; z++)
					if ((z - z0) % 8 != 4)
						put(x, y, z, Tile::cobblestone.id);
		}
		for (int_t f = 0; f < floors; f++)
		{
			const int_t y = y0 + f * floorHeight + 1;
			for (int_t x = x0 + torchSpacing / 2; x < x0 + size; x += torchSpacing)
				for (int_t z = z0 + torchSpacing / 2; z < z0 + size; z += torchSpacing)
					if (level.setTileAndData(x, y, z, Tile::torch.id, 5))
						placed++;
		}

		camX = static_cast<double>(x0) - 24.0;
		camY = static_cast<double>(y0 + height / 2);
		camZ = static_cast<double>(z0 + size / 2);
		pinPlayer(world.player, camX, camY, camZ, 270.0f, 10.0f);
	}

	void onTick(World &world, long_t) override
	{
		pinPlayer(world.player, camX, camY, camZ, 270.0f, 10.0f);
	}

	void report(World &, std::vector<std::string> &lines) override
	{
		lines.push_back("building_blocks_placed " + std::to_string(placed));
	}
};

// Continuous light churn inside a sealed dark hall: a grid of `--count`
// torches is placed and removed every `--period` ticks, and a roof section is
// opened and closed on the alternate phase so sky light recomputes too. Light
// records stay queued between frames, so the renderer sees mid-convergence
// states exactly as in play.
class LightingScenario : public Scenario
{
	int_t count = 1024;
	int_t period = 20;
	int_t x0 = 0, y0 = 0, z0 = 0;
	int_t width = 64, depth = 64, height = 12;
	std::vector<std::pair<int_t, int_t>> torches;
	double camX = 0.0, camY = 0.0, camZ = 0.0;
	long_t toggles = 0;

public:
	const char *name() const override { return "lighting"; }
	int defaultFrames() const override { return 1200; }
	bool settleBeforeMeasure() const override { return true; }

	void setup(World &world, const Params &params) override
	{
		count = static_cast<int_t>(params.intOr("count", 1024));
		period = static_cast<int_t>(params.intOr("period", 20));
		width = static_cast<int_t>(params.intOr("width", 64));
		depth = static_cast<int_t>(params.intOr("depth", 64));
		Level &level = world.level;
		x0 = Mth::floor(world.player.x) - width / 2;
		z0 = Mth::floor(world.player.z) - depth / 2;
		y0 = level.getHeightmap(x0 + width / 2, z0 + depth / 2) + 2;
		if (y0 + height + 2 > 126)
			y0 = 126 - height - 2;

		// Sealed shell: floor, roof, and four walls. Interior air.
		for (int_t x = x0 - 1; x <= x0 + width; x++)
			for (int_t z = z0 - 1; z <= z0 + depth; z++)
			{
				level.setTile(x, y0 - 1, z, Tile::rock.id);
				level.setTile(x, y0 + height, z, Tile::rock.id);
				for (int_t y = y0; y < y0 + height; y++)
				{
					const bool wall = x == x0 - 1 || x == x0 + width || z == z0 - 1 || z == z0 + depth;
					level.setTile(x, y, z, wall ? Tile::rock.id : 0);
				}
			}

		// Torch grid as square as the count allows.
		const int_t perRow = std::max<int_t>(1, static_cast<int_t>(std::sqrt(static_cast<double>(count))));
		const int_t sx = std::max<int_t>(1, width / perRow);
		const int_t sz = std::max<int_t>(1, depth / perRow);
		for (int_t x = x0 + sx / 2; x < x0 + width && static_cast<int_t>(torches.size()) < count; x += sx)
			for (int_t z = z0 + sz / 2; z < z0 + depth && static_cast<int_t>(torches.size()) < count; z += sz)
				torches.emplace_back(x, z);

		camX = static_cast<double>(x0 + width / 2);
		camY = static_cast<double>(y0 + height - 2);
		camZ = static_cast<double>(z0 + 2);
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 25.0f);
	}

	void onTick(World &world, long_t tick) override
	{
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 25.0f);
		if (tick % period != 0)
			return;
		Level &level = world.level;
		const long_t phase = (tick / period) % 4;
		switch (phase)
		{
		case 0:
			for (const auto &t : torches)
				level.setTileAndData(t.first, y0, t.second, Tile::torch.id, 5);
			break;
		case 1:
			for (int_t x = x0 + width / 4; x < x0 + 3 * width / 4; x++)
				for (int_t z = z0 + depth / 4; z < z0 + 3 * depth / 4; z++)
					level.setTile(x, y0 + height, z, 0);
			break;
		case 2:
			for (const auto &t : torches)
				level.setTile(t.first, y0, t.second, 0);
			break;
		default:
			for (int_t x = x0 + width / 4; x < x0 + 3 * width / 4; x++)
				for (int_t z = z0 + depth / 4; z < z0 + 3 * depth / 4; z++)
					level.setTile(x, y0 + height, z, Tile::rock.id);
			break;
		}
		toggles++;
	}

	void report(World &, std::vector<std::string> &lines) override
	{
		lines.push_back("lighting_torches " + std::to_string(torches.size()));
		lines.push_back("lighting_toggles " + std::to_string(toggles));
	}
};

// `--count` mobs of every Alpha kind spawned in a ring around the player at
// night, so hostiles path toward the player and animals wander: the entity
// tick, collision gather, pathfinding, and entity render paths at volume.
// Natural spawning stays on so the population keeps growing as in play.
class MobsScenario : public Scenario
{
	int_t requested = 500;
	int_t spawned = 0;
	double camX = 0.0, camY = 0.0, camZ = 0.0;

public:
	const char *name() const override { return "mobs"; }
	int defaultFrames() const override { return 1200; }

	void setup(World &world, const Params &params) override
	{
		requested = static_cast<int_t>(params.intOr("count", 500));
		Level &level = world.level;
		Random random(world.seed);
		const int_t px = Mth::floor(world.player.x);
		const int_t pz = Mth::floor(world.player.z);

		// Midnight: hostiles do not burn, and the spawner keeps adding more.
		level.setTime(18000);

		for (int_t i = 0; i < requested; i++)
		{
			const double angle = random.nextDouble() * 6.283185307179586;
			const double radius = 12.0 + random.nextDouble() * 36.0;
			const int_t x = px + Mth::floor(std::cos(angle) * radius);
			const int_t z = pz + Mth::floor(std::sin(angle) * radius);
			const double y = static_cast<double>(level.getHeightmap(x, z)) + 1.0;
			std::shared_ptr<Mob> mob;
			switch (i % 8)
			{
			case 0: mob = std::make_shared<Zombie>(level); break;
			case 1: mob = std::make_shared<Skeleton>(level); break;
			case 2: mob = std::make_shared<Spider>(level); break;
			case 3: mob = std::make_shared<Creeper>(level); break;
			case 4: mob = std::make_shared<Pig>(level); break;
			case 5: mob = std::make_shared<Sheep>(level); break;
			case 6: mob = std::make_shared<Cow>(level); break;
			default: mob = std::make_shared<Chicken>(level); break;
			}
			mob->moveTo(x + 0.5, y, z + 0.5, static_cast<float>(random.nextFloat() * 360.0f), 0.0f);
			level.addEntity(mob);
			spawned++;
		}

		camX = world.player.x;
		camZ = world.player.z;
		camY = hoverY(level, px, pz) + 6.0;
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 45.0f);
	}

	void onTick(World &world, long_t) override
	{
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 45.0f);
	}

	void report(World &world, std::vector<std::string> &lines) override
	{
		lines.push_back("mobs_spawned " + std::to_string(spawned));
		lines.push_back("mobs_alive " + std::to_string(world.level.countInstanceOf<Mob>()));
	}
};

// Water and lava sources dumped on a stone plateau: fluid spread is the main
// scheduled-tick producer, lava emits light while it flows, and every flow
// step dirties sections. `--size` plateau edge, `--spacing` between sources.
class FluidsScenario : public Scenario
{
	int_t sources = 0;
	double camX = 0.0, camY = 0.0, camZ = 0.0;

public:
	const char *name() const override { return "fluids"; }
	int defaultFrames() const override { return 1500; }
	bool settleBeforeMeasure() const override { return false; }

	void setup(World &world, const Params &params) override
	{
		const int_t size = static_cast<int_t>(params.intOr("size", 64));
		const int_t spacing = static_cast<int_t>(params.intOr("spacing", 8));
		Level &level = world.level;
		const int_t x0 = Mth::floor(world.player.x) - size / 2;
		const int_t z0 = Mth::floor(world.player.z) - size / 2;
		const int_t y0 = level.getHeightmap(x0 + size / 2, z0 + size / 2) + 3;

		// Plateau with a one-block rim so the fluids pool before spilling.
		for (int_t x = x0 - 1; x <= x0 + size; x++)
			for (int_t z = z0 - 1; z <= z0 + size; z++)
			{
				level.setTile(x, y0 - 1, z, Tile::rock.id);
				const bool rim = x == x0 - 1 || x == x0 + size || z == z0 - 1 || z == z0 + size;
				level.setTile(x, y0, z, rim ? Tile::rock.id : 0);
			}
		// Water on the west half, lava on the east half, a wall between.
		for (int_t z = z0; z < z0 + size; z++)
			level.setTile(x0 + size / 2, y0, z, Tile::rock.id);
		for (int_t x = x0 + spacing / 2; x < x0 + size; x += spacing)
			for (int_t z = z0 + spacing / 2; z < z0 + size; z += spacing)
			{
				const int_t id = x < x0 + size / 2 ? Tile::water.id : Tile::lava.id;
				if (level.setTile(x, y0 + 1, z, id))
					sources++;
			}

		camX = static_cast<double>(x0 + size / 2);
		camY = static_cast<double>(y0 + 12);
		camZ = static_cast<double>(z0) - 8.0;
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 35.0f);
	}

	void onTick(World &world, long_t) override
	{
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 35.0f);
	}

	void report(World &, std::vector<std::string> &lines) override
	{
		lines.push_back("fluids_sources " + std::to_string(sources));
	}
};

// Primed TNT dropped in batches over terrain: explosions run the block
// raycasts and entity pushes, then the craters relight and rebuild.
// `--count` per batch, `--period` ticks between batches.
class TntScenario : public Scenario
{
	int_t count = 32;
	int_t period = 100;
	int_t primed = 0;
	double camX = 0.0, camY = 0.0, camZ = 0.0;
	Random random;

public:
	TntScenario() : random(1LL) {}
	const char *name() const override { return "tnt"; }
	int defaultFrames() const override { return 1500; }

	void setup(World &world, const Params &params) override
	{
		count = static_cast<int_t>(params.intOr("count", 32));
		period = static_cast<int_t>(params.intOr("period", 100));
		camX = world.player.x;
		camZ = world.player.z;
		camY = hoverY(world.level, Mth::floor(camX), Mth::floor(camZ)) + 16.0;
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 60.0f);
	}

	void onTick(World &world, long_t tick) override
	{
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 60.0f);
		if (tick % period != 0)
			return;
		Level &level = world.level;
		for (int_t i = 0; i < count; i++)
		{
			const double x = camX + (random.nextDouble() - 0.5) * 48.0;
			const double z = camZ + (random.nextDouble() - 0.5) * 48.0;
			const double y = static_cast<double>(level.getHeightmap(Mth::floor(x), Mth::floor(z))) + 0.5;
			auto tnt = std::make_shared<PrimedTnt>(level, x, y, z);
			// Stagger fuses so explosions spread across the period.
			tnt->life = 20 + random.nextInt(60);
			level.addEntity(tnt);
			primed++;
		}
	}

	void report(World &, std::vector<std::string> &lines) override
	{
		lines.push_back("tnt_primed " + std::to_string(primed));
	}
};

// Non-mob entities at volume: dropped items, arrows in the ground, paintings
// on a wall, minecarts. These are the fixed-geometry renderers the audit
// flagged for transient draw packets (paintings, arrows). `--count` items,
// arrows and paintings each scale from it.
class EntitiesScenario : public Scenario
{
	int_t count = 400;
	int_t items = 0, arrows = 0, paintings = 0, carts = 0;
	double camX = 0.0, camY = 0.0, camZ = 0.0;

public:
	const char *name() const override { return "entities"; }
	int defaultFrames() const override { return 900; }

	void setup(World &world, const Params &params) override
	{
		count = static_cast<int_t>(params.intOr("count", 400));
		Level &level = world.level;
		Random random(world.seed);
		const int_t px = Mth::floor(world.player.x);
		const int_t pz = Mth::floor(world.player.z);
		const int_t y0 = level.getHeightmap(px, pz) + 2;

		// Flat stone stage so nothing falls into terrain, with a wall along
		// its north edge for the paintings.
		const int_t half = 32;
		for (int_t x = px - half; x <= px + half; x++)
			for (int_t z = pz - half; z <= pz + half; z++)
			{
				level.setTile(x, y0 - 1, z, Tile::rock.id);
				for (int_t y = y0; y < y0 + 4; y++)
					level.setTile(x, y, z, z == pz - half ? Tile::rock.id : 0);
			}

		for (int_t i = 0; i < count; i++)
		{
			const double x = px + (random.nextDouble() - 0.5) * 2.0 * (half - 2);
			const double z = pz - half + 2 + random.nextDouble() * (2.0 * half - 4);
			auto item = std::make_shared<EntityItem>(level, x, static_cast<double>(y0) + 0.5, z,
				ItemStack(Tile::cobblestone.id, 1));
			level.addEntity(item);
			items++;
		}
		for (int_t i = 0; i < count / 2; i++)
		{
			const double x = px + (random.nextDouble() - 0.5) * 2.0 * (half - 2);
			const double z = pz - half + 2 + random.nextDouble() * (2.0 * half - 4);
			auto arrow = std::make_shared<Arrow>(level, x, static_cast<double>(y0) + 0.3, z);
			level.addEntity(arrow);
			arrows++;
		}
		// Paintings along the north wall. Any that do not fit their wall drop
		// as items; the report counts survivors so a wrong facing is visible.
		for (int_t x = px - half + 1; x < px + half; x += 2)
		{
			auto painting = std::make_shared<Painting>(level, x, y0 + 1, pz - half + 1, 0);
			level.addEntity(painting);
			paintings++;
		}
		for (int_t i = 0; i < count / 8; i++)
		{
			const double x = px + (random.nextDouble() - 0.5) * 2.0 * (half - 2);
			const double z = pz - half + 2 + random.nextDouble() * (2.0 * half - 4);
			auto cart = std::make_shared<Minecart>(level, x, static_cast<double>(y0) + 0.5, z, 0);
			level.addEntity(cart);
			carts++;
		}

		camX = static_cast<double>(px);
		camZ = static_cast<double>(pz + half) + 4.0;
		camY = static_cast<double>(y0 + 10);
		pinPlayer(world.player, camX, camY, camZ, 180.0f, 25.0f);
	}

	void onTick(World &world, long_t) override
	{
		pinPlayer(world.player, camX, camY, camZ, 180.0f, 25.0f);
	}

	void report(World &world, std::vector<std::string> &lines) override
	{
		lines.push_back("entities_items_spawned " + std::to_string(items));
		lines.push_back("entities_arrows_spawned " + std::to_string(arrows));
		lines.push_back("entities_paintings_spawned " + std::to_string(paintings));
		lines.push_back("entities_carts_spawned " + std::to_string(carts));
		lines.push_back("entities_items_alive " + std::to_string(world.level.countInstanceOf<EntityItem>()));
		lines.push_back("entities_arrows_alive " + std::to_string(world.level.countInstanceOf<Arrow>()));
		lines.push_back("entities_paintings_alive " + std::to_string(world.level.countInstanceOf<Painting>()));
	}
};

// A sealed, torch-lit hall 20 blocks underground: the whole surface grid is
// hidden behind rock yet, with the occlusion path still a `// TODO`, every
// section the frustum admits is drawn. The `render_stats` line (C/F/O/E) is
// the measurement for item #11. `--width`, `--depth`.
class CaveScenario : public Scenario
{
	double camX = 0.0, camY = 0.0, camZ = 0.0;

public:
	const char *name() const override { return "cave"; }
	int defaultFrames() const override { return 600; }

	void setup(World &world, const Params &params) override
	{
		const int_t width = static_cast<int_t>(params.intOr("width", 48));
		const int_t depth = static_cast<int_t>(params.intOr("depth", 48));
		const int_t height = 6;
		Level &level = world.level;
		const int_t x0 = Mth::floor(world.player.x) - width / 2;
		const int_t z0 = Mth::floor(world.player.z) - depth / 2;
		int_t y0 = level.getHeightmap(x0 + width / 2, z0 + depth / 2) - 20 - height;
		if (y0 < 8)
			y0 = 8;
		for (int_t x = x0 - 1; x <= x0 + width; x++)
			for (int_t z = z0 - 1; z <= z0 + depth; z++)
			{
				level.setTile(x, y0 - 1, z, Tile::rock.id);
				level.setTile(x, y0 + height, z, Tile::rock.id);
				for (int_t y = y0; y < y0 + height; y++)
				{
					const bool wall = x == x0 - 1 || x == x0 + width || z == z0 - 1 || z == z0 + depth;
					level.setTile(x, y, z, wall ? Tile::rock.id : 0);
				}
			}
		for (int_t x = x0 + 4; x < x0 + width; x += 8)
			for (int_t z = z0 + 4; z < z0 + depth; z += 8)
				level.setTileAndData(x, y0, z, Tile::torch.id, 5);

		camX = static_cast<double>(x0 + 2);
		camY = static_cast<double>(y0 + 1);
		camZ = static_cast<double>(z0 + depth / 2);
		pinPlayer(world.player, camX, camY, camZ, 270.0f, 0.0f);
	}

	void onTick(World &world, long_t) override
	{
		pinPlayer(world.player, camX, camY, camZ, 270.0f, 0.0f);
	}
};

// Sound at volume: a streaming record at setup (the multi-MB synchronous OGG
// decode, C7) and a burst of `--count` positional effects every `--period`
// ticks through `Level::playSound`, the path the game uses.
class SoundsScenario : public Scenario
{
	int_t count = 16;
	int_t period = 20;
	long_t played = 0;
	double camX = 0.0, camY = 0.0, camZ = 0.0;
	Random random;

public:
	SoundsScenario() : random(3LL) {}
	const char *name() const override { return "sounds"; }
	int defaultFrames() const override { return 900; }

	void setup(World &world, const Params &params) override
	{
		count = static_cast<int_t>(params.intOr("count", 16));
		period = static_cast<int_t>(params.intOr("period", 20));
		camX = world.player.x;
		camZ = world.player.z;
		camY = restingY(world);
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 20.0f);
		world.level.playStreamingMusic(u"13", Mth::floor(camX), Mth::floor(camY), Mth::floor(camZ));
	}

	void onTick(World &world, long_t tick) override
	{
		pinPlayer(world.player, camX, camY, camZ, 0.0f, 20.0f);
		if (tick % period != 0)
			return;
		static const char16_t *const names[] = {
			u"random.click", u"step.stone", u"step.grass", u"random.explode", u"random.bow", u"mob.zombie"
		};
		for (int_t i = 0; i < count; i++)
		{
			const double x = camX + (random.nextDouble() - 0.5) * 32.0;
			const double z = camZ + (random.nextDouble() - 0.5) * 32.0;
			world.level.playSound(x, camY, z, names[i % 6], 1.0f, 0.8f + random.nextFloat() * 0.4f);
			played++;
		}
	}

	void report(World &, std::vector<std::string> &lines) override
	{
		lines.push_back("sounds_played " + std::to_string(played));
	}
};

// Types a chat line, normally a `/stress ...` command for the server-side
// plugin (stress-plugin/), and then observes. The server does the moving,
// spawning or building, so every position the client sees is legal and the
// measurement is the client's packet, chunk and render path under that load.
// `--cmd` the line, `--every` ticks between repeats (0 = once), `--frames`.
class CommandScenario : public Scenario
{
	std::string cmd;
	long_t every = 0;
	long_t sent = 0;

public:
	const char *name() const override { return "command"; }
	int defaultFrames() const override { return 1200; }
	bool supportsMultiplayer() const override { return true; }
	bool settleBeforeMeasure() const override { return false; }

	void setup(World &world, const Params &params) override
	{
		cmd = params.stringOr("cmd", "/stress");
		every = static_cast<long_t>(params.intOr("every", 0));
		send(world);
	}

	void onTick(World &world, long_t tick) override
	{
		if (every > 0 && tick > 0 && tick % every == 0)
			send(world);
	}

	void report(World &, std::vector<std::string> &lines) override
	{
		lines.push_back("command_sent " + std::to_string(sent));
	}

private:
	void send(World &world)
	{
		MultiPlayerLevel *mp = dynamic_cast<MultiPlayerLevel *>(&world.level);
		if (mp == nullptr || mp->getConnection() == nullptr)
			return;
		mp->getConnection()->addToSendQueue(new Packet3Chat(String::fromUTF8(cmd)));
		sent++;
	}
};

std::unique_ptr<Scenario> makeScenario(const std::string &name)
{
	if (name == "idle") return std::make_unique<IdleScenario>();
	if (name == "spin") return std::make_unique<SpinScenario>();
	if (name == "walk") return std::make_unique<WalkScenario>();
	if (name == "daycycle") return std::make_unique<DayCycleScenario>();
	if (name == "travel") return std::make_unique<TravelScenario>();
	if (name == "farlands") return std::make_unique<FarLandsScenario>();
	if (name == "building") return std::make_unique<BuildingScenario>();
	if (name == "lighting") return std::make_unique<LightingScenario>();
	if (name == "fluids") return std::make_unique<FluidsScenario>();
	if (name == "tnt") return std::make_unique<TntScenario>();
	if (name == "mobs") return std::make_unique<MobsScenario>();
	if (name == "entities") return std::make_unique<EntitiesScenario>();
	if (name == "command") return std::make_unique<CommandScenario>();
	if (name == "cave") return std::make_unique<CaveScenario>();
	if (name == "sounds") return std::make_unique<SoundsScenario>();
	return nullptr;
}

std::vector<std::string> scenarioNames()
{
	return { "idle", "spin", "walk", "daycycle", "travel", "farlands", "building",
		"lighting", "fluids", "tnt", "mobs", "entities", "cave", "sounds", "command" };
}

}
