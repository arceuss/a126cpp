#include "tools/BenchUtil.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#include "legacygl/PhaseProfile.h"

namespace benchutil
{

double percentile(const std::vector<double> &sorted, double fraction)
{
	if (sorted.empty())
		return 0.0;
	size_t index = static_cast<size_t>(fraction * (sorted.size() - 1));
	return sorted[index];
}

ProcessMemory processMemory()
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
#endif
	return result;
}

std::string heapHistogram(std::size_t topCount)
{
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
	if (rows.size() > topCount)
		rows.resize(topCount);
	std::string result;
	for (const auto &row : rows)
	{
		result += " size=" + std::to_string(row.first) +
			" count=" + std::to_string(row.second.first) +
			" bytes=" + std::to_string(row.second.second);
	}
	return result;
#else
	(void)topCount;
	return std::string();
#endif
}

void emitPhaseProfile(const std::function<void(const std::string &)> &emit)
{
	if (!legacygl::phaseProfileEnabled)
		return;

	std::uint64_t backendCycles = 0;
	for (std::size_t i = 0; i < static_cast<std::size_t>(legacygl::DrawPhase::Count); i++)
	{
		const legacygl::DrawPhase phase = static_cast<legacygl::DrawPhase>(i);
		const legacygl::PhaseAccumulator &accumulator = legacygl::phaseAccumulators[i];
		// CoreMatrices and CorePrimitives are nested inside CoreResolve, so
		// they are core detail rather than backend cost; the two upload
		// phases are nested inside Geometry for the same reason.
		if (phase != legacygl::DrawPhase::CoreResolve &&
			phase != legacygl::DrawPhase::CoreMatrices &&
			phase != legacygl::DrawPhase::CoreLighting &&
			phase != legacygl::DrawPhase::CoreTexture &&
			phase != legacygl::DrawPhase::CorePrimitives &&
			phase != legacygl::DrawPhase::GeometryUpload &&
			phase != legacygl::DrawPhase::GeometryResidentUpload)
			backendCycles += accumulator.cycles;
		emit(std::string("phase_") + legacygl::phaseName(phase) + "_cycles " +
			std::to_string(accumulator.cycles));
		emit(std::string("phase_") + legacygl::phaseName(phase) + "_calls " +
			std::to_string(accumulator.calls));
	}
	// CoreResolve brackets the sink call, so the core's own share is the
	// difference. Reported explicitly to keep the split unambiguous.
	const std::uint64_t totalCycles =
		legacygl::phaseAccumulators[static_cast<std::size_t>(
			legacygl::DrawPhase::CoreResolve)].cycles;
	emit("phase_core_only_cycles " + std::to_string(
		totalCycles > backendCycles ? totalCycles - backendCycles : 0));
	emit("phase_backend_cycles " + std::to_string(backendCycles));
}

}
