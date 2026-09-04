#include "tools/MemoryProbe.h"

#ifdef A126_ENABLE_MEMORY_PROBE

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "legacygl/Context.h"
#include "legacygl/PhaseProfile.h"
#include "world/level/chunk/LevelChunk.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
// Windows.h defines min/max macros that break std::max below.
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#elif defined(__SWITCH__)
#include <malloc.h>
#include <switch.h>
#else
#include <malloc.h>
#endif

namespace memoryprobe
{

static const int REPORT_INTERVAL_SECONDS = 5;
#ifdef __SWITCH__
// No working directory on the console; the log and crash dump live here too.
static const char *const REPORT_PATH = "sdmc:/switch/a126cpp-profile.txt";
#else
static const char *const REPORT_PATH = "a126cpp-profile.txt";
#endif

struct Accumulator
{
	std::int64_t totalNanoseconds = 0;
	std::int64_t peakNanoseconds = 0;
	std::int64_t samples = 0;
};

static std::array<Accumulator, static_cast<std::size_t>(Bucket::Count)> accumulators;
static std::chrono::steady_clock::time_point lastReport = std::chrono::steady_clock::now();
static bool reportStarted = false;
static std::uint64_t reportIndex = 0;

static const char *bucketName(Bucket bucket)
{
	switch (bucket)
	{
		case Bucket::Frame: return "frame";
		case Bucket::Tick: return "tick";
		case Bucket::LightUpdate: return "light-update";
		case Bucket::ChunkLoad: return "chunk-load";
		case Bucket::ChunkGenerate: return "chunk-generate";
		case Bucket::ChunkPopulate: return "chunk-populate";
		case Bucket::ChunkSave: return "chunk-save";
		case Bucket::ChunkRebuild: return "chunk-rebuild";
		case Bucket::ChunkCapture: return "chunk-capture";
		case Bucket::ChunkRegion: return "chunk-region";
		case Bucket::ChunkTesselate: return "chunk-tesselate";
		case Bucket::LevelCull: return "level-cull";
		case Bucket::LevelDirty: return "level-dirty";
		case Bucket::LevelOpaque: return "level-opaque";
		case Bucket::LevelEntities: return "level-entities";
		case Bucket::LevelParticles: return "level-particles";
		case Bucket::LevelTranslucent: return "level-translucent";
		case Bucket::LevelClouds: return "level-clouds";
		case Bucket::ItemInHand: return "item-in-hand";
		case Bucket::Gui: return "gui";
		case Bucket::GuiStats: return "gui-stats";
		case Bucket::GuiMemory: return "gui-memory";
		case Bucket::Swap: return "swap";
		case Bucket::TickPackets: return "tick-packets";
		case Bucket::TickEntities: return "tick-entities";
		case Bucket::TickLevel: return "tick-level";
		case Bucket::TickParticles: return "tick-particles";
		default: return "?";
	}
}

struct Tally
{
	std::uint64_t draws = 0;
	std::int64_t nanoseconds = 0;
	std::uint64_t samples = 0;
};
static std::unordered_map<const char *, Tally> tallies;

void addTally(const char *name, std::uint64_t draws, std::int64_t nanoseconds)
{
	Tally &tally = tallies[name];
	tally.draws += draws;
	tally.nanoseconds += nanoseconds;
	tally.samples++;
}

// Insertion order kept so the report reads in a stable order; a handful of
// literal names, so a linear scan beats a map.
static std::vector<CountEntry> counts;

void addCount(const char *name, std::int64_t count)
{
	for (CountEntry &entry : counts)
	{
		if (entry.name == name || std::strcmp(entry.name, name) == 0)
		{
			entry.count += count;
			return;
		}
	}
	counts.push_back(CountEntry{ name, count });
}

std::size_t counters(CountEntry *out, std::size_t capacity)
{
	std::size_t n = 0;
	for (const CountEntry &entry : counts)
	{
		if (n >= capacity)
			break;
		out[n++] = entry;
	}
	return n;
}

static const char *currentDirtySource = "dirty_sections_other";

void setDirtySource(const char *name)
{
	currentDirtySource = name;
}

const char *dirtySource()
{
	return currentDirtySource;
}

void addSample(Bucket bucket, std::int64_t nanoseconds)
{
	if (bucket >= Bucket::Count || nanoseconds < 0)
		return;

	Accumulator &accumulator = accumulators[static_cast<std::size_t>(bucket)];
	accumulator.totalNanoseconds += nanoseconds;
	accumulator.peakNanoseconds = std::max(accumulator.peakNanoseconds, nanoseconds);
	accumulator.samples++;
}

Scope::Scope(Bucket bucket)
	: bucket(bucket)
	, startNanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count())
{
}

Scope::~Scope()
{
	const std::int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
	addSample(bucket, now - startNanoseconds);
}

struct ProcessMemory
{
	std::uint64_t privateBytes = 0;
	std::uint64_t workingSetBytes = 0;
	std::uint64_t heapInUseBytes = 0;
};

static ProcessMemory processMemory()
{
	ProcessMemory result;
#if defined(_WIN32)
	PROCESS_MEMORY_COUNTERS_EX counters = {};
	counters.cb = sizeof(counters);
	if (GetProcessMemoryInfo(GetCurrentProcess(),
		reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters), sizeof(counters)))
	{
		result.privateBytes = static_cast<std::uint64_t>(counters.PrivateUsage);
		result.workingSetBytes = static_cast<std::uint64_t>(counters.WorkingSetSize);
	}
#else
	const struct mallinfo info = mallinfo();
	result.heapInUseBytes = static_cast<std::uint64_t>(info.uordblks);
	result.privateBytes = static_cast<std::uint64_t>(info.arena);
#endif
	return result;
}

// Walks the whole address space so the share of the footprint that is not busy
// heap can be named instead of inferred. PrivateUsage counts every committed
// private page, while HeapWalk only sees the CRT/Win32 heaps, so the difference
// lands here: graphics driver allocations, thread stacks, and anything that
// reserves through VirtualAlloc directly.
static std::string regionReport(std::size_t topCount)
{
	std::string out;

#if defined(_WIN32)
	// Heap segment bases, so heap-owned regions can be excluded from the rest.
	std::unordered_map<std::uintptr_t, bool> heapRegions;
	HANDLE heaps[64] = {};
	const DWORD heapCount = GetProcessHeaps(64, heaps);
	for (DWORD i = 0; i < heapCount && i < 64; i++)
	{
		PROCESS_HEAP_ENTRY entry = {};
		HeapLock(heaps[i]);
		while (HeapWalk(heaps[i], &entry))
		{
			if ((entry.wFlags & PROCESS_HEAP_REGION) == 0)
				continue;
			heapRegions[reinterpret_cast<std::uintptr_t>(entry.lpData)] = true;
		}
		HeapUnlock(heaps[i]);
	}

	struct RegionTotals
	{
		std::uint64_t privateBytes = 0;
		std::uint64_t mappedBytes = 0;
		std::uint64_t imageBytes = 0;
		std::uint64_t heapOwnedBytes = 0;
		std::uint64_t reservedBytes = 0;
	};
	RegionTotals totals;

	// Committed private regions outside the heaps, largest first: the rows that
	// name whoever is holding the unattributed share.
	std::vector<std::pair<std::uint64_t, std::uintptr_t>> outside;

	MEMORY_BASIC_INFORMATION info = {};
	std::uintptr_t address = 0;
	while (VirtualQuery(reinterpret_cast<LPCVOID>(address), &info, sizeof(info)) == sizeof(info))
	{
		const std::uint64_t size = static_cast<std::uint64_t>(info.RegionSize);
		if (info.State == MEM_COMMIT)
		{
			if (info.Type == MEM_IMAGE)
				totals.imageBytes += size;
			else if (info.Type == MEM_MAPPED)
				totals.mappedBytes += size;
			else
			{
				totals.privateBytes += size;
				const auto base = reinterpret_cast<std::uintptr_t>(info.AllocationBase);
				if (heapRegions.count(base) != 0 ||
					heapRegions.count(reinterpret_cast<std::uintptr_t>(info.BaseAddress)) != 0)
					totals.heapOwnedBytes += size;
				else
					outside.emplace_back(size, base);
			}
		}
		else if (info.State == MEM_RESERVE)
			totals.reservedBytes += size;

		const std::uintptr_t next = address + static_cast<std::uintptr_t>(info.RegionSize);
		if (next <= address)
			break;
		address = next;
	}

	// Coalesce by allocation base: one driver allocation often spans many
	// regions with different protections, which would otherwise read as noise.
	std::unordered_map<std::uintptr_t, std::uint64_t> byBase;
	for (const auto &region : outside)
		byBase[region.second] += region.first;

	std::vector<std::pair<std::uintptr_t, std::uint64_t>> bases(byBase.begin(), byBase.end());
	std::sort(bases.begin(), bases.end(), [](const auto &a, const auto &b)
	{
		return a.second > b.second;
	});

	const double megabyte = 1024.0 * 1024.0;
	std::uint64_t outsideBytes = 0;
	for (const auto &region : outside)
		outsideBytes += region.first;

	char line[256];
	std::snprintf(line, sizeof(line),
		"    committed: %.2f MiB private (%.2f MiB heap-owned, %.2f MiB outside heaps "
		"in %llu allocations), %.2f MiB mapped, %.2f MiB image\n",
		static_cast<double>(totals.privateBytes) / megabyte,
		static_cast<double>(totals.heapOwnedBytes) / megabyte,
		static_cast<double>(outsideBytes) / megabyte,
		static_cast<unsigned long long>(bases.size()),
		static_cast<double>(totals.mappedBytes) / megabyte,
		static_cast<double>(totals.imageBytes) / megabyte);
	out += line;

	for (std::size_t i = 0; i < bases.size() && i < topCount; i++)
	{
		std::snprintf(line, sizeof(line),
			"    outside heap: base %016llx = %10.2f MiB\n",
			static_cast<unsigned long long>(bases[i].first),
			static_cast<double>(bases[i].second) / megabyte);
		out += line;
	}
#else
	(void)topCount;
	out += "    (region report is Windows only)\n";
#endif

	return out;
}

// Buckets every busy heap block by exact size. The dominant row names the
// allocation size the footprint is made of, which locates its owner far faster
// than guessing from class layouts.
static std::string heapHistogram(std::size_t topCount)
{
	std::string out;

#if defined(_WIN32)
	std::unordered_map<std::size_t, std::pair<std::uint64_t, std::uint64_t>> buckets;
	HANDLE heaps[64] = {};
	const DWORD heapCount = GetProcessHeaps(64, heaps);
	for (DWORD i = 0; i < heapCount && i < 64; i++)
	{
		PROCESS_HEAP_ENTRY entry = {};
		HeapLock(heaps[i]);
		while (HeapWalk(heaps[i], &entry))
		{
			if ((entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) == 0)
				continue;
			auto &bucket = buckets[entry.cbData];
			bucket.first++;
			bucket.second += entry.cbData;
		}
		HeapUnlock(heaps[i]);
	}

	std::vector<std::pair<std::size_t, std::pair<std::uint64_t, std::uint64_t>>> rows(
		buckets.begin(), buckets.end());
	std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b)
	{
		return a.second.second > b.second.second;
	});

	// Totals matter as much as the top rows: HeapWalk sees only the CRT/Win32
	// heaps, so "private minus heap" is the share held outside them (large
	// VirtualAlloc blocks and the graphics driver's own copies). Without this
	// the unaccounted remainder can only be guessed at.
	std::uint64_t totalBytes = 0;
	std::uint64_t totalBlocks = 0;
	for (const auto &row : rows)
	{
		totalBlocks += row.second.first;
		totalBytes += row.second.second;
	}

	char summary[192];
	std::snprintf(summary, sizeof(summary),
		"    heap total: %.2f MiB in %llu busy blocks across %lu heaps\n",
		static_cast<double>(totalBytes) / (1024.0 * 1024.0),
		static_cast<unsigned long long>(totalBlocks),
		static_cast<unsigned long>(heapCount));
	out += summary;

	char line[192];
	for (std::size_t i = 0; i < rows.size() && i < topCount; i++)
	{
		std::snprintf(line, sizeof(line),
			"    %10zu B x %-10llu = %10.2f MiB\n",
			rows[i].first,
			static_cast<unsigned long long>(rows[i].second.first),
			static_cast<double>(rows[i].second.second) / (1024.0 * 1024.0));
		out += line;
	}
#else
	(void)topCount;
	out += "    (heap histogram is Windows only)\n";
#endif

	return out;
}

void writeReport(const char *reason)
{
	FILE *file = std::fopen(REPORT_PATH, reportStarted ? "a" : "w");
	if (file == nullptr)
		return;
	reportStarted = true;

	const ProcessMemory processBytes = processMemory();
	const legacygl::Context::RetainedGeometry retained =
		legacygl::context().retainedGeometry();

	const double megabyte = 1024.0 * 1024.0;

	std::fprintf(file, "===== report %llu (%s) =====\n",
		static_cast<unsigned long long>(reportIndex++), reason);

	std::fprintf(file, "process: working set %.2f MiB, private %.2f MiB",
		static_cast<double>(processBytes.workingSetBytes) / megabyte,
		static_cast<double>(processBytes.privateBytes) / megabyte);
	if (processBytes.heapInUseBytes != 0)
	{
		std::fprintf(file, ", heap in use %.2f MiB",
			static_cast<double>(processBytes.heapInUseBytes) / megabyte);
	}
	std::fprintf(file, "\n");

	// The suspected dominant consumer: legacygl retains every display list's
	// decoded vertices on the CPU for the list's lifetime.
	std::fprintf(file,
		"legacygl retained: %llu lists (%llu defined), %llu geometries, "
		"%llu vertices\n",
		static_cast<unsigned long long>(retained.lists),
		static_cast<unsigned long long>(retained.definedLists),
		static_cast<unsigned long long>(retained.geometries),
		static_cast<unsigned long long>(retained.vertices));
	std::fprintf(file,
		"legacygl bytes:    %.2f MiB vertices (%zu B each), %.2f MiB commands\n",
		static_cast<double>(retained.vertexBytes) / megabyte,
		sizeof(legacygl::Vertex),
		static_cast<double>(retained.commandBytes) / megabyte);
	std::fprintf(file,
		"legacygl cache:    %llu batches, %llu primitives, %.2f MiB\n",
		static_cast<unsigned long long>(retained.cachedBatches),
		static_cast<unsigned long long>(retained.cachedPrimitives),
		static_cast<double>(retained.cachedBytes) / megabyte);
	std::fprintf(file,
		"legacygl total:    %.2f MiB retained on the CPU\n",
		static_cast<double>(retained.vertexBytes + retained.commandBytes +
			retained.cachedBytes) / megabyte);

	// The GPU-side twin of the retained geometry: what the backend keeps in its
	// paged buffer objects. Page capacity minus logical bytes is allocation
	// slack; process private minus page capacity is everything the driver adds.
	legacygl::Sink::ResidentStats resident;
	if (legacygl::context().backendResidentStats(resident))
	{
		std::fprintf(file,
			"backend resident:  %.2f MiB logical in %zu entries, "
			"%zu pages = %.2f MiB capacity\n",
			static_cast<double>(resident.logicalBytes) / megabyte,
			resident.entries, resident.pages,
			static_cast<double>(resident.pageCapacityBytes) / megabyte);
		std::fprintf(file,
			"backend batching:  %llu draws in %llu multidraws, %llu block overflows (cumulative)\n",
			static_cast<unsigned long long>(resident.batchedDraws),
			static_cast<unsigned long long>(resident.multidraws),
			static_cast<unsigned long long>(resident.batchBlockOverflows));
	}

	// Chunk block storage: 16*128*16 blocks plus three nibble layers.
	const long long chunks = LevelChunk::liveInstances;
	const double chunkBytes = static_cast<double>(chunks) *
		(16.0 * 128.0 * 16.0 + 3.0 * 16.0 * 128.0 * 16.0 / 2.0 + 256.0);
	std::fprintf(file, "level chunks:      %lld live, ~%.2f MiB block storage\n",
		chunks, chunkBytes / megabyte);

	// Which entity types issued the draws: the top entries by draw count.
	if (!tallies.empty())
	{
		std::vector<std::pair<const char *, Tally>> rows(tallies.begin(), tallies.end());
		std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b)
		{
			return a.second.draws > b.second.draws;
		});
		std::fprintf(file, "entity draws over the last interval:\n");
		for (std::size_t i = 0; i < rows.size() && i < 10; i++)
		{
			std::fprintf(file, "    %-36s %8llu renders, %10llu draws, %9.2f ms\n",
				rows[i].first,
				static_cast<unsigned long long>(rows[i].second.samples),
				static_cast<unsigned long long>(rows[i].second.draws),
				static_cast<double>(rows[i].second.nanoseconds) / 1.0e6);
		}
		tallies.clear();
	}

	if (!counts.empty())
	{
		std::fprintf(file, "counters over the last interval:\n");
		for (const CountEntry &entry : counts)
			std::fprintf(file, "    %-28s %12lld\n", entry.name, static_cast<long long>(entry.count));
		counts.clear();
	}

	std::fprintf(file, "timings over the last interval:\n");
	for (std::size_t i = 0; i < accumulators.size(); i++)
	{
		Accumulator &accumulator = accumulators[i];
		if (accumulator.samples == 0)
			continue;

		const double totalMs = static_cast<double>(accumulator.totalNanoseconds) / 1.0e6;
		const double meanMs = totalMs / static_cast<double>(accumulator.samples);
		std::fprintf(file,
			"    %-14s %8lld calls, %9.2f ms total, %7.3f ms mean, %7.3f ms peak\n",
			bucketName(static_cast<Bucket>(i)),
			static_cast<long long>(accumulator.samples), totalMs, meanMs,
			static_cast<double>(accumulator.peakNanoseconds) / 1.0e6);

		accumulator = Accumulator();
	}

	// Draw-path attribution from legacygl's phase counters. On MSVC the unit is
	// TSC cycles; elsewhere it is steady_clock ticks (nanoseconds on both glibc
	// and newlib), so the column is labelled by build rather than converted.
	// Only the share between phases is meaningful, plus calls per frame.
	if (legacygl::phaseProfileEnabled)
	{
#if defined(_MSC_VER)
		const char *unit = "tsc cycles";
#else
		const char *unit = "ns";
#endif
		std::fprintf(file, "draw phases over the last interval (%s):\n", unit);
		for (std::size_t i = 0; i < static_cast<std::size_t>(legacygl::DrawPhase::Count); i++)
		{
			const legacygl::PhaseAccumulator &phase = legacygl::phaseAccumulators[i];
			if (phase.calls == 0)
				continue;
			std::fprintf(file, "    %-16s %10llu calls, %14llu total, %8.1f per call\n",
				legacygl::phaseName(static_cast<legacygl::DrawPhase>(i)),
				static_cast<unsigned long long>(phase.calls),
				static_cast<unsigned long long>(phase.cycles),
				static_cast<double>(phase.cycles) / static_cast<double>(phase.calls));
		}
		legacygl::resetPhaseProfile();
	}

	std::fprintf(file, "address space:\n");
	std::fputs(regionReport(8).c_str(), file);

	std::fprintf(file, "largest heap blocks by total size:\n");
	std::fputs(heapHistogram(12).c_str(), file);
	std::fprintf(file, "\n");

	std::fclose(file);
}

void tick()
{
	// The probe build is the profiling build: attribute every frame from the
	// first tick on, so the first interval is not missing the draw phases.
	legacygl::phaseProfileEnabled = true;

	const auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::seconds>(now - lastReport).count() <
		REPORT_INTERVAL_SECONDS)
	{
		return;
	}

	lastReport = now;
	writeReport("interval");
}

}

#endif
