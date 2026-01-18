#include "network/Packet3Chat.h"
#include "network/NetHandler.h"
#include "java/String.h"
#include <algorithm>

Packet3Chat::Packet3Chat()
	: message(u"")
{
}

Packet3Chat::Packet3Chat(const jstring& message)
{
	// Java: if(var1.length() > 119) {
	//           var1 = var1.substring(0, 119);
	//       }
	if (message.length() > 119)
	{
		this->message = message.substr(0, 119);
	}
	else
	{
		this->message = message;
	}
}

void Packet3Chat::readPacketData(SocketInputStream& in)
{
	// Java: this.message = readString(var1, 119);
	this->message = Packet::readString(in, 119);
}

void Packet3Chat::writePacketData(SocketOutputStream& out)
{
	// Java: writeString(this.message, var1);
	Packet::writeString(this->message, out);
}

void Packet3Chat::processPacket(NetHandler* handler)
{
	// Java: var1.handleChat(this);
	handler->handleChat(this);
}

int Packet3Chat::getPacketSize()
{
	// Java: return this.message.length();
	return static_cast<int>(message.length());
}

int Packet3Chat::getPacketId() const
{
	return 3;
}
