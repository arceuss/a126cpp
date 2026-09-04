#pragma once

// Periodic profiling probe, compiled only when A126_ENABLE_MEMORY_PROBE is on.
//
// Exists to answer two measured questions rather than guessed ones: where the
// resident bytes go while walking around a populated world, and which of the
// per-frame subsystems actually costs the time. It appends a report to
// a126cpp-profile.txt beside the executable.
//
// Every entry point compiles to nothing when the probe is off, so the shipping
// build carries no instrumentation.

#include <cstddef>
#include <cstdint>

#ifdef A126_ENABLE_MEMORY_PROBE

namespace memoryprobe
{

// Named accumulators, reported as total and mean per interval.
enum class Bucket
{
	Frame,
	Tick,
	LightUpdate,
	ChunkLoad,
	ChunkGenerate,
	ChunkPopulate,
	ChunkSave,
	ChunkRebuild,
	ChunkCapture,
	ChunkRegion,
	ChunkTesselate,
	// Renderer phases, so a frame's non-draw CPU can be attributed. Nested
	// inside Frame; the draw path itself is attributed by legacygl's phases.
	LevelCull,
	LevelDirty,
	LevelOpaque,
	LevelEntities,
	LevelParticles,
	LevelTranslucent,
	LevelClouds,
	ItemInHand,
	Gui,
	GuiStats,
	GuiMemory,
	Swap,
	TickPackets,
	TickEntities,
	TickLevel,
	TickParticles,
	Count
};

// Adds a sample in nanoseconds.
void addSample(Bucket bucket, std::int64_t nanoseconds);

// Scoped timer for a bucket.
class Scope
{
public:
	explicit Scope(Bucket bucket);
	~Scope();

private:
	Bucket bucket;
	std::int64_t startNanoseconds;
};

// Called once per frame. Writes a report when the interval has elapsed.
void tick();

// Forces a report, for startup and shutdown snapshots.
void writeReport(const char *reason);

// Per-name tally of resolved draws and time, for attributing a draw-count
// spike to the entity type that issued it. `name` must outlive the interval
// (a type name or string literal).
void addTally(const char *name, std::uint64_t draws, std::int64_t nanoseconds);

// Named event counters, reported per interval. Used to attribute which
// caller marks render sections dirty (the audit's fixture F1). `name` must
// outlive the interval (a string literal).
void addCount(const char *name, std::int64_t count);
// Snapshot of the counters accumulated since the last report, for a harness
// that logs them alongside its own samples.
struct CountEntry
{
	const char *name;
	std::int64_t count;
};
std::size_t counters(CountEntry *out, std::size_t capacity);

// Names the code path currently marking render sections dirty, so the
// section count in LevelRenderer::setDirty can be attributed to it.
void setDirtySource(const char *name);
const char *dirtySource();

}

#define A126_PROBE_SCOPE(bucket) ::memoryprobe::Scope probeScope##__LINE__(bucket)
#define A126_PROBE_TICK() ::memoryprobe::tick()
#define A126_PROBE_REPORT(reason) ::memoryprobe::writeReport(reason)
#define A126_PROBE_TALLY(name, draws, nanoseconds) ::memoryprobe::addTally(name, draws, nanoseconds)
#define A126_PROBE_COUNT(name, count) ::memoryprobe::addCount(name, count)
#define A126_PROBE_DIRTY_SOURCE(name) ::memoryprobe::setDirtySource(name)
#define A126_PROBE_COUNT_DIRTY_SOURCE() ::memoryprobe::addCount(::memoryprobe::dirtySource(), 1)

#else

#define A126_PROBE_SCOPE(bucket) ((void)0)
#define A126_PROBE_TICK() ((void)0)
#define A126_PROBE_REPORT(reason) ((void)0)
#define A126_PROBE_TALLY(name, draws, nanoseconds) ((void)0)
#define A126_PROBE_COUNT(name, count) ((void)0)
#define A126_PROBE_DIRTY_SOURCE(name) ((void)0)
#define A126_PROBE_COUNT_DIRTY_SOURCE() ((void)0)
#endif
