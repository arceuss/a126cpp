#include "nbt/ByteArrayTag.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "java/IOUtil.h"

ByteArrayTag::ByteArrayTag()
{

}

ByteArrayTag::ByteArrayTag(std::vector<byte_t> &&data) : data(std::move(data))
{
}

void ByteArrayTag::write(std::ostream &os)
{
	IOUtil::writeInt(os, static_cast<int_t>(data.size()));
	os.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
}

void ByteArrayTag::load(std::istream &is)
{
	// Alpha NBTTagByteArray.java:27-31 performs `new byte[n]` followed by
	// DataInput.readFully. Preserve that order exactly: negative allocation
	// fails before a read, and any short payload throws EOFException.
	const int_t size = IOUtil::readInt(is);
	if (size < 0)
		throw std::runtime_error("NegativeArraySizeException: NBT byte array length " + std::to_string(size));

	data.resize(static_cast<size_t>(size));
	IOUtil::readFully(is, reinterpret_cast<char *>(data.data()), data.size());
}

byte_t ByteArrayTag::getId() const
{
	return TAG_Byte_Array;
}

jstring ByteArrayTag::toString() const
{
	return u"[" + String::fromUTF8(std::to_string(data.size())) + u" bytes]";
}
