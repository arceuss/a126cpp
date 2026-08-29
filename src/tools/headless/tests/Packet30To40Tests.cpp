#include <type_traits>
#include <vector>

#include "network/Packet30Entity.h"
#include "network/Packet31RelEntityMove.h"
#include "network/Packet32EntityLook.h"
#include "network/Packet33RelEntityMoveLook.h"
#include "network/Packet34EntityTeleport.h"
#include "network/Packet38.h"
#include "network/Packet39.h"
#include "network/Packet40EntityMetadata.h"
#include "tools/headless/PacketTestUtils.h"
#include "tools/headless/TestFramework.h"

namespace
{
constexpr byte_t wireByte(unsigned value)
{
	return value <= 127U
		? static_cast<byte_t>(value)
		: static_cast<byte_t>(static_cast<int>(value) - 256);
}

template<typename PacketType>
void checkPacketBasics(headless::TestContext& ctx, PacketType& packet,
	const std::vector<byte_t>& expected, int packetId, int packetSize)
{
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Alpha payload bytes");
	ctx.checkEqual(packet.getPacketSize(), packetSize, "Alpha getPacketSize");
	ctx.checkEqual(packet.getPacketId(), packetId, "packet id");
	const std::vector<byte_t> framed = headless::encodeFramedPacket(packet);
	ctx.check(!framed.empty(), "framed packet has id byte");
	if (!framed.empty())
		ctx.checkEqual(static_cast<ubyte_t>(framed.front()), packetId, "framed packet id byte");
}

template<typename PacketType>
void checkRoundTrip(headless::TestContext& ctx, PacketType& packet,
	const std::vector<byte_t>& expected)
{
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"decoded fixture re-encodes exactly");
}
}

static_assert(std::is_base_of_v<Packet30Entity, Packet31RelEntityMove>);
static_assert(std::is_base_of_v<Packet30Entity, Packet32EntityLook>);
static_assert(std::is_base_of_v<Packet30Entity, Packet33RelEntityMoveLook>);

HEADLESS_TEST(packet, packet30_entity_alpha_wire)
{
	Packet30Entity packet;
	packet.entityId = 0x12345678;

	// Packet30Entity.java CFR 22-28 / Vineflower 20-28: writeInt(entityId).
	const std::vector<byte_t> expected{ 0x12, 0x34, 0x56, 0x78 };
	checkPacketBasics(ctx, packet, expected, 30, 4);

	auto decoded = headless::decodePacketData<Packet30Entity>(expected);
	ctx.checkEqual(decoded->entityId, 0x12345678, "entity id");
	ctx.check(!decoded->rotating, "Packet30 default rotating state");
	checkRoundTrip(ctx, *decoded, expected);
}

HEADLESS_TEST(packet, packet31_relative_move_alpha_wire)
{
	Packet31RelEntityMove packet;
	packet.entityId = 0x10203040;
	packet.xPosition = wireByte(0x80);
	packet.yPosition = wireByte(0xFF);
	packet.zPosition = 0x7F;

	// Packet31RelEntityMove.java CFR 13-25 / Vineflower 13-25: super int, then three signed bytes.
	const std::vector<byte_t> expected{
		0x10, 0x20, 0x30, 0x40, wireByte(0x80), wireByte(0xFF), 0x7F
	};
	checkPacketBasics(ctx, packet, expected, 31, 7);

	auto decoded = headless::decodePacketData<Packet31RelEntityMove>(expected);
	ctx.checkEqual(decoded->entityId, 0x10203040, "entity id");
	ctx.checkEqual(decoded->xPosition, -128, "negative x movement");
	ctx.checkEqual(decoded->yPosition, -1, "negative y movement");
	ctx.checkEqual(decoded->zPosition, 127, "positive z movement");
	ctx.check(!decoded->rotating, "relative move does not rotate");
	checkRoundTrip(ctx, *decoded, expected);
}

HEADLESS_TEST(packet, packet32_entity_look_alpha_wire)
{
	Packet32EntityLook packet;
	packet.entityId = static_cast<int_t>(0x89ABCDEFU);
	packet.yaw = wireByte(0x80);
	packet.pitch = 0x7F;

	// Packet32EntityLook.java CFR 17-27 / Vineflower 14-25: super int, then yaw and pitch bytes.
	const std::vector<byte_t> expected{
		wireByte(0x89), wireByte(0xAB), wireByte(0xCD), wireByte(0xEF), wireByte(0x80), 0x7F
	};
	checkPacketBasics(ctx, packet, expected, 32, 6);

	auto decoded = headless::decodePacketData<Packet32EntityLook>(expected);
	ctx.checkEqual(decoded->entityId, static_cast<int_t>(0x89ABCDEFU), "entity id");
	ctx.checkEqual(decoded->yaw, -128, "signed yaw");
	ctx.checkEqual(decoded->pitch, 127, "signed pitch");
	ctx.check(decoded->rotating, "look constructor sets rotating");
	checkRoundTrip(ctx, *decoded, expected);
}

HEADLESS_TEST(packet, packet33_relative_move_look_alpha_wire)
{
	Packet33RelEntityMoveLook packet;
	packet.entityId = -2;
	packet.xPosition = wireByte(0x81);
	packet.yPosition = 1;
	packet.zPosition = wireByte(0xFF);
	packet.yaw = 0x40;
	packet.pitch = wireByte(0xC0);

	// Packet33RelEntityMoveLook.java CFR 17-32 / Vineflower 14-30: super int, XYZ, yaw, pitch.
	const std::vector<byte_t> expected{
		wireByte(0xFF), wireByte(0xFF), wireByte(0xFF), wireByte(0xFE),
		wireByte(0x81), 0x01, wireByte(0xFF), 0x40, wireByte(0xC0)
	};
	checkPacketBasics(ctx, packet, expected, 33, 9);

	auto decoded = headless::decodePacketData<Packet33RelEntityMoveLook>(expected);
	ctx.checkEqual(decoded->entityId, -2, "entity id");
	ctx.checkEqual(decoded->xPosition, -127, "negative x movement");
	ctx.checkEqual(decoded->yPosition, 1, "positive y movement");
	ctx.checkEqual(decoded->zPosition, -1, "negative z movement");
	ctx.checkEqual(decoded->yaw, 64, "yaw");
	ctx.checkEqual(decoded->pitch, -64, "negative pitch");
	ctx.check(decoded->rotating, "move/look constructor sets rotating");
	checkRoundTrip(ctx, *decoded, expected);
}

HEADLESS_TEST(packet, packet34_entity_teleport_alpha_wire)
{
	Packet34EntityTeleport packet;
	packet.entityId = 0x01020304;
	packet.xPosition = 0x7FFFFFFF;
	packet.yPosition = static_cast<int_t>(0x80000000U);
	packet.zPosition = -2;
	packet.yaw = wireByte(0x80);
	packet.pitch = 0x7F;

	// Packet34EntityTeleport.java CFR 35-51 / Vineflower 29-47: four ints, then write(yaw), write(pitch).
	const std::vector<byte_t> expected{
		0x01, 0x02, 0x03, 0x04,
		0x7F, wireByte(0xFF), wireByte(0xFF), wireByte(0xFF),
		wireByte(0x80), 0x00, 0x00, 0x00,
		wireByte(0xFF), wireByte(0xFF), wireByte(0xFF), wireByte(0xFE),
		wireByte(0x80), 0x7F
	};
	checkPacketBasics(ctx, packet, expected, 34, 34);

	auto decoded = headless::decodePacketData<Packet34EntityTeleport>(expected);
	ctx.checkEqual(decoded->entityId, 0x01020304, "entity id");
	ctx.checkEqual(decoded->xPosition, 0x7FFFFFFF, "teleport x");
	ctx.checkEqual(decoded->yPosition, static_cast<int_t>(0x80000000U), "teleport y");
	ctx.checkEqual(decoded->zPosition, -2, "teleport z");
	ctx.checkEqual(decoded->yaw, -128, "teleport yaw");
	ctx.checkEqual(decoded->pitch, 127, "teleport pitch");
	checkRoundTrip(ctx, *decoded, expected);
}

HEADLESS_TEST(packet, packet38_entity_status_alpha_wire)
{
	Packet38 packet;
	packet.field_9274_a = 0x0A0B0C0D;
	packet.field_9273_b = wireByte(0x80);

	// Packet38.java CFR 17-25 / Vineflower 15-25: entity int followed by signed status byte.
	const std::vector<byte_t> expected{ 0x0A, 0x0B, 0x0C, 0x0D, wireByte(0x80) };
	checkPacketBasics(ctx, packet, expected, 38, 5);

	auto decoded = headless::decodePacketData<Packet38>(expected);
	ctx.checkEqual(decoded->field_9274_a, 0x0A0B0C0D, "status entity id");
	ctx.checkEqual(decoded->field_9273_b, -128, "signed entity status");
	checkRoundTrip(ctx, *decoded, expected);
}

HEADLESS_TEST(packet, packet39_attach_alpha_wire)
{
	Packet39 packet;
	packet.field_6365_a = 0x11223344;
	packet.field_6364_b = -2;

	// Packet39.java CFR 21-29 / Vineflower 20-30: field_6365_a then field_6364_b as ints.
	const std::vector<byte_t> expected{
		0x11, 0x22, 0x33, 0x44,
		wireByte(0xFF), wireByte(0xFF), wireByte(0xFF), wireByte(0xFE)
	};
	checkPacketBasics(ctx, packet, expected, 39, 8);

	auto decoded = headless::decodePacketData<Packet39>(expected);
	ctx.checkEqual(decoded->field_6365_a, 0x11223344, "first attach id");
	ctx.checkEqual(decoded->field_6364_b, -2, "second attach id");
	checkRoundTrip(ctx, *decoded, expected);
}

HEADLESS_TEST(packet, packet40_entity_metadata_alpha_wire)
{
	// Packet40EntityMetadata.java CFR 20-29 / Vineflower 17-26 delegates after entityId;
	// DataWatcher.java CFR 59-65, 75-113, 115-163 defines typed headers and the 0x7F terminator.
	const std::vector<byte_t> expected{
		0x01, 0x02, 0x03, 0x04,
		0x00, wireByte(0xFF),
		0x21, wireByte(0x80), 0x00,
		0x42, 0x12, 0x34, 0x56, 0x78,
		0x63, wireByte(0xC0), 0x20, 0x00, 0x00,
		wireByte(0x84), 0x00, 0x02, 0x00, 0x41, 0x03, wireByte(0xA9),
		wireByte(0xA5), 0x00, 0x01, wireByte(0xFE), wireByte(0xFF), wireByte(0xFD),
		wireByte(0xC6), 0x00, 0x00, 0x00, 0x01,
		wireByte(0xFF), wireByte(0xFF), wireByte(0xFF), wireByte(0xFE),
		0x10, 0x20, 0x30, 0x40,
		0x7F
	};

	auto decoded = headless::decodePacketData<Packet40EntityMetadata>(expected);
	checkPacketBasics(ctx, *decoded, expected, 40, 5);
	ctx.checkEqual(decoded->entityId, 0x01020304, "metadata entity id");
	const auto& objects = decoded->func_21047_b();
	ctx.checkEqual(static_cast<long long>(objects.size()), 7, "metadata entry count");
	if (objects.size() == 7)
	{
		ctx.checkEqual(objects[0]->getObjectType(), 0, "byte metadata type");
		ctx.checkEqual(objects[0]->getDataValueId(), 0, "byte metadata id");
		ctx.checkEqual(objects[0]->getByte(), -1, "byte metadata value");
		ctx.checkEqual(objects[1]->getShort(), -32768, "short metadata value");
		ctx.checkEqual(objects[2]->getInt(), 0x12345678, "int metadata value");
		ctx.checkEqualBits(objects[3]->getFloat(), -2.5f, "float metadata value");
		ctx.checkEqual(objects[4]->getString(), jstring(u"A\u03A9"), "string metadata value");
		const ItemStack stack = objects[5]->getItemStack();
		ctx.checkEqual(stack.itemID, 1, "item metadata id");
		ctx.checkEqual(stack.stackSize, -2, "item metadata count sign extension");
		ctx.checkEqual(stack.itemDamage, -3, "item metadata damage");
		const ChunkCoordinates coordinates = objects[6]->getChunkCoordinates();
		ctx.checkEqual(coordinates.x, 1, "coordinate metadata x");
		ctx.checkEqual(coordinates.y, -2, "coordinate metadata y");
		ctx.checkEqual(coordinates.z, 0x10203040, "coordinate metadata z");
	}
	checkRoundTrip(ctx, *decoded, expected);
}
