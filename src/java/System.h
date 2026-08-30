#pragma once

#include "Type.h"

namespace System
{

long_t currentTimeMillis();
long_t nanoTime();

// Deterministic scene capture fixes wall-clock-driven render effects without
// changing the clock used by normal gameplay.
void setCurrentTimeMillisForCapture(long_t time);
void clearCurrentTimeMillisForCapture();

}
