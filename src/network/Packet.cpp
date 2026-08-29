#include "network/Packet.h"
#include "java/System.h"
#include "java/IOUtil.h"
#include <iostream>
#include <stdexcept>
#include <mutex>

// Static member definitions
std::map<int, std::function<Packet*()>> Packet::packetIdToFactory;
std::map<std::type_index, int> Packet::packetClassToId;
std::set<int> Packet::clientPacketIdList;
std::set<int> Packet::serverPacketIdList;

struct PacketCounterState
{
	int_t totalPackets = 0;
	long_t totalBytes = 0;

	void addPacket(int_t size)
	{
		++totalPackets;
		totalBytes += static_cast<long_t>(size);
	}
};

static std::map<int_t, PacketCounterState> packetStats;
static int_t totalPacketsCount = 0;

Packet::Packet()
	: creationTimeMillis(System::currentTimeMillis())
	, isChunkDataPacket(false)
{
}

void Packet::addIdClassMapping(int packetId, bool isClientPacket, bool isServerPacket,
                                std::function<Packet*()> factory)
{
	if (packetIdToFactory.find(packetId) != packetIdToFactory.end())
	{
		throw std::invalid_argument("Duplicate packet id: " + std::to_string(packetId));
	}
	
	// Store factory function for creating packets
	packetIdToFactory[packetId] = factory;
	
	// Note: We can't store class-to-ID mapping here since we're using factory functions
	// Each packet class must register itself with getPacketIdForType when needed
	
	if (isClientPacket)
	{
		clientPacketIdList.insert(packetId);
	}
	
	if (isServerPacket)
	{
		serverPacketIdList.insert(packetId);
	}
}

Packet* Packet::getNewPacket(int packetId)
{
	ensurePacketRegistryInitialized();
	
	auto it = packetIdToFactory.find(packetId);
	if (it == packetIdToFactory.end())
	{
		return nullptr;
	}
	
	try
	{
		return it->second();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error creating packet " << packetId << ": " << e.what() << std::endl;
		return nullptr;
	}
}

int Packet::getPacketIdForType(const std::type_info& type)
{
	ensurePacketRegistryInitialized();
	std::type_index typeIdx(type);
	auto it = packetClassToId.find(typeIdx);
	if (it != packetClassToId.end())
	{
		return it->second;
	}
	// If not found in map, we'll need packet classes to implement getPacketId() directly
	throw std::runtime_error("Packet type not registered: " + std::string(type.name()));
}


std::unique_ptr<Packet> Packet::readPacket(SocketInputStream& in, bool isServerHandler)
{
	ensurePacketRegistryInitialized();
	
	// Java: int var4 = var0.read();
	int packetIdByte = in.read();
	if (packetIdByte == -1)
	{
		return nullptr;  // EOF
	}
	
	int packetId = packetIdByte & 0xFF;

	// Direct Packet.java:128-133 direction and registration checks.
	if ((isServerHandler && serverPacketIdList.find(packetId) == serverPacketIdList.end())
		|| (!isServerHandler && clientPacketIdList.find(packetId) == clientPacketIdList.end()))
		throw std::runtime_error("Bad packet id " + std::to_string(packetId));

	std::unique_ptr<Packet> packet(getNewPacket(packetId));
	if (packet == nullptr)
		throw std::runtime_error("Bad packet id " + std::to_string(packetId));

	try
	{
		packet->readPacketData(in);
	}
	catch (const EOFException &)
	{
		// Alpha Packet.java:137-140.
		std::cout << "Reached end of stream" << std::endl;
		return nullptr;
	}

	packetStats[packetId].addPacket(packet->getPacketSize());
	++totalPacketsCount;
	if (totalPacketsCount % 1000 == 0)
	{
		// Alpha's block is intentionally empty (Packet.java:147-149).
	}

	return packet;
}

void Packet::writePacket(Packet* packet, SocketOutputStream& out)
{
	// Java: var1.write(var0.getPacketId());
	out.write(packet->getPacketId());
	
	// Java: var0.writePacketData(var1);
	packet->writePacketData(out);
}

jstring Packet::readString(SocketInputStream& in, int maxLength)
{
	// Java: short var2 = var0.readShort();
	short_t length = in.readShort();
	
	if (length > maxLength)
	{
		throw std::runtime_error("Received string length longer than maximum allowed (" + 
		                         std::to_string(length) + " > " + std::to_string(maxLength) + ")");
	}
	
	if (length < 0)
	{
		throw std::runtime_error("Received string length is less than zero! Weird string!");
	}
	
	// Java: StringBuilder var3 = new StringBuilder();
	//       for(int var4 = 0; var4 < var2; ++var4) {
	//           var3.append(var0.readChar());
	//       }
	jstring result;
	result.reserve(length);
	
	for (int i = 0; i < length; ++i)
	{
		// Java: readChar() reads 2 bytes (UTF-16 char)
		ushort_t ch = in.readShort();
		result += static_cast<char16_t>(ch);
	}
	
	return result;
}

void Packet::writeString(const jstring& str, SocketOutputStream& out, int maxLength)
{
	// Java: if(var0.length() > Short.MAX_VALUE) {
	//           throw new IOException("String too big");
	//       }
	if (str.length() > static_cast<size_t>(maxLength))
	{
		throw std::runtime_error("String too big");
	}
	
	// Java: var1.writeShort(var0.length());
	out.writeShort(static_cast<short_t>(str.length()));
	
	// Java: var1.writeChars(var0);
	for (size_t i = 0; i < str.length(); ++i)
	{
		// Java: writeChar() writes 2 bytes (UTF-16 char)
		out.writeShort(static_cast<short_t>(str[i]));
	}
}

// Overloads for std::ostream/std::istream (used by DataWatcher)
// These match Java Packet.readString/writeString but work with std::ostream/std::istream
jstring Packet::readString(std::istream& in, int maxLength)
{
	// Java: short var2 = var0.readShort();
	short_t length = IOUtil::readShort(in);
	
	if (length > maxLength)
	{
		throw std::runtime_error("Received string length longer than maximum allowed (" + 
		                         std::to_string(length) + " > " + std::to_string(maxLength) + ")");
	}
	
	if (length < 0)
	{
		throw std::runtime_error("Received string length is less than zero! Weird string!");
	}
	
	// Java: StringBuilder var3 = new StringBuilder();
	//       for(int var4 = 0; var4 < var2; ++var4) {
	//           var3.append(var0.readChar());
	//       }
	jstring result;
	result.reserve(length);
	
	for (int i = 0; i < length; ++i)
	{
		// Java: readChar() reads 2 bytes (UTF-16 char) via IOUtil::readChar
		char_t ch = IOUtil::readChar(in);
		result += static_cast<char16_t>(ch);
	}
	
	return result;
}

void Packet::writeString(const jstring& str, std::ostream& out, int maxLength)
{
	// Java: if(var0.length() > Short.MAX_VALUE) {
	//           throw new IOException("String too big");
	//       }
	if (str.length() > static_cast<size_t>(maxLength))
	{
		throw std::runtime_error("String too big");
	}
	
	// Java: var1.writeShort(var0.length());
	IOUtil::writeShort(out, static_cast<short_t>(str.length()));
	
	// Java: var1.writeChars(var0);
	for (size_t i = 0; i < str.length(); ++i)
	{
		// Java: writeChar() writes 2 bytes (UTF-16 char) via IOUtil::writeChar
		IOUtil::writeChar(out, static_cast<char_t>(str[i]));
	}
}

// Initialize packet registry - must be called at program startup
// This is implemented in PacketRegistration.cpp to avoid circular dependencies
// DO NOT define it here - it's defined in PacketRegistration.cpp
