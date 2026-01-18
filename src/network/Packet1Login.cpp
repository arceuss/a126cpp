#include "network/Packet1Login.h"
#include "network/NetHandler.h"

Packet1Login::Packet1Login()
	: protocolVersion(0)
	, username(u"")
	, field_4074_d(0)
	, field_4073_e(0)
{
}

Packet1Login::Packet1Login(const jstring& username, int_t protocolVersion)
	: protocolVersion(protocolVersion)
	, username(username)
	, field_4074_d(0)
	, field_4073_e(0)
{
}

void Packet1Login::readPacketData(SocketInputStream& in)
{
	// Java: EXACT ORDER - must match byte sequence
	// this.protocolVersion = var1.readInt();
	this->protocolVersion = in.readInt();
	
	// this.username = readString(var1, 16);
	this->username = Packet::readString(in, 16);
	
	// this.field_4074_d = var1.readLong();
	this->field_4074_d = in.readLong();
	
	// this.field_4073_e = var1.readByte();
	this->field_4073_e = in.readByte();
}

void Packet1Login::writePacketData(SocketOutputStream& out)
{
	// Java: EXACT ORDER - must match byte sequence
	// var1.writeInt(this.protocolVersion);
	out.writeInt(this->protocolVersion);
	
	// writeString(this.username, var1);
	Packet::writeString(this->username, out);
	
	// var1.writeLong(this.field_4074_d);
	out.writeLong(this->field_4074_d);
	
	// var1.writeByte(this.field_4073_e);
	out.writeByte(this->field_4073_e);
}

void Packet1Login::processPacket(NetHandler* handler)
{
	// Java: var1.handleLogin(this);
	handler->handleLogin(this);
}

int Packet1Login::getPacketSize()
{
	// Java: return 4 + this.username.length() + 4 + 5;
	// Note: username.length() is in characters (UTF-16), so 2 bytes per char
	// String: 2 bytes for length + (length * 2) bytes for UTF-16 chars = 2 + length*2
	// int = 4, long = 8, byte = 1
	// Java calculation: 4 (int) + (2 + username.length()*2) (string) + 8 (long) + 1 (byte) = 15 + username.length()*2
	// But Java says: 4 + username.length() + 4 + 5 = 13 + username.length()
	// Looking at Java code, it seems to count: 4 (protocolVersion) + 2 (string length short) + username.length()*2 (UTF-16 chars) + 8 (long) + 1 (byte)
	// But the formula suggests they're counting differently. Let's match Java exactly:
	// Java formula: 4 + this.username.length() + 4 + 5 = 13 + username.length()
	// This suggests: 4 (protocolVersion) + username.length() (treating as UTF-16 code units, so 2 bytes each?) + 4 (maybe long is counted as 4?) + 5
	// Actually, wait - Java String.length() returns number of characters, and each char is 2 bytes in UTF-16
	// So: 4 (int) + 2 (short for string length) + username.length()*2 (UTF-16 bytes) + 8 (long) + 1 (byte) = 15 + username.length()*2
	// But Java says 4 + username.length() + 4 + 5. This is confusing.
	// Let me trust the Java calculation exactly as written:
	return 4 + static_cast<int>(username.length()) + 4 + 5;
}

int Packet1Login::getPacketId() const
{
	return 1;
}
