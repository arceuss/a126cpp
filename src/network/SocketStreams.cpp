#include "network/SocketStreams.h"
#include "java/String.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#endif

// Matches the 30-second SO_RCVTIMEO NetworkManager sets on the socket.
static const int SOCKET_READ_TIMEOUT_MS = 30000;

// Waits for the socket to become readable before reading it.
//
// A blocking recv is not dependable on every platform: the Switch BSD service
// returns ETIMEDOUT straight away even when a packet is already pending, which
// the readers below would report as a socket timeout and tear the connection
// down. poll reports readability correctly, and this also gives a real timeout
// on stacks that ignore SO_RCVTIMEO.
static bool waitSocketReadable(SocketHandle socket)
{
#ifdef _WIN32
	WSAPOLLFD waiting = {};
	waiting.fd = socket;
	waiting.events = POLLRDNORM;
	return WSAPoll(&waiting, 1, SOCKET_READ_TIMEOUT_MS) > 0;
#else
	struct pollfd waiting = {};
	waiting.fd = socket;
	waiting.events = POLLIN;
	return ::poll(&waiting, 1, SOCKET_READ_TIMEOUT_MS) > 0;
#endif
}

// ============================================================================
// SocketInputStream - matches Java DataInputStream exactly
// ============================================================================

SocketInputStream::SocketInputStream(SocketHandle sock)
	: socket(sock)
	, readBuffer(4096)
	, readBufferPos(0)
{
	if (socket == INVALID_SOCKET_HANDLE)
	{
		throw std::runtime_error("Invalid socket handle");
	}
}

SocketInputStream::SocketInputStream(const std::vector<byte_t> &data)
	: socket(INVALID_SOCKET_HANDLE)
	, readBuffer(data)
	, readBufferPos(0)
	, memoryMode(true)
{
}

SocketInputStream::~SocketInputStream()
{
	close();
}

int SocketInputStream::read()
{
	if (memoryMode)
	{
		if (readBufferPos >= readBuffer.size())
			return -1;
		return static_cast<int_t>(static_cast<ubyte_t>(readBuffer[readBufferPos++]));
	}

	// Match Java InputStream.read():
	// - returns 0..255 for a byte
	// - returns -1 on EOF
	// - does NOT throw on EOF (DataInputStream.readFully does)
	// - throws SocketTimeoutException on timeout (which should be caught and handled gracefully)
	byte_t b;
	// Wait for readability first; see waitSocketReadable.
	if (!waitSocketReadable(socket))
		throw std::runtime_error("Socket timeout");

#ifdef _WIN32
	int result = recv(socket, reinterpret_cast<char*>(&b), 1, 0);
	if (result == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error == WSAETIMEDOUT)
		{
			// Timeout - Java's SocketTimeoutException (blocking socket with SO_RCVTIMEO set)
			// WSAEWOULDBLOCK only occurs with non-blocking sockets, not applicable here
			throw std::runtime_error("Socket timeout");  // Specific exception type for timeout
		}
		if (error == WSAECONNRESET || error == WSAECONNABORTED)
		{
			return -1;  // Connection closed
		}
		throw std::runtime_error("Socket read failed");
	}
#else
	ssize_t result = ::recv(socket, &b, 1, 0);
	if (result < 0)
	{
		if (errno == ETIMEDOUT)
		{
			// Timeout - Java's SocketTimeoutException (blocking socket with SO_RCVTIMEO set)
			// EAGAIN/EWOULDBLOCK only occur with non-blocking sockets, not applicable here
			throw std::runtime_error("Socket timeout");  // Specific exception type for timeout
		}
		if (errno == ECONNRESET || errno == EPIPE)
		{
			return -1;  // Connection closed
		}
		throw std::runtime_error("Socket read failed");
	}
#endif

	if (result == 0)
	{
		return -1;  // EOF
	}

	return static_cast<int>(b) & 0xFF;
}

void SocketInputStream::readFully(byte_t* buf, size_t len)
{
	if (memoryMode)
	{
		if (len > readBuffer.size() - readBufferPos)
			throw EOFException();
		if (len > 0)
			std::memcpy(buf, readBuffer.data() + readBufferPos, len);
		readBufferPos += len;
		return;
	}

	size_t totalRead = 0;
	while (totalRead < len)
	{
		// Wait for readability first; see waitSocketReadable.
		if (!waitSocketReadable(socket))
			throw std::runtime_error("Socket timeout");

#ifdef _WIN32
		int bytesRead = recv(socket, reinterpret_cast<char*>(buf + totalRead), static_cast<int>(len - totalRead), 0);
		if (bytesRead == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
		if (error == WSAETIMEDOUT)
		{
			// Timeout - Java's SocketTimeoutException
			throw std::runtime_error("Socket timeout");
		}
			if (error == WSAECONNRESET || error == WSAECONNABORTED)
			{
				throw EOFException();
			}
			throw std::runtime_error("Socket read failed");
		}
#else
		ssize_t bytesRead = ::recv(socket, buf + totalRead, len - totalRead, 0);
		if (bytesRead < 0)
		{
		if (errno == ETIMEDOUT)
		{
			// Timeout - Java's SocketTimeoutException
			throw std::runtime_error("Socket timeout");
		}
			if (errno == ECONNRESET || errno == EPIPE)
			{
				throw EOFException();
			}
			throw std::runtime_error("Socket read failed");
		}
#endif
		
		if (bytesRead == 0)
		{
			throw EOFException();
		}
		
		totalRead += bytesRead;
	}
}

byte_t SocketInputStream::readByte()
{
	byte_t b;
	readFully(&b, 1);
	return b;
}

short_t SocketInputStream::readShort()
{
	byte_t bytes[2];
	readFully(bytes, 2);
	const ushort_t bits = static_cast<ushort_t>(
		(static_cast<uint_t>(static_cast<ubyte_t>(bytes[0])) << 8)
		| static_cast<uint_t>(static_cast<ubyte_t>(bytes[1])));
	short_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

int_t SocketInputStream::readInt()
{
	byte_t bytes[4];
	readFully(bytes, 4);
	const uint_t bits =
		(static_cast<uint_t>(static_cast<ubyte_t>(bytes[0])) << 24)
		| (static_cast<uint_t>(static_cast<ubyte_t>(bytes[1])) << 16)
		| (static_cast<uint_t>(static_cast<ubyte_t>(bytes[2])) << 8)
		| static_cast<uint_t>(static_cast<ubyte_t>(bytes[3]));
	int_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

long_t SocketInputStream::readLong()
{
	byte_t bytes[8];
	readFully(bytes, 8);
	ulong_t bits = 0;
	for (int_t i = 0; i < 8; ++i)
		bits = (bits << 8) | static_cast<ulong_t>(static_cast<ubyte_t>(bytes[i]));
	long_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

float SocketInputStream::readFloat()
{
	int_t bits = readInt();
	// Java Float.intBitsToFloat: bit-exact conversion (avoid strict-aliasing UB)
	float f;
	static_assert(sizeof(float) == sizeof(int_t), "float must be 32-bit");
	std::memcpy(&f, &bits, sizeof(float));
	return f;
}

double SocketInputStream::readDouble()
{
	long_t bits = readLong();
	// Java Double.longBitsToDouble: bit-exact conversion (avoid strict-aliasing UB)
	double d;
	static_assert(sizeof(double) == sizeof(long_t), "double must be 64-bit");
	std::memcpy(&d, &bits, sizeof(double));
	return d;
}

jstring SocketInputStream::readString(int maxLength)
{
	// Java Packet.readString: reads short length, then UTF-16 chars
	short_t length = readShort();
	if (length < 0)
	{
		throw std::runtime_error("Received string length is less than zero");
	}
	if (length > maxLength)
	{
		throw std::runtime_error("Received string length longer than maximum allowed");
	}
	
	if (length == 0)
	{
		return u"";
	}
	
	// Read UTF-16 characters (Java uses char which is 16-bit)
	std::vector<ushort_t> chars(length);
	for (int i = 0; i < length; ++i)
	{
		chars[i] = readShort();  // Java char is 16-bit, read as short
	}
	
	return jstring(reinterpret_cast<const char16_t*>(chars.data()), length);
}

bool SocketInputStream::readBoolean()
{
	return readByte() != 0;
}

void SocketInputStream::close()
{
	// Socket is closed by NetworkManager, not here
}

// ============================================================================
// SocketOutputStream - matches Java DataOutputStream/BufferedOutputStream exactly
// ============================================================================

SocketOutputStream::SocketOutputStream(SocketHandle sock)
	: socket(sock)
	, writeBuffer(BUFFER_SIZE)
	, writeBufferPos(0)
{
	if (socket == INVALID_SOCKET_HANDLE)
	{
		throw std::runtime_error("Invalid socket handle");
	}
}

SocketOutputStream::SocketOutputStream(std::vector<byte_t> &sink)
	: socket(INVALID_SOCKET_HANDLE)
	, writeBuffer(BUFFER_SIZE)
	, writeBufferPos(0)
	, memorySink(&sink)
{
	sink.clear();
}

SocketOutputStream::~SocketOutputStream()
{
	flush();
	close();
}

void SocketOutputStream::write(int b)
{
	byte_t byte = static_cast<byte_t>(b);
	write(&byte, 1);
}

void SocketOutputStream::write(const byte_t* buf, size_t len)
{
	// BufferedOutputStream: write to buffer, flush when full
	size_t written = 0;
	while (written < len)
	{
		// Find how much space is in buffer
		size_t available = writeBuffer.size() - writeBufferPos;
		if (available == 0)
		{
			flush();  // Buffer full, flush it
			available = writeBuffer.size();
		}
		
		size_t toWrite = (len - written < available) ? (len - written) : available;
		std::memcpy(writeBuffer.data() + writeBufferPos, buf + written, toWrite);
		writeBufferPos += toWrite;
		written += toWrite;
	}
}

void SocketOutputStream::writeByte(byte_t b)
{
	write(&b, 1);
}

void SocketOutputStream::writeShort(short_t s)
{
	const ushort_t bits = static_cast<ushort_t>(s);
	const byte_t bytes[2] = {
		static_cast<byte_t>((bits >> 8) & 0xFF),
		static_cast<byte_t>(bits & 0xFF)
	};
	write(bytes, 2);
}

void SocketOutputStream::writeInt(int_t i)
{
	const uint_t bits = static_cast<uint_t>(i);
	const byte_t bytes[4] = {
		static_cast<byte_t>((bits >> 24) & 0xFF),
		static_cast<byte_t>((bits >> 16) & 0xFF),
		static_cast<byte_t>((bits >> 8) & 0xFF),
		static_cast<byte_t>(bits & 0xFF)
	};
	write(bytes, 4);
}

void SocketOutputStream::writeLong(long_t l)
{
	const ulong_t bits = static_cast<ulong_t>(l);
	const byte_t bytes[8] = {
		static_cast<byte_t>((bits >> 56) & 0xFF),
		static_cast<byte_t>((bits >> 48) & 0xFF),
		static_cast<byte_t>((bits >> 40) & 0xFF),
		static_cast<byte_t>((bits >> 32) & 0xFF),
		static_cast<byte_t>((bits >> 24) & 0xFF),
		static_cast<byte_t>((bits >> 16) & 0xFF),
		static_cast<byte_t>((bits >> 8) & 0xFF),
		static_cast<byte_t>(bits & 0xFF)
	};
	write(bytes, 8);
}

void SocketOutputStream::writeFloat(float f)
{
	uint_t bits = 0;
	if (std::isnan(f))
		bits = 0x7FC00000U;
	else
		std::memcpy(&bits, &f, sizeof(bits));
	writeInt(static_cast<int_t>(bits));
}

void SocketOutputStream::writeDouble(double d)
{
	ulong_t bits = 0;
	if (std::isnan(d))
		bits = 0x7FF8000000000000ULL;
	else
		std::memcpy(&bits, &d, sizeof(bits));
	writeLong(static_cast<long_t>(bits));
}

void SocketOutputStream::writeString(const jstring& str, int maxLength)
{
	// Java Packet.writeString: writes short length, then UTF-16 chars
	if (str.length() > maxLength)
	{
		throw std::runtime_error("String too big");
	}
	
	writeShort(static_cast<short_t>(str.length()));
	
	// Write UTF-16 characters (Java uses char which is 16-bit)
	const char16_t* chars = str.c_str();
	for (size_t i = 0; i < str.length(); ++i)
	{
		writeShort(static_cast<short_t>(chars[i]));  // Java char is 16-bit
	}
}

void SocketOutputStream::writeBoolean(bool b)
{
	writeByte(b ? 1 : 0);
}

void SocketOutputStream::flush()
{
	if (memorySink != nullptr)
	{
		memorySink->insert(memorySink->end(), writeBuffer.begin(),
			writeBuffer.begin() + static_cast<std::ptrdiff_t>(writeBufferPos));
		writeBufferPos = 0;
		return;
	}

	if (writeBufferPos > 0)
	{
		size_t totalSent = 0;
		
		while (totalSent < writeBufferPos)
		{
#ifdef _WIN32
			int sent = send(socket, reinterpret_cast<const char*>(writeBuffer.data() + totalSent), 
			                static_cast<int>(writeBufferPos - totalSent), 0);
#else
			ssize_t sent = ::send(socket, writeBuffer.data() + totalSent, writeBufferPos - totalSent, 0);
#endif
			
			if (sent <= 0)
			{
#ifdef _WIN32
				int error = WSAGetLastError();
				std::cerr << "[SocketOutputStream] flush: send() failed, WSAGetLastError=" << error << ", sent=" << sent << std::endl;
#else
				std::cerr << "[SocketOutputStream] flush: send() failed, errno=" << errno << ", sent=" << sent << std::endl;
#endif
				throw std::runtime_error("Failed to send data");
			}
			
			totalSent += sent;
		}
		
		writeBufferPos = 0;
	}
}

void SocketOutputStream::close()
{
	// Socket is closed by NetworkManager, not here
}
