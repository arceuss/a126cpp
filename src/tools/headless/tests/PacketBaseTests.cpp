// Alpha Packet.java base framing and DataInput/DataOutput primitive semantics.

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include "network/Packet.h"
#include "network/Packet0KeepAlive.h"
#include "network/SocketStreams.h"
#include "tools/headless/PacketTestUtils.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"

template<typename Function>
static bool packetThrows(Function &&function)
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

HEADLESS_TEST(packet, memory_stream_primitives_match_data_streams)
{
	std::vector<byte_t> bytes;
	SocketOutputStream output(bytes);
	output.writeByte(static_cast<byte_t>(-128));
	output.writeShort(static_cast<short_t>(-32768));
	output.writeInt((std::numeric_limits<int_t>::min)());
	output.writeLong((std::numeric_limits<long_t>::min)());
	output.writeFloat(-0.0f);
	output.writeDouble(-0.0);
	output.writeBoolean(true);
	output.flush();

	const std::vector<byte_t> expected = {
		static_cast<byte_t>(0x80),
		static_cast<byte_t>(0x80), 0x00,
		static_cast<byte_t>(0x80), 0x00, 0x00, 0x00,
		static_cast<byte_t>(0x80), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		static_cast<byte_t>(0x80), 0x00, 0x00, 0x00,
		static_cast<byte_t>(0x80), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01
	};
	headless::checkPacketBytes(ctx, bytes, expected, "DataOutputStream primitive bytes");

	SocketInputStream input(bytes);
	ctx.checkEqual(input.readByte(), -128, "packet byte");
	ctx.checkEqual(input.readShort(), -32768, "packet short");
	ctx.checkEqual(input.readInt(), (std::numeric_limits<int_t>::min)(), "packet int");
	ctx.checkEqual(input.readLong(), (std::numeric_limits<long_t>::min)(), "packet long");
	ctx.checkEqualBits(input.readFloat(), -0.0f, "packet float");
	ctx.checkEqualBits(input.readDouble(), -0.0, "packet double");
	ctx.check(input.readBoolean(), "packet boolean");
	ctx.checkEqual(input.read(), -1, "memory stream EOF");
}

HEADLESS_TEST(packet, data_stream_nan_bits_are_canonical)
{
	uint32_t floatPayload = 0x7FA12345U;
	float nonCanonicalFloat = 0.0f;
	std::memcpy(&nonCanonicalFloat, &floatPayload, sizeof(nonCanonicalFloat));
	uint64_t doublePayload = 0x7FF0123456789ABCULL;
	double nonCanonicalDouble = 0.0;
	std::memcpy(&nonCanonicalDouble, &doublePayload, sizeof(nonCanonicalDouble));

	std::vector<byte_t> bytes;
	SocketOutputStream output(bytes);
	output.writeFloat(nonCanonicalFloat);
	output.writeDouble(nonCanonicalDouble);
	output.flush();

	const std::vector<byte_t> expected = {
		0x7F, static_cast<byte_t>(0xC0), 0x00, 0x00,
		0x7F, static_cast<byte_t>(0xF8), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	headless::checkPacketBytes(ctx, bytes, expected,
		"Float.floatToIntBits/Double.doubleToLongBits canonical NaN bytes");
}

HEADLESS_TEST(packet, utf16_string_wire_format_matches_alpha)
{
	const jstring text({ u'A', static_cast<char16_t>(0), static_cast<char16_t>(0xD83D),
		static_cast<char16_t>(0xDE03), u'Z' });
	std::vector<byte_t> bytes;
	SocketOutputStream output(bytes);
	Packet::writeString(text, output);
	output.flush();

	// Packet.java:158-164: signed-short UTF-16 code-unit count, then writeChars.
	const std::vector<byte_t> expected = {
		0x00, 0x05, 0x00, 0x41, 0x00, 0x00,
		static_cast<byte_t>(0xD8), 0x3D,
		static_cast<byte_t>(0xDE), 0x03,
		0x00, 0x5A
	};
	if (!headless::checkPacketBytes(ctx, bytes, expected, "Packet.writeString bytes"))
		return;

	SocketInputStream input(bytes);
	ctx.checkEqual(Packet::readString(input, 5), text, "Packet.readString round trip");
	ctx.checkEqual(input.read(), -1, "string decoder consumed all bytes");
}

HEADLESS_TEST(packet, string_length_validation_matches_alpha)
{
	const std::vector<byte_t> negativeLength{ static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF) };
	SocketInputStream negativeInput(negativeLength);
	ctx.check(packetThrows([&]() { (void)Packet::readString(negativeInput, 32767); }),
		"negative signed-short string length must throw");

	const std::vector<byte_t> excessiveLength{ 0x00, 0x02 };
	SocketInputStream excessiveInput(excessiveLength);
	ctx.check(packetThrows([&]() { (void)Packet::readString(excessiveInput, 1); }),
		"string length above packet maximum must throw");

	std::vector<byte_t> outputBytes;
	SocketOutputStream output(outputBytes);
	const jstring tooLarge(32768, u'x');
	ctx.check(packetThrows([&]() { Packet::writeString(tooLarge, output); }),
		"writeString above Short.MAX_VALUE must throw");
}

HEADLESS_TEST(packet, framing_writes_id_before_payload)
{
	headless::initGameRegistries();
	Packet0KeepAlive packet;
	const std::vector<byte_t> bytes = headless::encodeFramedPacket(packet);
	headless::checkPacketBytes(ctx, bytes, { 0x00 }, "Packet.writePacket framing");

	SocketInputStream input(bytes);
	std::unique_ptr<Packet> decoded = Packet::readPacket(input, false);
	ctx.check(decoded != nullptr, "framed packet must decode");
	if (decoded != nullptr)
		ctx.checkEqual(decoded->getPacketId(), 0, "decoded packet id");
	ctx.checkEqual(input.read(), -1, "framed decoder consumed all bytes");
}

HEADLESS_TEST(packet, registry_initializes_exactly_once)
{
	Packet::ensurePacketRegistryInitialized();
	Packet::ensurePacketRegistryInitialized();
	ctx.checkEqual(static_cast<long long>(Packet::packetIdToFactory.size()), 59,
		"Alpha registered packet count");
}
