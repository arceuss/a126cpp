#include "client/gui/ChatLine.h"

// Alpha 1.2.6: ChatLine constructor
// Java: public ChatLine(int var1, String var2, int var3) {
//     this.lineString = var2;
//     this.updateCounterCreated = var1;
//     this.chatLineID = var3;
// }
ChatLine::ChatLine(int_t updateCounter, const jstring &line, int_t id)
	: updateCounterCreated(updateCounter), lineString(line), chatLineID(id)
{
}

jstring ChatLine::getChatLineString() const
{
	return lineString;
}

int_t ChatLine::getUpdatedCounter() const
{
	return updateCounterCreated;
}

int_t ChatLine::getChatLineID() const
{
	return chatLineID;
}
