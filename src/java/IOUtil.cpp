#include "java/IOUtil.h"

namespace IOUtil
{

void writeBoolean(std::ostream &os, bool b)
{
	os.put(b ? 1 : 0);
}
void writeByte(std::ostream &os, int_t b)
{
	os.put(b);
}
void writeChar(std::ostream &os, int_t c)
{
	os.put((c >> 8) & 0xFF);
	os.put(c & 0xFF);
}
void writeShort(std::ostream &os, int_t s)
{
	os.put((s >> 8) & 0xFF);
	os.put(s & 0xFF);
}
void writeInt(std::ostream &os, int_t i)
{
	os.put((i >> 24) & 0xFF);
	os.put((i >> 16) & 0xFF);
	os.put((i >> 8) & 0xFF);
	os.put(i & 0xFF);
}
void writeLong(std::ostream &os, long_t l)
{
	os.put((l >> 56) & 0xFF);
	os.put((l >> 48) & 0xFF);
	os.put((l >> 40) & 0xFF);
	os.put((l >> 32) & 0xFF);
	os.put((l >> 24) & 0xFF);
	os.put((l >> 16) & 0xFF);
	os.put((l >> 8) & 0xFF);
	os.put(l & 0xFF);
}
void writeFloat(std::ostream &os, float f)
{
	int_t fi = *(reinterpret_cast<int_t*>(&f));
	writeInt(os, fi);
}
void writeDouble(std::ostream &os, double d)
{
	long_t di = *(reinterpret_cast<long_t *>(&d));
	writeLong(os, di);
}
void writeUTF(std::ostream &os, const jstring &str)
{
	// Java modified UTF-8 encoding
	// First, calculate the byte length (modified UTF-8 can be longer than standard UTF-8)
	int byteLength = 0;
	for (char16_t c : str)
	{
		if (c == 0)
		{
			// Null character: encoded as 2 bytes (0xC0, 0x80) in modified UTF-8
			byteLength += 2;
		}
		else if (c < 0x80)
		{
			// ASCII: 1 byte
			byteLength += 1;
		}
		else if (c < 0x800)
		{
			// 2-byte sequence
			byteLength += 2;
		}
		else
		{
			// 3-byte sequence (modified UTF-8 only supports up to 3 bytes per char)
			byteLength += 3;
		}
	}
	
	// Write length as unsigned short (big-endian)
	writeShort(os, static_cast<short_t>(byteLength));
	
	// Write the modified UTF-8 encoded bytes
	for (char16_t c : str)
	{
		if (c == 0)
		{
			// Null character: encode as 0xC0 0x80
			os.put(0xC0);
			os.put(0x80);
		}
		else if (c < 0x80)
		{
			// ASCII: single byte
			os.put(static_cast<char>(c));
		}
		else if (c < 0x800)
		{
			// 2-byte sequence: 110xxxxx 10xxxxxx
			os.put(static_cast<char>(0xC0 | (c >> 6)));
			os.put(static_cast<char>(0x80 | (c & 0x3F)));
		}
		else
		{
			// 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
			os.put(static_cast<char>(0xE0 | (c >> 12)));
			os.put(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
			os.put(static_cast<char>(0x80 | (c & 0x3F)));
		}
	}
}

bool readBoolean(std::istream &is)
{
	return is.get() != 0;
}

byte_t readByte(std::istream &is)
{
	return is.get();
}

char_t readChar(std::istream &is)
{
	int b1 = is.get() & 0xFF;
	int b2 = is.get() & 0xFF;
	return (b1 << 8) | b2;
}

short_t readShort(std::istream &is)
{
	int b1 = is.get() & 0xFF;
	int b2 = is.get() & 0xFF;
	return (b1 << 8) | b2;
}

int_t readInt(std::istream &is)
{
	int b1 = is.get() & 0xFF;
	int b2 = is.get() & 0xFF;
	int b3 = is.get() & 0xFF;
	int b4 = is.get() & 0xFF;
	return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}

long_t readLong(std::istream &is)
{
	long_t b1 = (long_t)(is.get() & 0xFF);
	long_t b2 = (long_t)(is.get() & 0xFF);
	long_t b3 = (long_t)(is.get() & 0xFF);
	long_t b4 = (long_t)(is.get() & 0xFF);
	long_t b5 = (long_t)(is.get() & 0xFF);
	long_t b6 = (long_t)(is.get() & 0xFF);
	long_t b7 = (long_t)(is.get() & 0xFF);
	long_t b8 = (long_t)(is.get() & 0xFF);
	return (b1 << 56) | (b2 << 48) | (b3 << 40) | (b4 << 32) | (b5 << 24) | (b6 << 16) | (b7 << 8) | b8;
}

float readFloat(std::istream &is)
{
	int_t fi = readInt(is);
	return *(reinterpret_cast<float *>(&fi));
}

double readDouble(std::istream &is)
{
	long_t di = readLong(is);
	return *(reinterpret_cast<double *>(&di));
}

jstring readUTF(std::istream &is)
{
	// Java modified UTF-8 decoding
	// Read length (in bytes, not characters)
	short_t byteLength = readShort(is);
	if (byteLength < 0)
	{
		throw std::runtime_error("Invalid UTF string length: " + std::to_string(byteLength));
	}
	
	// Read all bytes
	std::vector<byte_t> bytes(byteLength);
	for (int i = 0; i < byteLength; ++i)
	{
		int b = is.get();
		if (b == -1)
		{
			throw std::runtime_error("Unexpected EOF while reading UTF string");
		}
		bytes[i] = static_cast<byte_t>(b);
	}
	
	// Decode modified UTF-8 to UTF-16 (jstring)
	jstring result;
	result.reserve(byteLength); // Reserve space (will be <= byteLength)
	
	for (int i = 0; i < byteLength; )
	{
		byte_t b1 = bytes[i++];
		
		if ((b1 & 0x80) == 0)
		{
			// Single byte: 0xxxxxxx (ASCII)
			// But if it's 0x00, it's invalid (null should be 0xC0 0x80)
			if (b1 == 0)
			{
				throw std::runtime_error("Invalid modified UTF-8: null byte found");
			}
			result += static_cast<char16_t>(b1);
		}
		else if ((b1 & 0xE0) == 0xC0)
		{
			// Two-byte sequence: 110xxxxx 10xxxxxx
			if (i >= byteLength)
			{
				throw std::runtime_error("Invalid modified UTF-8: incomplete 2-byte sequence");
			}
			byte_t b2 = bytes[i++];
			if ((b2 & 0xC0) != 0x80)
			{
				throw std::runtime_error("Invalid modified UTF-8: invalid continuation byte");
			}
			
			char16_t c = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
			
			// Check for null character encoding (0xC0 0x80)
			if (c == 0)
			{
				result += static_cast<char16_t>(0);
			}
			else
			{
				result += c;
			}
		}
		else if ((b1 & 0xF0) == 0xE0)
		{
			// Three-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
			if (i + 1 >= byteLength)
			{
				throw std::runtime_error("Invalid modified UTF-8: incomplete 3-byte sequence");
			}
			byte_t b2 = bytes[i++];
			byte_t b3 = bytes[i++];
			if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
			{
				throw std::runtime_error("Invalid modified UTF-8: invalid continuation bytes");
			}
			
			char16_t c = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
			result += c;
		}
		else
		{
			// Invalid: continuation byte without lead, or 4-byte sequence (not supported)
			throw std::runtime_error("Invalid modified UTF-8: invalid lead byte 0x" + 
			                         std::to_string(static_cast<int>(b1)));
		}
	}
	
	return result;
}

std::vector<char> readAllBytes(std::istream &is)
{
	is.seekg(0, std::ios::end);
	std::streampos size = is.tellg();
	is.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	is.read(buffer.data(), size);
	return buffer;
}

}
