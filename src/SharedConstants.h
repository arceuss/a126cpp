#pragma once

#include "java/String.h"

namespace SharedConstants
{

static const jstring VERSION_STRING = u"Alpha v1.2.6";
extern const int NETWORK_PROTOCOL_VERSION;
extern const int maxChatLength;
extern const jstring acceptableLetters;

// Index of a character in `acceptableLetters`, or -1 when it has no glyph.
// Same result as `acceptableLetters.find(c)`, without the linear scan that
// dominates sign and chat text rendering.
int_t letterIndex(char_t c);

}
