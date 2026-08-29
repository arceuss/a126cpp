// Exact Alpha 1.2.6 wire fixtures for packet IDs 0 through 9.

#include <memory>
#include <string>
#include <vector>

#include "network/Packet0KeepAlive.h"
#include "network/Packet1Login.h"
#include "network/Packet2Handshake.h"
#include "network/Packet3Chat.h"
#include "network/Packet4UpdateTime.h"
#include "network/Packet5PlayerInventory.h"
#include "network/Packet6SpawnPosition.h"
#include "network/Packet7.h"
#include "network/Packet8.h"
#include "network/Packet9.h"
#include "tools/headless/PacketTestUtils.h"
#include "tools/headless/TestFramework.h"

static void checkFramedPacketId(headless::TestContext &ctx, Packet &packet, int expectedId)
{
	ctx.checkEqual(packet.getPacketId(), expectedId, "packet id");
	const std::vector<byte_t> framed = headless::encodeFramedPacket(packet);
	if (!ctx.check(!framed.empty(), "framed packet has an id byte"))
		return;
	ctx.checkEqual(static_cast<ubyte_t>(framed.front()), expectedId, "framed packet id byte");
}

template<typename Function>
static bool packetCodecThrows(Function &&function)
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

HEADLESS_TEST(packet, packet_0_keep_alive_alpha_wire)
{
	Packet0KeepAlive packet;

	// Alpha Packet0KeepAlive.java:17-24: both codecs are empty and the size is zero.
	const std::vector<byte_t> expected;
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet0KeepAlive payload"))
		return;
	std::unique_ptr<Packet0KeepAlive> decoded =
		headless::decodePacketData<Packet0KeepAlive>(expected);
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet0KeepAlive decoded re-encode");
	ctx.checkEqual(packet.getPacketSize(), 0, "Packet0KeepAlive Alpha size");
	checkFramedPacketId(ctx, packet, 0);
}

HEADLESS_TEST(packet, packet_1_login_alpha_wire)
{
	Packet1Login packet(u"A\u03A9", 0x12345678);
	packet.field_4074_d = 0x0102030405060708LL;
	packet.field_4073_e = static_cast<byte_t>(-1);

	// Alpha Packet1Login.java:36-40: int, UTF-16 string, long, then signed byte.
	const std::vector<byte_t> expected = {
		0x12, 0x34, 0x56, 0x78,
		0x00, 0x02, 0x00, 0x41, 0x03, static_cast<byte_t>(0xA9),
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		static_cast<byte_t>(0xFF)
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet1Login payload"))
		return;
	std::unique_ptr<Packet1Login> decoded = headless::decodePacketData<Packet1Login>(expected);
	ctx.checkEqual(decoded->protocolVersion, 0x12345678, "Packet1Login protocol version");
	ctx.checkEqual(decoded->username, jstring(u"A\u03A9"), "Packet1Login username");
	ctx.checkEqual(decoded->field_4074_d, 0x0102030405060708LL, "Packet1Login world seed");
	ctx.checkEqual(decoded->field_4073_e, -1, "Packet1Login dimension");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet1Login decoded re-encode");
	// Alpha Packet1Login.java:49-50 preserves the original, non-wire-length expression.
	ctx.checkEqual(packet.getPacketSize(), 15, "Packet1Login Alpha size expression");
	checkFramedPacketId(ctx, packet, 1);
}

HEADLESS_TEST(packet, packet_2_handshake_alpha_wire)
{
	Packet2Handshake packet(u"Hi\u20AC");

	// Alpha Packet2Handshake.java:24-30: a 32-code-unit-limited UTF-16 string.
	const std::vector<byte_t> expected = {
		0x00, 0x03, 0x00, 0x48, 0x00, 0x69, 0x20, static_cast<byte_t>(0xAC)
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet2Handshake payload"))
		return;
	std::unique_ptr<Packet2Handshake> decoded =
		headless::decodePacketData<Packet2Handshake>(expected);
	ctx.checkEqual(decoded->username, jstring(u"Hi\u20AC"), "Packet2Handshake username");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet2Handshake decoded re-encode");
	// Alpha Packet2Handshake.java:39-40 preserves the original size expression.
	ctx.checkEqual(packet.getPacketSize(), 11, "Packet2Handshake Alpha size expression");
	checkFramedPacketId(ctx, packet, 2);
}

HEADLESS_TEST(packet, packet_3_chat_alpha_wire_and_constructor_limit)
{
	Packet3Chat packet(u"\u00A7A");

	// Alpha Packet3Chat.java:27-33: a 119-code-unit-limited UTF-16 string.
	const std::vector<byte_t> expected = {
		0x00, 0x02, 0x00, static_cast<byte_t>(0xA7), 0x00, 0x41
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet3Chat payload"))
		return;
	std::unique_ptr<Packet3Chat> decoded = headless::decodePacketData<Packet3Chat>(expected);
	ctx.checkEqual(decoded->message, jstring(u"\u00A7A"), "Packet3Chat message");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet3Chat decoded re-encode");
	ctx.checkEqual(packet.getPacketSize(), 2, "Packet3Chat Alpha size");

	// Alpha Packet3Chat.java:19-23 truncates the value constructor at 119 code units.
	Packet3Chat truncated(jstring(120, u'x'));
	ctx.checkEqual(static_cast<long long>(truncated.message.length()), 119,
		"Packet3Chat constructor limit");
	checkFramedPacketId(ctx, packet, 3);
}

HEADLESS_TEST(packet, packet_1_to_3_alpha_read_string_limits)
{
	// Alpha Packet1Login.java:28-32 rejects usernames above 16 UTF-16 code units.
	const std::vector<byte_t> loginTooLong = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x11
	};
	ctx.check(packetCodecThrows([&]() {
		(void)headless::decodePacketData<Packet1Login>(loginTooLong);
	}), "Packet1Login 16-code-unit read limit");

	// Alpha Packet2Handshake.java:24-25 rejects usernames above 32 UTF-16 code units.
	const std::vector<byte_t> handshakeTooLong = { 0x00, 0x21 };
	ctx.check(packetCodecThrows([&]() {
		(void)headless::decodePacketData<Packet2Handshake>(handshakeTooLong);
	}), "Packet2Handshake 32-code-unit read limit");

	// Alpha Packet3Chat.java:27-28 rejects messages above 119 UTF-16 code units.
	const std::vector<byte_t> chatTooLong = { 0x00, 0x78 };
	ctx.check(packetCodecThrows([&]() {
		(void)headless::decodePacketData<Packet3Chat>(chatTooLong);
	}), "Packet3Chat 119-code-unit read limit");
}

HEADLESS_TEST(packet, packet_4_update_time_alpha_wire)
{
	Packet4UpdateTime packet;
	packet.time = -81985529216486896LL;

	// Alpha Packet4UpdateTime.java:16-21: one signed big-endian long.
	const std::vector<byte_t> expected = {
		static_cast<byte_t>(0xFE), static_cast<byte_t>(0xDC), static_cast<byte_t>(0xBA),
		static_cast<byte_t>(0x98), 0x76, 0x54, 0x32, 0x10
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet4UpdateTime payload"))
		return;
	std::unique_ptr<Packet4UpdateTime> decoded =
		headless::decodePacketData<Packet4UpdateTime>(expected);
	ctx.checkEqual(decoded->time, -81985529216486896LL, "Packet4UpdateTime time");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet4UpdateTime decoded re-encode");
	ctx.checkEqual(packet.getPacketSize(), 8, "Packet4UpdateTime Alpha size");
	checkFramedPacketId(ctx, packet, 4);
}

HEADLESS_TEST(packet, packet_5_player_inventory_alpha_wire_and_null_item)
{
	Packet5PlayerInventory packet;
	packet.entityID = 0x10203040;
	packet.slot = -32768;
	packet.itemID = 32767;
	packet.itemDamage = -2;

	// Alpha Packet5PlayerInventory.java:28-32: int followed by three signed shorts.
	const std::vector<byte_t> expected = {
		0x10, 0x20, 0x30, 0x40,
		static_cast<byte_t>(0x80), 0x00,
		0x7F, static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFE)
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet5PlayerInventory payload"))
		return;
	std::unique_ptr<Packet5PlayerInventory> decoded =
		headless::decodePacketData<Packet5PlayerInventory>(expected);
	ctx.checkEqual(decoded->entityID, 0x10203040, "Packet5PlayerInventory entity id");
	ctx.checkEqual(decoded->slot, -32768, "Packet5PlayerInventory slot");
	ctx.checkEqual(decoded->itemID, 32767, "Packet5PlayerInventory item id");
	ctx.checkEqual(decoded->itemDamage, -2, "Packet5PlayerInventory item damage");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet5PlayerInventory decoded re-encode");
	// Alpha Packet5PlayerInventory.java:41-42 returns 8 despite its ten wire bytes.
	ctx.checkEqual(packet.getPacketSize(), 8, "Packet5PlayerInventory Alpha size expression");
	checkFramedPacketId(ctx, packet, 5);

	Packet5PlayerInventory nullItem;
	nullItem.entityID = 0x556677;
	nullItem.slot = 5;
	nullItem.itemID = -1;
	nullItem.itemDamage = 0;
	// Alpha Packet5PlayerInventory.java:28-32 has no conditional ItemStack branch;
	// itemID -1 is emitted through the same fixed-width short field.
	const std::vector<byte_t> nullExpected = {
		0x00, 0x55, 0x66, 0x77, 0x00, 0x05,
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), 0x00, 0x00
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(nullItem), nullExpected,
		"Packet5PlayerInventory null item payload"))
		return;
	std::unique_ptr<Packet5PlayerInventory> decodedNull =
		headless::decodePacketData<Packet5PlayerInventory>(nullExpected);
	ctx.checkEqual(decodedNull->itemID, -1, "Packet5PlayerInventory null item sentinel");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decodedNull), nullExpected,
		"Packet5PlayerInventory null item decoded re-encode");
}

HEADLESS_TEST(packet, packet_6_spawn_position_alpha_wire)
{
	Packet6SpawnPosition packet;
	packet.xPosition = 0x01234567;
	packet.yPosition = -2;
	packet.zPosition = 0x76543210;

	// Alpha Packet6SpawnPosition.java:18-27: x, y, and z as three signed ints.
	const std::vector<byte_t> expected = {
		0x01, 0x23, 0x45, 0x67,
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFE),
		0x76, 0x54, 0x32, 0x10
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet6SpawnPosition payload"))
		return;
	std::unique_ptr<Packet6SpawnPosition> decoded =
		headless::decodePacketData<Packet6SpawnPosition>(expected);
	ctx.checkEqual(decoded->xPosition, 0x01234567, "Packet6SpawnPosition x");
	ctx.checkEqual(decoded->yPosition, -2, "Packet6SpawnPosition y");
	ctx.checkEqual(decoded->zPosition, 0x76543210, "Packet6SpawnPosition z");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet6SpawnPosition decoded re-encode");
	ctx.checkEqual(packet.getPacketSize(), 12, "Packet6SpawnPosition Alpha size");
	checkFramedPacketId(ctx, packet, 6);
}

HEADLESS_TEST(packet, packet_7_alpha_wire_and_int_backed_byte)
{
	Packet7 packet(0x10203040, -2, -128);

	// Alpha Packet7.java:27-36: two ints followed by writeByte of an int field.
	const std::vector<byte_t> expected = {
		0x10, 0x20, 0x30, 0x40,
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFE),
		static_cast<byte_t>(0x80)
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet7 payload"))
		return;
	std::unique_ptr<Packet7> decoded = headless::decodePacketData<Packet7>(expected);
	ctx.checkEqual(decoded->field_9277_a, 0x10203040, "Packet7 first entity id");
	ctx.checkEqual(decoded->field_9276_b, -2, "Packet7 second entity id");
	ctx.checkEqual(decoded->field_9278_c, -128, "Packet7 signed byte read into int");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet7 decoded re-encode");
	ctx.checkEqual(packet.getPacketSize(), 9, "Packet7 Alpha size");
	checkFramedPacketId(ctx, packet, 7);

	// Alpha Packet7.java:14-24 keeps the constructor argument as int; writeByte emits its low byte.
	Packet7 wideAction(1, 2, 0x123);
	ctx.checkEqual(wideAction.field_9278_c, 0x123, "Packet7 constructor preserves int action");
	const std::vector<byte_t> wideExpected = {
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x23
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(wideAction), wideExpected,
		"Packet7 writeByte low-byte semantics");
}

HEADLESS_TEST(packet, packet_8_health_alpha_wire)
{
	Packet8 packet;
	packet.healthMP = -32768;

	// Alpha Packet8.java:17-23: the int field is read and written as a signed short.
	const std::vector<byte_t> expected = { static_cast<byte_t>(0x80), 0x00 };
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet8 payload"))
		return;
	std::unique_ptr<Packet8> decoded = headless::decodePacketData<Packet8>(expected);
	ctx.checkEqual(decoded->healthMP, -32768, "Packet8 signed short health");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet8 decoded re-encode");
	ctx.checkEqual(packet.getPacketSize(), 2, "Packet8 Alpha size");
	checkFramedPacketId(ctx, packet, 8);

	Packet8 wideHealth;
	wideHealth.healthMP = 0x12345;
	const std::vector<byte_t> truncatedExpected = { 0x23, 0x45 };
	headless::checkPacketBytes(ctx, headless::encodePacketData(wideHealth), truncatedExpected,
		"Packet8 writeShort low-word semantics");
}

HEADLESS_TEST(packet, packet_9_respawn_alpha_wire)
{
	Packet9 packet(static_cast<byte_t>(-128), 0x0102030405060708LL);

	// Alpha Packet9.java:31-39: signed dimension byte followed by the world-seed long.
	const std::vector<byte_t> expected = {
		static_cast<byte_t>(0x80),
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
	};
	if (!headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet9 payload"))
		return;
	std::unique_ptr<Packet9> decoded = headless::decodePacketData<Packet9>(expected);
	ctx.checkEqual(decoded->field_28048_a, -128, "Packet9 dimension");
	ctx.checkEqual(decoded->seed, 0x0102030405060708LL, "Packet9 seed");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet9 decoded re-encode");
	ctx.checkEqual(packet.getPacketSize(), 9, "Packet9 Alpha size");
	checkFramedPacketId(ctx, packet, 9);
}
