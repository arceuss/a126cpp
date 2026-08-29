// Layers 1 and 3: Java primitive streams and Alpha NBT semantics.
//
// Alpha authority: NBTBase.java, NBTTagCompound.java, NBTTagList.java,
// NBTTagByteArray.java and java.io.DataInputStream/DataOutputStream.

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "java/IOUtil.h"
#include "nbt/ByteArrayTag.h"
#include "nbt/ByteTag.h"
#include "nbt/CompoundTag.h"
#include "nbt/DoubleTag.h"
#include "nbt/FloatTag.h"
#include "nbt/IntTag.h"
#include "nbt/ListTag.h"
#include "nbt/LongTag.h"
#include "nbt/NbtIo.h"
#include "nbt/ShortTag.h"
#include "nbt/StringTag.h"
#include "nbt/Tag.h"
#include "tools/headless/TestFramework.h"

template<typename Function>
static bool throwsException(Function &&function)
{
	try
	{
		function();
	}
	catch (const std::exception &)
	{
		return true;
	}
	return false;
}

static std::unique_ptr<CompoundTag> roundTrip(CompoundTag &root)
{
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	NbtIo::write(root, stream);
	stream.seekg(0);
	return std::unique_ptr<CompoundTag>(NbtIo::read(stream));
}

static uint32_t floatBits(float value)
{
	uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static uint64_t doubleBits(double value)
{
	uint64_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

HEADLESS_TEST(nbt, binary_primitives_round_trip_exactly)
{
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeBoolean(stream, true);
	IOUtil::writeByte(stream, -128);
	IOUtil::writeChar(stream, 0xFFFF);
	IOUtil::writeShort(stream, -32768);
	IOUtil::writeInt(stream, std::numeric_limits<int_t>::min());
	IOUtil::writeLong(stream, std::numeric_limits<long_t>::min());
	IOUtil::writeFloat(stream, -0.0f);
	IOUtil::writeDouble(stream, -0.0);
	stream.seekg(0);

	ctx.check(IOUtil::readBoolean(stream), "boolean round trip");
	ctx.checkEqual(IOUtil::readByte(stream), -128, "byte round trip");
	ctx.checkEqual(IOUtil::readChar(stream), 0xFFFF, "unsigned char round trip");
	ctx.checkEqual(IOUtil::readShort(stream), -32768, "short round trip");
	ctx.checkEqual(IOUtil::readInt(stream), std::numeric_limits<int_t>::min(), "int round trip");
	ctx.checkEqual(IOUtil::readLong(stream), std::numeric_limits<long_t>::min(), "long round trip");
	ctx.checkEqualBits(IOUtil::readFloat(stream), -0.0f, "float raw bits");
	ctx.checkEqualBits(IOUtil::readDouble(stream), -0.0, "double raw bits");
}

HEADLESS_TEST(nbt, primitive_tags_round_trip_semantically)
{
	CompoundTag root;
	root.setName(u"root");
	root.putByte(u"byte", static_cast<byte_t>(-127));
	root.putShort(u"short", static_cast<short_t>(-32000));
	root.putInt(u"int", -2000000000);
	root.putLong(u"long", -9000000000000000000LL);
	root.putFloat(u"float", -0.0f);
	root.putDouble(u"double", -0.0);
	root.putString(u"string", u"Alpha");
	root.putByteArray(u"bytes", std::vector<byte_t>{ -128, -1, 0, 1, 127 });

	std::shared_ptr<ListTag> ints = std::make_shared<ListTag>();
	ints->add(std::make_shared<IntTag>(1));
	ints->add(std::make_shared<IntTag>(-2));
	ints->add(std::make_shared<IntTag>(3));
	root.put(u"list", ints);

	std::unique_ptr<CompoundTag> loaded = roundTrip(root);
	ctx.checkEqual(loaded->getName(), jstring(u"root"), "root name");
	ctx.checkEqual(loaded->getByte(u"byte"), -127, "byte tag");
	ctx.checkEqual(loaded->getShort(u"short"), -32000, "short tag");
	ctx.checkEqual(loaded->getInt(u"int"), -2000000000, "int tag");
	ctx.checkEqual(loaded->getLong(u"long"), -9000000000000000000LL, "long tag");
	ctx.checkEqualBits(loaded->getFloat(u"float"), -0.0f, "float tag raw bits");
	ctx.checkEqualBits(loaded->getDouble(u"double"), -0.0, "double tag raw bits");
	ctx.checkEqual(loaded->getString(u"string"), jstring(u"Alpha"), "string tag");

	const std::vector<byte_t> &bytes = loaded->getByteArray(u"bytes");
	ctx.check(bytes == std::vector<byte_t>({ -128, -1, 0, 1, 127 }), "byte array contents");

	std::shared_ptr<ListTag> loadedInts = loaded->getList(u"list");
	if (!ctx.check(loadedInts != nullptr, "integer list must load"))
		return;
	ctx.checkEqual(loadedInts->size(), 3, "integer list length");
	ctx.checkEqual(std::static_pointer_cast<IntTag>(loadedInts->get(1))->data, -2,
		"integer list middle value");
}

HEADLESS_TEST(nbt, compound_put_replaces_existing_key)
{
	CompoundTag tag;
	tag.putInt(u"value", 1);
	tag.putInt(u"value", 2);
	ctx.checkEqual(tag.getInt(u"value"), 2, "Map.put replacement semantics");
}

HEADLESS_TEST(nbt, duplicate_stream_key_uses_last_value)
{
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeByte(stream, Tag::TAG_Int);
	IOUtil::writeUTF(stream, u"value");
	IOUtil::writeInt(stream, 1);
	IOUtil::writeByte(stream, Tag::TAG_Int);
	IOUtil::writeUTF(stream, u"value");
	IOUtil::writeInt(stream, 2);
	IOUtil::writeByte(stream, Tag::TAG_End);
	stream.seekg(0);

	CompoundTag tag;
	tag.load(stream);
	ctx.checkEqual(tag.getInt(u"value"), 2, "last duplicate stream key must win");
}

HEADLESS_TEST(nbt, nested_names_and_lists_round_trip)
{
	CompoundTag root;
	root.setName(u"root");

	std::unique_ptr<CompoundTag> child = std::make_unique<CompoundTag>();
	child->putString(u"marker", u"nested");
	root.putCompound(u"child", std::move(child));

	std::shared_ptr<ListTag> compounds = std::make_shared<ListTag>();
	std::shared_ptr<CompoundTag> first = std::make_shared<CompoundTag>();
	first->putInt(u"index", 1);
	compounds->add(first);
	std::shared_ptr<CompoundTag> second = std::make_shared<CompoundTag>();
	second->putInt(u"index", 2);
	compounds->add(second);
	root.put(u"compounds", compounds);
	root.put(u"empty", std::make_shared<ListTag>());

	ctx.checkEqual(root.getCompound(u"child")->getName(), jstring(u"child"),
		"putCompound must set the child name");
	ctx.checkEqual(root.get(u"compounds")->getName(), jstring(u"compounds"),
		"list tag name before write");

	std::unique_ptr<CompoundTag> loaded = roundTrip(root);
	std::shared_ptr<CompoundTag> loadedChild = loaded->getCompound(u"child");
	ctx.checkEqual(loadedChild->getName(), jstring(u"child"), "nested compound name");
	ctx.checkEqual(loadedChild->get(u"marker")->getName(), jstring(u"marker"), "nested value name");
	ctx.checkEqual(loadedChild->getString(u"marker"), jstring(u"nested"), "nested value");

	std::shared_ptr<ListTag> loadedCompounds = loaded->getList(u"compounds");
	ctx.checkEqual(loadedCompounds->size(), 2, "compound list length");
	ctx.checkEqual(std::static_pointer_cast<CompoundTag>(loadedCompounds->get(0))->getInt(u"index"),
		1, "first compound list value");
	ctx.checkEqual(std::static_pointer_cast<CompoundTag>(loadedCompounds->get(1))->getInt(u"index"),
		2, "second compound list value");
	ctx.checkEqual(loaded->getList(u"empty")->size(), 0, "empty list round trip");
}

HEADLESS_TEST(nbt, modified_utf_matches_java_encoding)
{
	const jstring text({ u'A', static_cast<char16_t>(0), static_cast<char16_t>(0xD83D),
		static_cast<char16_t>(0xDE03), u'Z' });
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeUTF(stream, text);
	const std::string encoded = stream.str();
	const unsigned char expected[] = {
		0x00, 0x0A, 0x41, 0xC0, 0x80, 0xED, 0xA0, 0xBD, 0xED, 0xB8, 0x83, 0x5A
	};
	ctx.check(encoded.size() == sizeof(expected), "modified UTF encoded length");
	if (encoded.size() == sizeof(expected))
	{
		ctx.check(std::memcmp(encoded.data(), expected, sizeof(expected)) == 0,
			"modified UTF bytes must match DataOutputStream.writeUTF");
	}

	stream.seekg(0);
	ctx.checkEqual(IOUtil::readUTF(stream), text, "modified UTF round trip");
}

HEADLESS_TEST(nbt, floating_nan_payloads_are_canonicalized)
{
	uint32_t floatPayload = 0x7FA12345U;
	float nonCanonicalFloat = 0.0f;
	std::memcpy(&nonCanonicalFloat, &floatPayload, sizeof(nonCanonicalFloat));
	std::stringstream floatStream(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeFloat(floatStream, nonCanonicalFloat);
	floatStream.seekg(0);
	ctx.checkEqual(floatBits(IOUtil::readFloat(floatStream)), 0x7FC00000U,
		"Float.floatToIntBits canonical NaN");

	uint64_t doublePayload = 0x7FF0123456789ABCULL;
	double nonCanonicalDouble = 0.0;
	std::memcpy(&nonCanonicalDouble, &doublePayload, sizeof(nonCanonicalDouble));
	std::stringstream doubleStream(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeDouble(doubleStream, nonCanonicalDouble);
	doubleStream.seekg(0);
	ctx.checkEqual(static_cast<long long>(doubleBits(IOUtil::readDouble(doubleStream))),
		static_cast<long long>(0x7FF8000000000000ULL), "Double.doubleToLongBits canonical NaN");
}

HEADLESS_TEST(nbt, negative_byte_array_length_is_rejected)
{
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeInt(stream, -1);
	stream.seekg(0);
	ByteArrayTag tag;
	ctx.check(throwsException([&]() { tag.load(stream); }), "negative byte-array length must throw");
}

HEADLESS_TEST(nbt, truncated_byte_array_is_rejected)
{
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeInt(stream, 4);
	IOUtil::writeByte(stream, 1);
	IOUtil::writeByte(stream, 2);
	stream.seekg(0);
	ByteArrayTag tag;
	ctx.check(throwsException([&]() { tag.load(stream); }), "truncated byte array must throw");
}

HEADLESS_TEST(nbt, truncated_primitive_is_rejected)
{
	std::stringstream stream(std::string("\x01\x02\x03", 3), std::ios::in | std::ios::binary);
	ctx.check(throwsException([&]() { (void)IOUtil::readInt(stream); }), "truncated int must throw");
}

HEADLESS_TEST(nbt, truncated_modified_utf_is_rejected)
{
	std::stringstream stream(std::string("\x00\x02\xC0", 3), std::ios::in | std::ios::binary);
	ctx.check(throwsException([&]() { (void)IOUtil::readUTF(stream); }), "truncated UTF must throw");
}

HEADLESS_TEST(nbt, malformed_tag_ids_are_rejected)
{
	std::stringstream named(std::string("\x63", 1), std::ios::in | std::ios::binary);
	ctx.check(throwsException([&]() {
		std::unique_ptr<Tag> tag(Tag::readNamedTag(named));
	}), "unknown named-tag id must throw");

	std::stringstream list(std::ios::in | std::ios::out | std::ios::binary);
	IOUtil::writeByte(list, 99);
	IOUtil::writeInt(list, 1);
	list.seekg(0);
	ListTag listTag;
	ctx.check(throwsException([&]() { listTag.load(list); }), "unknown positive list element id must throw");
}
