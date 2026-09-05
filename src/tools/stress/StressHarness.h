#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "java/Type.h"

class Minecraft;
class Level;
class LocalPlayer;

// Stress harness: drives the production client (unattended window, real or
// null sink) through scripted worst-case workloads and logs per-phase timings,
// process memory, and the game's own retention counters at a fixed sample
// interval. A run with the null sink measures everything above the backend
// boundary; the same run on a real backend adds that backend's cost, so the
// difference between the two logs is the backend.
namespace stress
{

// `--key value` pairs after the scenario name. Scenarios read their own keys.
class Params
{
public:
	std::map<std::string, std::string> values;

	bool has(const std::string &key) const;
	int intOr(const std::string &key, int fallback) const;
	double doubleOr(const std::string &key, double fallback) const;
	std::string stringOr(const std::string &key, const std::string &fallback) const;
};

struct Options
{
	std::string scenario;
	std::string logPath;
	// Write the final frame to this PNG (real backend only).
	std::string capturePath;
	// Options::fancyGraphics; -1 keeps the options.txt value.
	int fancyGraphics = -1;
	// Connect to a running server instead of generating a world. The scenario
	// then runs on the MultiPlayerLevel the server streams; only scenarios that
	// declare supportsMultiplayer() are accepted.
	std::string serverHost;
	int serverPort = 25565;
	// Username sent at login; empty keeps the one from options.txt.
	std::string user;
	// Load this saved world from .mcbetacpp/saves instead of generating one.
	std::string world;
	// Rendering sink: real backend or legacygl's resident null sink.
	bool nullSink = false;
	// Fullbright comparison mode. Use explicit propagation timings rather than
	// treating its frame-time delta as the lighting engine's cost.
	// Offline levels only; ignored with a warning for server-driven ones.
	bool noLighting = false;
	// Frames rendered after warm-up; 0 = scenario default.
	int frames = 0;
	// Frames between game ticks. 3 approximates a 60 Hz client at 20 TPS.
	int tickInterval = 3;
	int warmupFrames = 60;
	int sampleEvery = 100;
	// 0 = far. Same scale as Options::viewDistance.
	int viewDistance = 0;
	bool finishEachFrame = true;
	Params params;
};

struct World
{
	Minecraft &minecraft;
	Level &level;
	LocalPlayer &player;
	long_t seed;
	int tickInterval;
	// Server-owned level: block edits and spawns are not authoritative.
	bool online;
};

class Scenario
{
public:
	virtual ~Scenario() = default;

	virtual const char *name() const = 0;
	virtual int defaultFrames() const = 0;
	// True when the scenario only moves the player and observes, so it is
	// valid against a server's level too.
	virtual bool supportsMultiplayer() const { return false; }
	// Runs once after the level is bound and before warm-up. The harness times
	// it and samples memory around it.
	virtual void setup(World &world, const Params &params) = 0;
	// Whether the light queue is drained and every section rebuilt before the
	// warm-up. Scenarios that measure convergence itself leave it queued.
	virtual bool settleBeforeMeasure() const { return true; }
	// Runs after each game tick.
	virtual void onTick(World &world, long_t tick) = 0;
	// Extra `key value` lines for the report.
	virtual void report(World &world, std::vector<std::string> &lines) { (void)world; (void)lines; }
};

// Registry of scenarios by name. Defined in Scenarios.cpp.
std::unique_ptr<Scenario> makeScenario(const std::string &name);
std::vector<std::string> scenarioNames();

// Keeps the player exactly where it is: overwrites the interpolation
// history, zeroes velocity, and keeps it alive against mobs.
void pinPlayer(LocalPlayer &player, double x, double y, double z, float yRot, float xRot);

int run(const Options &options);

}
