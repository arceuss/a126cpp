#include "legacygl/Trace.h"

#include <fstream>
#include <iostream>

namespace legacygl
{

static bool traceIsEnabled = false;
static bool traceFrameOnly = false;
static long long traceSequenceBase = 0;
static std::ofstream traceStream;

bool traceEnabled()
{
	return traceIsEnabled;
}

void traceOpen(const char *path)
{
	if (traceStream.is_open())
		traceStream.close();
	traceIsEnabled = false;
	traceSequenceBase = 0;
	if (path == nullptr || path[0] == '\0')
		return;

	traceStream.open(path, std::ios::out | std::ios::trunc);
	if (!traceStream.is_open())
	{
		std::cerr << "legacygl: cannot open trace file " << path << '\n';
		return;
	}

	traceIsEnabled = !traceFrameOnly;
	traceStream << "# a126cpp LegacyGL frontend trace\n";
}

void traceClose()
{
	if (traceStream.is_open())
		traceStream.close();
	traceIsEnabled = false;
	traceFrameOnly = false;
	traceSequenceBase = 0;
}

void traceCaptureFrameOnly()
{
	traceFrameOnly = true;
	traceIsEnabled = false;
}

void traceBeginCaptureFrame(long long sequenceBase)
{
	if (!traceFrameOnly || !traceStream.is_open())
		return;
	traceSequenceBase = sequenceBase;
	traceIsEnabled = true;
}

void traceEndCaptureFrame()
{
	if (!traceFrameOnly)
		return;
	traceClose();
}

long long traceSequence(long long absoluteSequence)
{
	return absoluteSequence - traceSequenceBase;
}

void traceRawLine(const std::string &line)
{
	if (!traceIsEnabled)
		return;
	traceStream << line << '\n';
}

void traceListContext(unsigned int list, unsigned int mode)
{
	if (!traceIsEnabled)
		return;
	if (list == 0)
		traceStream << "# compile end\n";
	else
		traceStream << "# compile begin list=" << list << " mode=0x" << std::hex << mode << std::dec << '\n';
}

void traceExecutionContext(unsigned int list)
{
	if (!traceIsEnabled)
		return;
	traceStream << "# execute list=" << list << '\n';
}

// FNV-1a. Only used to identify upload payloads in a trace, so a short,
// dependency-free hash is enough.
unsigned long long traceHash(const void *data, std::size_t size)
{
	if (data == nullptr)
		return 0;

	const unsigned char *bytes = static_cast<const unsigned char *>(data);
	unsigned long long hash = 14695981039346656037ULL;
	for (std::size_t i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= 1099511628211ULL;
	}
	return hash;
}

}
