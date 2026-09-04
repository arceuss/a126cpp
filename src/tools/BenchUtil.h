#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Measurement helpers shared by the developer fixtures (sign bench, chunk
// benches, stress harness). They read process state only; nothing here touches
// the game or GL.
namespace benchutil
{

// `sorted` must already be ascending.
double percentile(const std::vector<double> &sorted, double fraction);

struct ProcessMemory
{
	std::uint64_t privateBytes = 0;
	std::uint64_t workingSetBytes = 0;
};

// Win32 only; zeros elsewhere.
ProcessMemory processMemory();

// Buckets every busy heap block by exact size and returns the `topCount`
// largest buckets by total bytes as " size=N count=N bytes=N" fragments.
// Diffing two samples names the exact allocation size that a leak is made of,
// which locates its owner. Win32 only; empty elsewhere.
std::string heapHistogram(std::size_t topCount);

// Emits the legacygl draw-phase accumulators as `phase_<name>_cycles/_calls`
// lines plus the core/backend split. No-op unless `legacygl::phaseProfileEnabled`.
void emitPhaseProfile(const std::function<void(const std::string &)> &emit);

}
