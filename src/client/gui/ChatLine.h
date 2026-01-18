#pragma once

#include "java/Type.h"
#include "java/String.h"

// Alpha 1.2.6: ChatLine - holds a single chat message line
// Java: net.minecraft.src.ChatLine
class ChatLine
{
private:
	int_t updateCounterCreated;
	jstring lineString;
	int_t chatLineID;

public:
	ChatLine(int_t updateCounter, const jstring &line, int_t id);
	
	jstring getChatLineString() const;
	int_t getUpdatedCounter() const;
	int_t getChatLineID() const;
};
