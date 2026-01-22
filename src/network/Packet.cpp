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
	// Ensure packet registry is initialized (matches Java static initializer)
	static bool initialized = false;
	if (!initialized)
	{
		// Initialize packet registry if empty (auto-initialization)
		if (packetIdToFactory.empty())
		{
			initializePacketRegistry();
		}
		initialized = true;
	}
	
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
	std::type_index typeIdx(type);
	auto it = packetClassToId.find(typeIdx);
	if (it != packetClassToId.end())
	{
		return it->second;
	}
	// If not found in map, we'll need packet classes to implement getPacketId() directly
	throw std::runtime_error("Packet type not registered: " + std::string(type.name()));
}

// Register a packet type's ID mapping (called from packet class constructor or initialization)
void registerPacketTypeId(const std::type_info& type, int packetId)
{
	std::type_index typeIdx(type);
	if (Packet::packetClassToId.find(typeIdx) != Packet::packetClassToId.end())
	{
		throw std::invalid_argument("Duplicate packet class registration");
	}
	Packet::packetClassToId[typeIdx] = packetId;
}

std::unique_ptr<Packet> Packet::readPacket(SocketInputStream& in, bool isServerHandler)
{
	// Ensure packet registry is initialized before reading
	// This ensures clientPacketIdList and serverPacketIdList are populated
	static std::mutex initMutex;
	static bool initialized = false;
	
	{
		std::lock_guard<std::mutex> lock(initMutex);
		if (!initialized || packetIdToFactory.empty())
		{
			initializePacketRegistry();
			initialized = true;
		}
	}
	
	// Java: int var4 = var0.read();
	int packetIdByte = in.read();
	if (packetIdByte == -1)
	{
		return nullptr;  // EOF
	}
	
	int packetId = packetIdByte & 0xFF;
	
	// Java: Check if packet ID is valid for handler type
	// Note: For client handler (isServerHandler=false), we check clientPacketIdList
	// For server handler (isServerHandler=true), we check serverPacketIdList
	if ((isServerHandler && serverPacketIdList.find(packetId) == serverPacketIdList.end()) ||
	    (!isServerHandler && clientPacketIdList.find(packetId) == clientPacketIdList.end()))
	{
		// Debug: Print registry state
		std::cerr << "Bad packet id " << packetId << " for handler (isServerHandler=" << isServerHandler << ")" << std::endl;
		std::cerr << "  clientPacketIdList size: " << clientPacketIdList.size() << std::endl;
		std::cerr << "  serverPacketIdList size: " << serverPacketIdList.size() << std::endl;
		std::cerr << "  packetIdToFactory size: " << packetIdToFactory.size() << std::endl;
		if (packetIdToFactory.find(packetId) != packetIdToFactory.end())
		{
			std::cerr << "  Packet ID " << packetId << " exists in factory but not in appropriate list!" << std::endl;
		}
		throw std::runtime_error("Bad packet id " + std::to_string(packetId));
	}
	
	// Java: var3 = getNewPacket(var4);
	Packet* packet = getNewPacket(packetId);
	if (packet == nullptr)
	{
		throw std::runtime_error("Bad packet id " + std::to_string(packetId));
	}
	
	// Java: var3.readPacketData(var0);
	try
	{
		packet->readPacketData(in);
	}
	catch (const std::runtime_error& e)
	{
		std::string msg = e.what();
		if (msg.find("end of stream") != std::string::npos || 
		    msg.find("EOF") != std::string::npos)
		{
			delete packet;
			return nullptr;
		}
		throw;
	}
	
	// Note: Packet stats tracking omitted for now (PacketCounter)
	
	return std::unique_ptr<Packet>(packet);
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
