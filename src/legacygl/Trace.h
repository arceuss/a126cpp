#pragma once

#include <cstddef>
#include <sstream>
#include <string>

// Deterministic frontend call trace.
//
// Gate A of the parity plan: the game's GL call stream must not change when a
// backend changes. The trace records every frontend call in order, with its
// parameters, the display-list compile context and a hash of any pixel payload,
// so two runs can be compared byte for byte.
//
// Tracing is opt-in through the A126_LEGACYGL_TRACE environment variable (its
// value is the output path). When disabled, every call site costs one load and
// branch, and traceCall's arguments are never formatted.

namespace legacygl
{

bool traceEnabled();
void traceOpen(const char *path);
void traceClose();
void traceRawLine(const std::string &line);

// Scene capture arms this before game renderer work, then enables the configured
// trace only for its final production render call. The semantic context keeps
// its absolute sequence; subtraction is confined to trace output.
void traceCaptureFrameOnly();
void traceBeginCaptureFrame(long long sequenceBase);
void traceEndCaptureFrame();
long long traceSequence(long long absoluteSequence);

// Records which list is compiling, so the reader can tell a compiled command
// from an executed one. Zero means no list is being compiled.
void traceListContext(unsigned int list, unsigned int mode);

// Records that the following calls come from executing a list.
void traceExecutionContext(unsigned int list);

unsigned long long traceHash(const void *data, std::size_t size);

inline void traceAppendArgs(std::ostringstream &) {}

template <typename T>
void traceAppendOne(std::ostringstream &out, const T &value)
{
	out << value;
}

inline void traceAppendOne(std::ostringstream &out, const void *value)
{
	if (value == nullptr)
		out << "null";
	else
		out << "ptr";
}

inline void traceAppendOne(std::ostringstream &out, unsigned char value)
{
	out << static_cast<unsigned int>(value);
}

inline void traceAppendOne(std::ostringstream &out, signed char value)
{
	out << static_cast<int>(value);
}

template <typename First, typename... Rest>
void traceAppendArgs(std::ostringstream &out, const First &first, const Rest &...rest)
{
	traceAppendOne(out, first);
	if (sizeof...(rest) != 0)
		out << ", ";
	traceAppendArgs(out, rest...);
}

template <typename... Args>
void traceCall(long long sequence, const char *name, const Args &...args)
{
	if (!traceEnabled())
		return;

	std::ostringstream out;
	out << traceSequence(sequence) << ' ' << name << '(';
	traceAppendArgs(out, args...);
	out << ')';
	traceRawLine(out.str());
}

}
