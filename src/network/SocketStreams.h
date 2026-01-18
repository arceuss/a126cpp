#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET SocketHandle;
#define INVALID_SOCKET_HANDLE INVALID_SOCKET
#else
#include <sys/socket.h>
#include <unistd.h>
typedef int SocketHandle;
#define INVALID_SOCKET_HANDLE (-1)
#endif

#include "java/Type.h"
#include "java/String.h"
#include <vector>
#include <cstring>

// Wrapper for DataInputStream - matches Java DataInputStream behavior exactly
class SocketInputStream {
private:
	SocketHandle socket;
	std::vector<byte_t> readBuffer;
	size_t readBufferPos;

public:
	SocketInputStream(SocketHandle sock);
	~SocketInputStream();

	// DataInputStream methods - must match Java byte sequence exactly
	int read();  // Reads single byte, returns -1 on EOF
	void readFully(byte_t* buf, size_t len);  // Reads exactly len bytes (matching Java readFully)
	byte_t readByte();  // Reads single byte
	short_t readShort();  // Reads big-endian short
	int_t readInt();  // Reads big-endian int
	long_t readLong();  // Reads big-endian long
	float readFloat();  // Reads big-endian float
	double readDouble();  // Reads big-endian double
	jstring readString(int maxLength);  // Reads UTF-16 string (Java readString)
	bool readBoolean();  // Reads boolean
	
	void close();
};

// Wrapper for DataOutputStream - matches Java DataOutputStream behavior exactly  
class SocketOutputStream {
private:
	SocketHandle socket;
	std::vector<byte_t> writeBuffer;
	size_t writeBufferPos;
	static const size_t BUFFER_SIZE = 5120;  // Java BufferedOutputStream default size

public:
	SocketOutputStream(SocketHandle sock);
	~SocketOutputStream();

	// DataOutputStream methods - must match Java byte sequence exactly
	void write(int b);  // Writes single byte
	void write(const byte_t* buf, size_t len);  // Writes bytes
	void writeByte(byte_t b);  // Writes single byte
	void writeShort(short_t s);  // Writes big-endian short
	void writeInt(int_t i);  // Writes big-endian int
	void writeLong(long_t l);  // Writes big-endian long
	void writeFloat(float f);  // Writes big-endian float
	void writeDouble(double d);  // Writes big-endian double
	void writeString(const jstring& str, int maxLength);  // Writes UTF-16 string (Java writeString)
	void writeBoolean(bool b);  // Writes boolean
	
	void flush();  // Flushes buffer to socket
	void close();
};
