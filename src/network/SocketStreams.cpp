#include "network/SocketStreams.h"
#include "java/String.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

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

SocketInputStream::~SocketInputStream()
{
	close();
}

int SocketInputStream::read()
{
	// Match Java InputStream.read():
	// - returns 0..255 for a byte
	// - returns -1 on EOF
	// - does NOT throw on EOF (DataInputStream.readFully does)
	// - throws SocketTimeoutException on timeout (which should be caught and handled gracefully)
	byte_t b;
	// Read single byte directly
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
	size_t totalRead = 0;
	while (totalRead < len)
	{
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
				throw std::runtime_error("End of stream reached");
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
				throw std::runtime_error("End of stream reached");
			}
			throw std::runtime_error("Socket read failed");
		}
#endif
		
		if (bytesRead == 0)
		{
			throw std::runtime_error("End of stream reached");
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
	// Java DataInputStream.readShort(): ((ch1 << 8) + (ch2 << 0))
	// Where ch1, ch2 are int values 0-255 from in.read()
	// We use unsigned cast to match Java's unsigned byte treatment
	return static_cast<short_t>((static_cast<uint_t>(bytes[0] & 0xFF) << 8) + (static_cast<uint_t>(bytes[1] & 0xFF)));
}

int_t SocketInputStream::readInt()
{
	byte_t bytes[4];
	readFully(bytes, 4);
	// Java DataInputStream.readInt(): ((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0))
	// Where ch1-ch4 are int values 0-255 from in.read()
	return static_cast<int_t>(
		(static_cast<uint_t>(bytes[0] & 0xFF) << 24) +
		(static_cast<uint_t>(bytes[1] & 0xFF) << 16) +
		(static_cast<uint_t>(bytes[2] & 0xFF) << 8) +
		(static_cast<uint_t>(bytes[3] & 0xFF))
	);
}

long_t SocketInputStream::readLong()
{
	byte_t bytes[8];
	readFully(bytes, 8);
	// Java DataInputStream.readLong(): Uses readFully(readBuffer, 0, 8) then:
	// (((long)readBuffer[0] << 56) + ((long)(readBuffer[1] & 255) << 48) + ...)
	// Note: JDK8 does NOT mask readBuffer[0], only readBuffer[1-7]
	return static_cast<long_t>(
		(static_cast<ulong_t>(bytes[0]) << 56) +
		(static_cast<ulong_t>(bytes[1] & 0xFF) << 48) +
		(static_cast<ulong_t>(bytes[2] & 0xFF) << 40) +
		(static_cast<ulong_t>(bytes[3] & 0xFF) << 32) +
		(static_cast<ulong_t>(bytes[4] & 0xFF) << 24) +
		(static_cast<ulong_t>(bytes[5] & 0xFF) << 16) +
		(static_cast<ulong_t>(bytes[6] & 0xFF) << 8) +
		(static_cast<ulong_t>(bytes[7] & 0xFF))
	);
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
	// Java DataOutputStream uses big-endian (network byte order)
	byte_t bytes[2];
	bytes[0] = static_cast<byte_t>((s >> 8) & 0xFF);
	bytes[1] = static_cast<byte_t>(s & 0xFF);
	write(bytes, 2);
}

void SocketOutputStream::writeInt(int_t i)
{
	// Java DataOutputStream uses big-endian (network byte order)
	byte_t bytes[4];
	bytes[0] = static_cast<byte_t>((i >> 24) & 0xFF);
	bytes[1] = static_cast<byte_t>((i >> 16) & 0xFF);
	bytes[2] = static_cast<byte_t>((i >> 8) & 0xFF);
	bytes[3] = static_cast<byte_t>(i & 0xFF);
	write(bytes, 4);
}

void SocketOutputStream::writeLong(long_t l)
{
	// Java DataOutputStream uses big-endian (network byte order)
	byte_t bytes[8];
	bytes[0] = static_cast<byte_t>((l >> 56) & 0xFF);
	bytes[1] = static_cast<byte_t>((l >> 48) & 0xFF);
	bytes[2] = static_cast<byte_t>((l >> 40) & 0xFF);
	bytes[3] = static_cast<byte_t>((l >> 32) & 0xFF);
	bytes[4] = static_cast<byte_t>((l >> 24) & 0xFF);
	bytes[5] = static_cast<byte_t>((l >> 16) & 0xFF);
	bytes[6] = static_cast<byte_t>((l >> 8) & 0xFF);
	bytes[7] = static_cast<byte_t>(l & 0xFF);
	write(bytes, 8);
}

void SocketOutputStream::writeFloat(float f)
{
	// Java Float.floatToIntBits: bit-exact conversion (avoid strict-aliasing UB)
	int_t bits;
	static_assert(sizeof(float) == sizeof(int_t), "float must be 32-bit");
	std::memcpy(&bits, &f, sizeof(int_t));
	writeInt(bits);
}

void SocketOutputStream::writeDouble(double d)
{
	// Java Double.doubleToLongBits: bit-exact conversion (avoid strict-aliasing UB)
	long_t bits;
	static_assert(sizeof(double) == sizeof(long_t), "double must be 64-bit");
	std::memcpy(&bits, &d, sizeof(long_t));
	writeLong(bits);
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
