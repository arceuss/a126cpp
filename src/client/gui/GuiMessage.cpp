#include "client/gui/GuiMessage.h"

// Beta 1.2/newb12: GuiMessage constructor
// Java: public GuiMessage(String string) {
//     this.string = string;
//     this.ticks = 0;
// }
GuiMessage::GuiMessage(const jstring &str) : string(str), ticks(0)
{
}
