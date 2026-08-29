#include <algorithm>
#include <stdexcept>
#include <vector>

#include "network/Packet130UpdateSign.h"
#include "network/Packet131MapData.h"
#include "network/Packet200Statistic.h"
#include "network/Packet255KickDisconnect.h"
#include "tools/headless/PacketTestUtils.h"

using headless::checkPacketBytes;
using headless::decodePacketData;
using headless::encodeFramedPacket;
using headless::encodePacketData;

namespace
{

template<typename PacketType>
bool decodeThrows(const std::vector<byte_t> &bytes)
{
	try
	{
		(void) decodePacketData<PacketType>(bytes);
	}
	catch (const std::runtime_error &)
	{
		return true;
	}
	return false;
}

void checkFramedId(headless::TestContext &ctx, Packet &packet, int_t expectedId,
	const std::string &message)
{
	const std::vector<byte_t> framed = encodeFramedPacket(packet);
	if (!ctx.check(!framed.empty(), message + " frame is not empty"))
		return;
	ctx.checkEqual(static_cast<long long>(static_cast<ubyte_t>(framed[0])), expectedId,
		message + " framed packet id");
}

}

HEADLESS_TEST(packet, packet130_alpha_wire_and_roundtrip)
{
	const jstring lines[4] = { u"A", u"BC", u"\u732b", u"123456789abcdef" };
	Packet130UpdateSign packet(0x10203040, -2, static_cast<int_t>(0x80000001u), lines);

	// Alpha Packet130UpdateSign.java CFR lines 43-49 / VF lines 40-47:
	// int, short, int, then four independently length-prefixed UTF-16 strings.
	const std::vector<byte_t> expected = {
		16, 32, 48, 64, -1, -2, -128, 0, 0, 1,
		0, 1, 0, 65,
		0, 2, 0, 66, 0, 67,
		0, 1, 115, 43,
		0, 15,
		0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56,
		0, 57, 0, 97, 0, 98, 0, 99, 0, 100, 0, 101, 0, 102
	};
	if (!checkPacketBytes(ctx, encodePacketData(packet), expected, "packet 130 Alpha payload"))
		return;
	ctx.check(packet.isChunkDataPacket, "packet 130 value constructor marks chunk data");
	ctx.checkEqual(packet.getPacketSize(), 19, "packet 130 Alpha character-count size");
	ctx.checkEqual(packet.getPacketId(), 130, "packet 130 id");
	checkFramedId(ctx, packet, 130, "packet 130");

	const std::unique_ptr<Packet130UpdateSign> decoded = decodePacketData<Packet130UpdateSign>(expected);
	ctx.check(decoded->isChunkDataPacket, "packet 130 default constructor marks chunk data");
	ctx.checkEqual(decoded->xPosition, 0x10203040, "packet 130 x");
	ctx.checkEqual(decoded->yPosition, -2, "packet 130 signed short y");
	ctx.checkEqual(decoded->zPosition, static_cast<int_t>(0x80000001u), "packet 130 z");
	for (int_t i = 0; i < 4; ++i)
		ctx.checkEqual(decoded->signLines[i], lines[i], "packet 130 sign line " + std::to_string(i));
	checkPacketBytes(ctx, encodePacketData(*decoded), expected, "packet 130 decode/re-encode");
}

HEADLESS_TEST(packet, packet130_per_line_length_boundary)
{
	const jstring lines[4] = { u"0123456789abcdef", u"", u"", u"" };
	Packet130UpdateSign packet(0, 0, 0, lines);

	// Alpha Packet130UpdateSign.java CFR lines 43-49 writes through writeString with
	// only Short.MAX_VALUE protection; CFR lines 32-39 reads every line with max 15.
	const std::vector<byte_t> expected = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 16,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55,
		0, 56, 0, 57, 0, 97, 0, 98, 0, 99, 0, 100, 0, 101, 0, 102,
		0, 0, 0, 0, 0, 0
	};
	checkPacketBytes(ctx, encodePacketData(packet), expected,
		"packet 130 writer permits a 16-character line");
	ctx.checkEqual(packet.getPacketSize(), 16, "packet 130 16-character Alpha size");
	ctx.check(decodeThrows<Packet130UpdateSign>(expected),
		"packet 130 reader rejects an over-15-character line");
}

HEADLESS_TEST(packet, packet131_zero_and_nonzero_arrays)
{
	Packet131MapData zero;
	zero.field_28055_a = 0x1234;
	zero.field_28054_b = -2;

	// Alpha Packet131MapData.java CFR lines 31-35 / VF lines 26-30:
	// item id short, damage short, byte array length, then the raw array.
	const std::vector<byte_t> zeroExpected = { 18, 52, -1, -2, 0 };
	checkPacketBytes(ctx, encodePacketData(zero), zeroExpected, "packet 131 zero-length payload");
	ctx.check(zero.isChunkDataPacket, "packet 131 constructor marks chunk data");
	ctx.checkEqual(zero.getPacketSize(), 4, "packet 131 zero-length Alpha size");

	const std::unique_ptr<Packet131MapData> zeroDecoded = decodePacketData<Packet131MapData>(zeroExpected);
	ctx.checkEqual(zeroDecoded->field_28055_a, 0x1234, "packet 131 zero item id");
	ctx.checkEqual(zeroDecoded->field_28054_b, -2, "packet 131 zero item damage");
	ctx.check(zeroDecoded->field_28056_c.empty(), "packet 131 zero array decoded empty");
	checkPacketBytes(ctx, encodePacketData(*zeroDecoded), zeroExpected,
		"packet 131 zero decode/re-encode");

	Packet131MapData nonzero;
	nonzero.field_28055_a = -32768;
	nonzero.field_28054_b = 32767;
	nonzero.field_28056_c = { -128, 1, -1 };
	const std::vector<byte_t> nonzeroExpected = { -128, 0, 127, -1, 3, -128, 1, -1 };
	checkPacketBytes(ctx, encodePacketData(nonzero), nonzeroExpected,
		"packet 131 nonzero signed-byte payload");
	ctx.checkEqual(nonzero.getPacketSize(), 7, "packet 131 nonzero Alpha size");
	ctx.checkEqual(nonzero.getPacketId(), 131, "packet 131 id");
	checkFramedId(ctx, nonzero, 131, "packet 131");

	const std::unique_ptr<Packet131MapData> nonzeroDecoded =
		decodePacketData<Packet131MapData>(nonzeroExpected);
	ctx.checkEqual(nonzeroDecoded->field_28055_a, -32768, "packet 131 nonzero item id");
	ctx.checkEqual(nonzeroDecoded->field_28054_b, 32767, "packet 131 nonzero item damage");
	checkPacketBytes(ctx, nonzeroDecoded->field_28056_c, { -128, 1, -1 },
		"packet 131 signed data bytes");
	checkPacketBytes(ctx, encodePacketData(*nonzeroDecoded), nonzeroExpected,
		"packet 131 nonzero decode/re-encode");
}

HEADLESS_TEST(packet, packet131_unsigned_length_byte)
{
	// Alpha Packet131MapData.java CFR lines 23-27 / VF lines 18-22 masks
	// readByte() with 0xFF, so wire FF is a 255-byte array rather than -1.
	const std::vector<byte_t> expected = {
		1, 2, 3, 4, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};
	const std::unique_ptr<Packet131MapData> decoded = decodePacketData<Packet131MapData>(expected);
	ctx.checkEqual(static_cast<long long>(decoded->field_28056_c.size()), 255,
		"packet 131 unsigned FF length");
	ctx.check(std::all_of(decoded->field_28056_c.begin(), decoded->field_28056_c.end(),
		[](byte_t value) { return value == -1; }), "packet 131 preserves signed FF data bytes");
	ctx.checkEqual(decoded->getPacketSize(), 259, "packet 131 255-byte Alpha size");
	checkPacketBytes(ctx, encodePacketData(*decoded), expected,
		"packet 131 unsigned-length decode/re-encode");
}

HEADLESS_TEST(packet, packet200_alpha_wire_and_signed_count)
{
	Packet200Statistic packet;
	packet.field_27052_a = 0x12345678;
	packet.field_27051_b = -2;

	// Alpha Packet200Statistic.java CFR lines 23-31 / VF lines 21-29:
	// statistic id is an int and count is a signed readByte/writeByte value.
	const std::vector<byte_t> expected = { 18, 52, 86, 120, -2 };
	checkPacketBytes(ctx, encodePacketData(packet), expected, "packet 200 Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 6, "packet 200 deliberately reports Alpha size 6");
	ctx.checkEqual(packet.getPacketId(), 200, "packet 200 id");
	checkFramedId(ctx, packet, 200, "packet 200");

	const std::unique_ptr<Packet200Statistic> decoded = decodePacketData<Packet200Statistic>(expected);
	ctx.checkEqual(decoded->field_27052_a, 0x12345678, "packet 200 statistic id");
	ctx.checkEqual(decoded->field_27051_b, -2, "packet 200 signed count");
	checkPacketBytes(ctx, encodePacketData(*decoded), expected, "packet 200 decode/re-encode");
}

HEADLESS_TEST(packet, packet255_string_boundaries_and_roundtrip)
{
	const jstring maxReason =
		u"01234567890123456789012345678901234567890123456789"
		u"01234567890123456789012345678901234567890123456789";
	Packet255KickDisconnect packet(maxReason);

	// Alpha Packet255KickDisconnect.java CFR lines 29-30 / VF lines 25-26 writes
	// one UTF-16 string; CFR lines 24-25 / VF lines 20-21 reads it with max 100.
	const std::vector<byte_t> expected = {
		0, 100,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57,
		0, 48, 0, 49, 0, 50, 0, 51, 0, 52, 0, 53, 0, 54, 0, 55, 0, 56, 0, 57
	};
	checkPacketBytes(ctx, encodePacketData(packet), expected, "packet 255 100-character payload");
	ctx.checkEqual(packet.getPacketSize(), 100, "packet 255 100-character Alpha size");
	ctx.checkEqual(packet.getPacketId(), 255, "packet 255 id");
	checkFramedId(ctx, packet, 255, "packet 255");

	const std::unique_ptr<Packet255KickDisconnect> decoded =
		decodePacketData<Packet255KickDisconnect>(expected);
	ctx.checkEqual(decoded->reason, maxReason, "packet 255 100-character reason");
	checkPacketBytes(ctx, encodePacketData(*decoded), expected, "packet 255 decode/re-encode");

	Packet255KickDisconnect overlong(jstring(101, u'A'));
	const std::vector<byte_t> overlongBytes = encodePacketData(overlong);
	ctx.checkEqual(static_cast<long long>(overlongBytes.size()), 204,
		"packet 255 writer permits 101 characters like Alpha writeString");
	ctx.checkEqual(static_cast<long long>(static_cast<ubyte_t>(overlongBytes[0])), 0,
		"packet 255 overlong high length byte");
	ctx.checkEqual(static_cast<long long>(static_cast<ubyte_t>(overlongBytes[1])), 101,
		"packet 255 overlong low length byte");
	ctx.check(decodeThrows<Packet255KickDisconnect>({ 0, 101 }),
		"packet 255 reader rejects a 101-character reason before reading characters");
	ctx.checkEqual(overlong.getPacketSize(), 101, "packet 255 101-character Alpha size");
}
