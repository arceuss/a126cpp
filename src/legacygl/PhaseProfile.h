#pragma once

// Per-phase cycle accounting for the renderer hot path.
//
// The frame-time harness can tell which backend is slow but not which part of a
// draw is slow, which makes it easy to "optimise" a phase that was never the
// cost. These accumulators attribute cycles to named phases so an optimisation
// is chosen from measurement instead of resemblance to a previous fix.
//
// Enabled by setting A126_PHASE_PROFILE. When disabled each site costs one
// predictable branch, so the profiling build and the shipping build are the
// same binary and the same code path.
//
// The counter is the CPU timestamp counter. It is not a wall-clock source and
// is not comparable across machines or frequency states; it is used only to
// apportion one frame's CPU cost between phases of that same frame.

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <chrono>
#endif

namespace legacygl
{

enum class DrawPhase : std::size_t
{
	// Semantic core: turning a recorded/immediate call into a ResolvedDraw.
	CoreResolve,
	// Semantic core: derived matrices (normal matrix and its rescale factor).
	CoreMatrices,
	// Semantic core: canonical primitive decomposition or its cache lookup.
	CorePrimitives,
	// Semantic core: copying light, material and model-ambient state.
	CoreLighting,
	// Semantic core: resolving the bound texture object's sampled state.
	CoreTexture,
	// Backend: building or locating the vertex data for this draw.
	Geometry,
	// Backend: the subset of Geometry that converts and uploads vertex data
	// (transient draws and resident misses); Geometry minus this is the
	// resident-hit lookup cost.
	GeometryUpload,
	// Backend: packing resolved state into the backend's constant layout.
	StatePack,
	// Backend: getting that constant data to the GPU.
	StateUpload,
	// Backend: pipeline, descriptor, vertex-buffer and dynamic-state binds.
	Bind,
	// Backend: the draw call itself.
	Draw,
	Count
};

struct PhaseAccumulator
{
	std::uint64_t cycles = 0;
	std::uint64_t calls = 0;
};

// Mutable so the memory-probe build can switch it on at startup: a console has
// no environment to set the variable in, and a compile-time definition would
// not reach the backend libraries, which are separate targets.
inline bool phaseProfileEnabled = std::getenv("A126_PHASE_PROFILE") != nullptr;
inline PhaseAccumulator phaseAccumulators[static_cast<std::size_t>(DrawPhase::Count)];

inline std::uint64_t readPhaseCounter()
{
#if defined(_MSC_VER)
	return __rdtsc();
#else
	return static_cast<std::uint64_t>(
		std::chrono::steady_clock::now().time_since_epoch().count());
#endif
}

inline const char *phaseName(DrawPhase phase)
{
	switch (phase)
	{
	case DrawPhase::CoreMatrices: return "core_matrices";
	case DrawPhase::CorePrimitives: return "core_primitives";
	case DrawPhase::CoreResolve: return "core_resolve";
	case DrawPhase::Geometry: return "geometry";
	case DrawPhase::GeometryUpload: return "geometry_upload";
	case DrawPhase::CoreLighting: return "core_lighting";
	case DrawPhase::CoreTexture: return "core_texture";
	case DrawPhase::StatePack: return "state_pack";
	case DrawPhase::StateUpload: return "state_upload";
	case DrawPhase::Bind: return "bind";
	case DrawPhase::Draw: return "draw";
	default: return "unknown";
	}
}

class PhaseScope
{
public:
	explicit PhaseScope(DrawPhase phase) : phase(phase)
	{
		if (phaseProfileEnabled)
			start = readPhaseCounter();
	}

	~PhaseScope()
	{
		if (!phaseProfileEnabled)
			return;
		PhaseAccumulator &accumulator =
			phaseAccumulators[static_cast<std::size_t>(phase)];
		accumulator.cycles += readPhaseCounter() - start;
		accumulator.calls++;
	}

	PhaseScope(const PhaseScope &) = delete;
	PhaseScope &operator=(const PhaseScope &) = delete;

private:
	DrawPhase phase;
	std::uint64_t start = 0;
};

inline void resetPhaseProfile()
{
	for (std::size_t i = 0; i < static_cast<std::size_t>(DrawPhase::Count); i++)
		phaseAccumulators[i] = PhaseAccumulator();
}

}
