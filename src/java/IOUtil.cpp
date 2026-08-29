#include "java/IOUtil.h"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace IOUtil
{

// Java: every DataInputStream primitive read ends in InputStream.read() and
// raises EOFException the moment a byte is missing, so a short stream must
// never surface as data. The message keeps the "EOF" substring the network
// packet reader matches on.
static int_t readUnsignedByteOrThrow(std::istream &is)
{
	std::istream::int_type c = is.get();
	if (c == std::char_traits<char>::eof())
		throw std::runtime_error("EOFException: unexpected end of stream");
	return static_cast<int_t>(static_cast<ubyte_t>(c));
}

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
	const uint_t bits = static_cast<uint_t>(c);
	os.put(static_cast<char>((bits >> 8) & 0xFF));
	os.put(static_cast<char>(bits & 0xFF));
}
void writeShort(std::ostream &os, int_t s)
{
	const uint_t bits = static_cast<uint_t>(s);
	os.put(static_cast<char>((bits >> 8) & 0xFF));
	os.put(static_cast<char>(bits & 0xFF));
}
void writeInt(std::ostream &os, int_t i)
{
	const uint_t bits = static_cast<uint_t>(i);
	os.put(static_cast<char>((bits >> 24) & 0xFF));
	os.put(static_cast<char>((bits >> 16) & 0xFF));
	os.put(static_cast<char>((bits >> 8) & 0xFF));
	os.put(static_cast<char>(bits & 0xFF));
}
void writeLong(std::ostream &os, long_t l)
{
	const ulong_t bits = static_cast<ulong_t>(l);
	os.put(static_cast<char>((bits >> 56) & 0xFF));
	os.put(static_cast<char>((bits >> 48) & 0xFF));
	os.put(static_cast<char>((bits >> 40) & 0xFF));
	os.put(static_cast<char>((bits >> 32) & 0xFF));
	os.put(static_cast<char>((bits >> 24) & 0xFF));
	os.put(static_cast<char>((bits >> 16) & 0xFF));
	os.put(static_cast<char>((bits >> 8) & 0xFF));
	os.put(static_cast<char>(bits & 0xFF));
}
void writeFloat(std::ostream &os, float f)
{
	// DataOutputStream.writeFloat uses Float.floatToIntBits, which
	// canonicalises every NaN payload rather than preserving its raw bits.
	uint_t bits = 0;
	if (std::isnan(f))
		bits = 0x7FC00000U;
	else
		std::memcpy(&bits, &f, sizeof(bits));
	writeInt(os, static_cast<int_t>(bits));
}
void writeDouble(std::ostream &os, double d)
{
	// DataOutputStream.writeDouble uses Double.doubleToLongBits.
	ulong_t bits = 0;
	if (std::isnan(d))
		bits = 0x7FF8000000000000ULL;
	else
		std::memcpy(&bits, &d, sizeof(bits));
	writeLong(os, static_cast<long_t>(bits));
}
void writeUTF(std::ostream &os, const jstring &str)
{
	// Java modified UTF-8 encoding
	// First, calculate the byte length (modified UTF-8 can be longer than standard UTF-8)
	// Java DataOutputStream.writeUTF classifies per UTF-16 code unit, so a
	// non-BMP character is emitted as two 3-byte surrogate sequences (CESU-8).
	int_t byteLength = 0;
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

	// Java: writeUTF refuses anything the unsigned short length cannot express.
	if (byteLength > 65535)
		throw std::runtime_error("UTFDataFormatException: encoded string too long: " + std::to_string(byteLength) + " bytes");

	// Write length as unsigned short (big-endian)
	writeShort(os, byteLength);
	
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

void readFully(std::istream &is, char *dst, size_t len)
{
	// Java: DataInputStream.readFully loops until the buffer is full and throws
	// EOFException otherwise; it never reports a partial fill.
	if (len == 0)
		return;
	is.read(dst, static_cast<std::streamsize>(len));
	if (static_cast<size_t>(is.gcount()) != len)
		throw std::runtime_error("EOFException: unexpected end of stream");
}

bool readBoolean(std::istream &is)
{
	return readUnsignedByteOrThrow(is) != 0;
}

byte_t readByte(std::istream &is)
{
	const ubyte_t bits = static_cast<ubyte_t>(readUnsignedByteOrThrow(is));
	byte_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

char_t readChar(std::istream &is)
{
	const uint_t b1 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	const uint_t b2 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	return static_cast<char_t>((b1 << 8) | b2);
}

short_t readShort(std::istream &is)
{
	const uint_t b1 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	const uint_t b2 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	const ushort_t bits = static_cast<ushort_t>((b1 << 8) | b2);
	short_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

int_t readInt(std::istream &is)
{
	const uint_t b1 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	const uint_t b2 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	const uint_t b3 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	const uint_t b4 = static_cast<uint_t>(readUnsignedByteOrThrow(is));
	const uint_t bits = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
	int_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

long_t readLong(std::istream &is)
{
	ulong_t bits = 0;
	for (int_t i = 0; i < 8; ++i)
		bits = (bits << 8) | static_cast<ulong_t>(readUnsignedByteOrThrow(is));
	long_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

float readFloat(std::istream &is)
{
	// Java: Float.intBitsToFloat(readInt()).
	uint_t fi = static_cast<uint_t>(readInt(is));
	float f;
	std::memcpy(&f, &fi, sizeof(f));
	return f;
}

double readDouble(std::istream &is)
{
	ulong_t di = static_cast<ulong_t>(readLong(is));
	double d;
	std::memcpy(&d, &di, sizeof(d));
	return d;
}

jstring readUTF(std::istream &is)
{
	// Java modified UTF-8 decoding, mirroring DataInputStream.readUTF.
	// The length prefix is an UNSIGNED short, so 0x8000..0xFFFF are legal byte
	// counts and must not be read back as a negative length.
	int_t byteLength = static_cast<int_t>(static_cast<ushort_t>(readShort(is)));

	std::vector<ubyte_t> bytes(static_cast<size_t>(byteLength));
	readFully(is, reinterpret_cast<char *>(bytes.data()), bytes.size());

	// Decode modified UTF-8 to UTF-16 (jstring)
	jstring result;
	result.reserve(static_cast<size_t>(byteLength)); // Reserve space (will be <= byteLength)

	for (int_t i = 0; i < byteLength; )
	{
		ubyte_t b1 = bytes[static_cast<size_t>(i++)];

		if ((b1 & 0x80) == 0)
		{
			// Single byte: 0xxxxxxx. Java's readUTF dispatches on (c >> 4) and
			// treats 0x00 as an ordinary one-byte U+0000, so a bare null byte is
			// accepted here even though writeUTF never emits one.
			result += static_cast<char16_t>(b1);
		}
		else if ((b1 & 0xE0) == 0xC0)
		{
			// Two-byte sequence: 110xxxxx 10xxxxxx
			if (i >= byteLength)
			{
				throw std::runtime_error("UTFDataFormatException: malformed input: partial character at end");
			}
			ubyte_t b2 = bytes[static_cast<size_t>(i++)];
			if ((b2 & 0xC0) != 0x80)
			{
				throw std::runtime_error("UTFDataFormatException: malformed input around byte " + std::to_string(i));
			}

			// 0xC0 0x80 decodes to U+0000, which is how writeUTF encodes it.
			result += static_cast<char16_t>(((b1 & 0x1F) << 6) | (b2 & 0x3F));
		}
		else if ((b1 & 0xF0) == 0xE0)
		{
			// Three-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx. A surrogate code
			// unit arrives as its own 3-byte sequence, so a non-BMP character
			// round-trips as the surrogate pair Java stores in its String.
			if (i + 1 >= byteLength)
			{
				throw std::runtime_error("UTFDataFormatException: malformed input: partial character at end");
			}
			ubyte_t b2 = bytes[static_cast<size_t>(i++)];
			ubyte_t b3 = bytes[static_cast<size_t>(i++)];
			if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
			{
				throw std::runtime_error("UTFDataFormatException: malformed input around byte " + std::to_string(i - 1));
			}

			result += static_cast<char16_t>(((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F));
		}
		else
		{
			// Continuation byte without a lead, or a 4-byte sequence: Java's
			// readUTF rejects both (switch default).
			throw std::runtime_error("UTFDataFormatException: malformed input around byte " + std::to_string(i));
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
