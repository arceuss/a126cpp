#pragma once

#include "java/String.h"
#include "java/Type.h"

// Beta 1.2/newb12: GuiMessage - holds a single chat message
// Java: net.minecraft.client.GuiMessage
class GuiMessage
{
public:
	jstring string;
	int_t ticks;

	GuiMessage(const jstring &str);
};
