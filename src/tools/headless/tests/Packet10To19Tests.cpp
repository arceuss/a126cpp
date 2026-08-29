#include <memory>
#include <vector>

#include "network/NetHandler.h"
#include "network/Packet10Flying.h"
#include "network/Packet11PlayerPosition.h"
#include "network/Packet12PlayerLook.h"
#include "network/Packet13PlayerLookMove.h"
#include "network/Packet14BlockDig.h"
#include "network/Packet15Place.h"
#include "network/Packet16BlockItemSwitch.h"
#include "network/Packet17Sleep.h"
#include "network/Packet18ArmAnimation.h"
#include "network/Packet19EntityAction.h"
#include "tools/headless/PacketTestUtils.h"
#include "world/item/ItemStack.h"

static void checkPacketContract(headless::TestContext &ctx, Packet &packet, int expectedId,
	int expectedSize, const std::vector<byte_t> &expectedPayload)
{
	ctx.checkEqual(packet.getPacketId(), expectedId, "packet id");
	ctx.checkEqual(packet.getPacketSize(), expectedSize, "Alpha packet size");
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expectedPayload,
		"Alpha payload bytes");

	const std::vector<byte_t> framed = headless::encodeFramedPacket(packet);
	if (ctx.check(!framed.empty(), "framed packet must contain its id"))
		ctx.checkEqual(static_cast<ubyte_t>(framed[0]), expectedId, "framed packet id byte");
}

HEADLESS_TEST(packet, packet10_flying_wire)
{
	Packet10Flying defaults;
	ctx.check(!defaults.onGround && !defaults.moving && !defaults.rotating,
		"Packet10 default flags");

	Packet10Flying packet(true);
	// Alpha Packet10Flying.java CFR 35-40 / Vineflower 33-39: write(onGround ? 1 : 0).
	const std::vector<byte_t> expected = { 0x01 };
	checkPacketContract(ctx, packet, 10, 1, expected);

	std::unique_ptr<Packet10Flying> decoded = headless::decodePacketData<Packet10Flying>(expected);
	ctx.check(decoded->onGround, "decoded onGround");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet10 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet11_player_position_wire)
{
	Packet11PlayerPosition defaults;
	ctx.check(defaults.moving && !defaults.rotating && !defaults.onGround,
		"Packet11 default flags");

	Packet11PlayerPosition packet(1.0, -2.0, 3.5, -4.25, true);
	// Alpha Packet11PlayerPosition.java CFR 34-39 / Vineflower 33-38:
	// x, y, stance, z as doubles, followed by Packet10's onGround byte.
	const std::vector<byte_t> expected = {
		0x3f, static_cast<byte_t>(0xf0), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		static_cast<byte_t>(0xc0), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		static_cast<byte_t>(0xc0), 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01
	};
	checkPacketContract(ctx, packet, 11, 33, expected);

	std::unique_ptr<Packet11PlayerPosition> decoded =
		headless::decodePacketData<Packet11PlayerPosition>(expected);
	ctx.checkEqualBits(decoded->xPosition, 1.0, "decoded x");
	ctx.checkEqualBits(decoded->yPosition, -2.0, "decoded y");
	ctx.checkEqualBits(decoded->stance, 3.5, "decoded stance");
	ctx.checkEqualBits(decoded->zPosition, -4.25, "decoded z");
	ctx.check(decoded->onGround && decoded->moving, "decoded Packet11 flags");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet11 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet12_player_look_wire)
{
	Packet12PlayerLook defaults;
	ctx.check(defaults.rotating && !defaults.moving && !defaults.onGround,
		"Packet12 default flags");

	Packet12PlayerLook packet(1.5f, -2.25f, false);
	// Alpha Packet12PlayerLook.java CFR 30-33 / Vineflower 29-32:
	// yaw then pitch as floats, followed by Packet10's onGround byte.
	const std::vector<byte_t> expected = {
		0x3f, static_cast<byte_t>(0xc0), 0x00, 0x00,
		static_cast<byte_t>(0xc0), 0x10, 0x00, 0x00,
		0x00
	};
	checkPacketContract(ctx, packet, 12, 9, expected);

	std::unique_ptr<Packet12PlayerLook> decoded = headless::decodePacketData<Packet12PlayerLook>(expected);
	ctx.checkEqualBits(decoded->yaw, 1.5f, "decoded yaw");
	ctx.checkEqualBits(decoded->pitch, -2.25f, "decoded pitch");
	ctx.check(!decoded->onGround && decoded->rotating, "decoded Packet12 flags");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet12 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet13_player_look_move_wire)
{
	Packet13PlayerLookMove defaults;
	ctx.check(defaults.moving && defaults.rotating && !defaults.onGround,
		"Packet13 default flags");

	Packet13PlayerLookMove packet(1.0, -2.0, 3.5, -4.25, 5.5f, -6.75f, true);
	// Alpha Packet13PlayerLookMove.java CFR 40-47 / Vineflower 39-46:
	// x, y, stance, z doubles; yaw, pitch floats; then Packet10's onGround byte.
	const std::vector<byte_t> expected = {
		0x3f, static_cast<byte_t>(0xf0), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		static_cast<byte_t>(0xc0), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		static_cast<byte_t>(0xc0), 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, static_cast<byte_t>(0xb0), 0x00, 0x00,
		static_cast<byte_t>(0xc0), static_cast<byte_t>(0xd8), 0x00, 0x00,
		0x01
	};
	checkPacketContract(ctx, packet, 13, 41, expected);

	std::unique_ptr<Packet13PlayerLookMove> decoded =
		headless::decodePacketData<Packet13PlayerLookMove>(expected);
	ctx.checkEqualBits(decoded->xPosition, 1.0, "decoded x");
	ctx.checkEqualBits(decoded->yPosition, -2.0, "decoded y");
	ctx.checkEqualBits(decoded->stance, 3.5, "decoded stance");
	ctx.checkEqualBits(decoded->zPosition, -4.25, "decoded z");
	ctx.checkEqualBits(decoded->yaw, 5.5f, "decoded yaw");
	ctx.checkEqualBits(decoded->pitch, -6.75f, "decoded pitch");
	ctx.check(decoded->onGround && decoded->moving && decoded->rotating,
		"decoded Packet13 flags");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet13 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet14_block_dig_wire)
{
	Packet14BlockDig packet(0x1fe, 0x12345678, 0x180, -2, -1);
	ctx.checkEqual(packet.status, 0x1fe, "constructor preserves Java int status");
	ctx.checkEqual(packet.yPosition, 0x180, "constructor preserves Java int y");
	ctx.checkEqual(packet.face, -1, "constructor preserves Java int face");

	// Alpha Packet14BlockDig.java CFR 39-44 / Vineflower 37-42:
	// status byte, x int, y byte, z int, face byte.
	const std::vector<byte_t> expected = {
		static_cast<byte_t>(0xfe), 0x12, 0x34, 0x56, 0x78, static_cast<byte_t>(0x80),
		static_cast<byte_t>(0xff), static_cast<byte_t>(0xff), static_cast<byte_t>(0xff),
		static_cast<byte_t>(0xfe), static_cast<byte_t>(0xff)
	};
	checkPacketContract(ctx, packet, 14, 11, expected);

	std::unique_ptr<Packet14BlockDig> decoded = headless::decodePacketData<Packet14BlockDig>(expected);
	ctx.checkEqual(decoded->status, 254, "read() status is unsigned");
	ctx.checkEqual(decoded->xPosition, 0x12345678, "decoded x");
	ctx.checkEqual(decoded->yPosition, 128, "read() y is unsigned");
	ctx.checkEqual(decoded->zPosition, -2, "decoded z");
	ctx.checkEqual(decoded->face, 255, "read() face is unsigned");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet14 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet15_place_non_null_wire)
{
	std::shared_ptr<ItemStack> stack = std::make_shared<ItemStack>(0x1234, -2, -3);
	Packet15Place packet(0x12345678, 0x180, -2, -1, stack);
	ctx.checkEqual(packet.yPosition, 0x180, "constructor preserves Java int y");
	ctx.checkEqual(packet.direction, -1, "constructor preserves Java int direction");

	// Alpha Packet15Place.java CFR 49-60 / Vineflower 44-55:
	// x int, y byte, z int, direction byte, item id short, count byte, damage short.
	const std::vector<byte_t> expected = {
		0x12, 0x34, 0x56, 0x78, static_cast<byte_t>(0x80),
		static_cast<byte_t>(0xff), static_cast<byte_t>(0xff), static_cast<byte_t>(0xff),
		static_cast<byte_t>(0xfe), static_cast<byte_t>(0xff),
		0x12, 0x34, static_cast<byte_t>(0xfe), static_cast<byte_t>(0xff),
		static_cast<byte_t>(0xfd)
	};
	checkPacketContract(ctx, packet, 15, 15, expected);

	std::unique_ptr<Packet15Place> decoded = headless::decodePacketData<Packet15Place>(expected);
	ctx.checkEqual(decoded->xPosition, 0x12345678, "decoded x");
	ctx.checkEqual(decoded->yPosition, 128, "read() y is unsigned");
	ctx.checkEqual(decoded->zPosition, -2, "decoded z");
	ctx.checkEqual(decoded->direction, 255, "read() direction is unsigned");
	if (ctx.check(decoded->itemStack != nullptr, "non-null stack decoded"))
	{
		ctx.checkEqual(decoded->itemStack->itemID, 0x1234, "decoded item id");
		ctx.checkEqual(decoded->itemStack->stackSize, -2, "readByte count is signed");
		ctx.checkEqual(decoded->itemStack->itemDamage, -3, "decoded item damage");
	}
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet15 non-null decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet15_place_null_wire)
{
	Packet15Place packet(-1, 0x7f, 0x01020304, 0x80, nullptr);
	// Alpha Packet15Place.java CFR 49-55 / Vineflower 44-50:
	// the nullable branch ends the payload with the literal short -1 sentinel.
	const std::vector<byte_t> expected = {
		static_cast<byte_t>(0xff), static_cast<byte_t>(0xff), static_cast<byte_t>(0xff),
		static_cast<byte_t>(0xff), 0x7f, 0x01, 0x02, 0x03, 0x04,
		static_cast<byte_t>(0x80), static_cast<byte_t>(0xff), static_cast<byte_t>(0xff)
	};
	checkPacketContract(ctx, packet, 15, 15, expected);

	std::unique_ptr<Packet15Place> decoded = headless::decodePacketData<Packet15Place>(expected);
	ctx.check(decoded->itemStack == nullptr, "-1 item id decodes to null");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet15 null decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet16_block_item_switch_wire)
{
	Packet16BlockItemSwitch packet(0x18000);
	ctx.checkEqual(packet.id, 0x18000, "constructor preserves Java int id");
	// Alpha Packet16BlockItemSwitch.java CFR 29-30 / Vineflower 25-26: writeShort(id).
	const std::vector<byte_t> expected = { static_cast<byte_t>(0x80), 0x00 };
	checkPacketContract(ctx, packet, 16, 2, expected);

	std::unique_ptr<Packet16BlockItemSwitch> decoded =
		headless::decodePacketData<Packet16BlockItemSwitch>(expected);
	ctx.checkEqual(decoded->id, -32768, "readShort sign extension");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet16 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet17_sleep_wire)
{
	Packet17Sleep packet;
	packet.field_22045_a = 0x12345678;
	packet.field_22046_e = -128;
	packet.field_22044_b = -2;
	packet.field_22048_c = 127;
	packet.field_22047_d = 0x01020304;

	// Alpha Packet17Sleep.java CFR 30-35 / Vineflower 28-33:
	// field a int, field e signed byte, field b int, field c signed byte, field d int.
	const std::vector<byte_t> expected = {
		0x12, 0x34, 0x56, 0x78, static_cast<byte_t>(0x80),
		static_cast<byte_t>(0xff), static_cast<byte_t>(0xff), static_cast<byte_t>(0xff),
		static_cast<byte_t>(0xfe), 0x7f, 0x01, 0x02, 0x03, 0x04
	};
	checkPacketContract(ctx, packet, 17, 14, expected);

	std::unique_ptr<Packet17Sleep> decoded = headless::decodePacketData<Packet17Sleep>(expected);
	ctx.checkEqual(decoded->field_22045_a, 0x12345678, "decoded field a");
	ctx.checkEqual(decoded->field_22046_e, -128, "readByte field e is signed");
	ctx.checkEqual(decoded->field_22044_b, -2, "decoded field b");
	ctx.checkEqual(decoded->field_22048_c, 127, "decoded field c");
	ctx.checkEqual(decoded->field_22047_d, 0x01020304, "decoded field d");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet17 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet18_arm_animation_wire)
{
	Packet18ArmAnimation packet(0x12345678, -128);
	// Alpha Packet18ArmAnimation.java CFR 31-33 / Vineflower 28-30:
	// entity id int followed by signed animation byte.
	const std::vector<byte_t> expected = {
		0x12, 0x34, 0x56, 0x78, static_cast<byte_t>(0x80)
	};
	checkPacketContract(ctx, packet, 18, 5, expected);

	std::unique_ptr<Packet18ArmAnimation> decoded =
		headless::decodePacketData<Packet18ArmAnimation>(expected);
	ctx.checkEqual(decoded->entityId, 0x12345678, "decoded entity id");
	ctx.checkEqual(decoded->animate, -128, "readByte animation is signed");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet18 decoded fixture re-encode");
}

HEADLESS_TEST(packet, packet19_entity_action_wire)
{
	Packet19EntityAction packet(-2, -1);
	// Alpha Packet19EntityAction.java CFR 33-35 / Vineflower 28-30:
	// entity id int followed by signed state byte.
	const std::vector<byte_t> expected = {
		static_cast<byte_t>(0xff), static_cast<byte_t>(0xff), static_cast<byte_t>(0xff),
		static_cast<byte_t>(0xfe), static_cast<byte_t>(0xff)
	};
	checkPacketContract(ctx, packet, 19, 5, expected);

	std::unique_ptr<Packet19EntityAction> decoded =
		headless::decodePacketData<Packet19EntityAction>(expected);
	ctx.checkEqual(decoded->entityId, -2, "decoded entity id");
	ctx.checkEqual(decoded->state, -1, "readByte state is signed");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet19 decoded fixture re-encode");
}

class PacketRouteHandler : public NetHandler
{
public:
	enum class Route
	{
		None,
		Flying,
		BlockDig,
		Place,
		BlockItemSwitch,
		AddToInventory,
		ArmAnimation,
		EntityAction
	};

	bool isServerHandler() override { return false; }
	void handleFlying(Packet10Flying *packet) override { record(Route::Flying, packet); }
	void handleBlockDig(Packet14BlockDig *packet) override { record(Route::BlockDig, packet); }
	void handlePlace(Packet15Place *packet) override { record(Route::Place, packet); }
	void handleBlockItemSwitch(Packet16BlockItemSwitch *packet) override
	{
		record(Route::BlockItemSwitch, packet);
	}
	void handleAddToInventory(Packet17Sleep *packet) override { record(Route::AddToInventory, packet); }
	void handleArmAnimation(Packet18ArmAnimation *packet) override { record(Route::ArmAnimation, packet); }
	void handleEntityAction(Packet19EntityAction *packet) override { record(Route::EntityAction, packet); }

	void clear()
	{
		route = Route::None;
		received = nullptr;
	}

	Route route = Route::None;
	Packet *received = nullptr;

private:
	void record(Route newRoute, Packet *packet)
	{
		route = newRoute;
		received = packet;
	}
};

HEADLESS_TEST(packet, packet10_to_19_handler_routes)
{
	PacketRouteHandler handler;
	Packet10Flying packet10;
	Packet11PlayerPosition packet11;
	Packet12PlayerLook packet12;
	Packet13PlayerLookMove packet13;
	Packet14BlockDig packet14;
	Packet15Place packet15;
	Packet16BlockItemSwitch packet16;
	Packet17Sleep packet17;
	Packet18ArmAnimation packet18;
	Packet19EntityAction packet19;

	auto checkRoute = [&ctx, &handler](Packet &packet, PacketRouteHandler::Route expected,
		const char *message)
	{
		handler.clear();
		packet.processPacket(&handler);
		ctx.check(handler.route == expected, message);
		ctx.check(handler.received == &packet, std::string(message) + " packet argument");
	};

	checkRoute(packet10, PacketRouteHandler::Route::Flying, "Packet10 handleFlying");
	checkRoute(packet11, PacketRouteHandler::Route::Flying, "Packet11 inherited handleFlying");
	checkRoute(packet12, PacketRouteHandler::Route::Flying, "Packet12 inherited handleFlying");
	checkRoute(packet13, PacketRouteHandler::Route::Flying, "Packet13 inherited handleFlying");
	checkRoute(packet14, PacketRouteHandler::Route::BlockDig, "Packet14 handleBlockDig");
	checkRoute(packet15, PacketRouteHandler::Route::Place, "Packet15 handlePlace");
	checkRoute(packet16, PacketRouteHandler::Route::BlockItemSwitch,
		"Packet16 handleBlockItemSwitch");
	checkRoute(packet17, PacketRouteHandler::Route::AddToInventory,
		"Packet17 handleAddToInventory");
	checkRoute(packet18, PacketRouteHandler::Route::ArmAnimation, "Packet18 handleArmAnimation");
	checkRoute(packet19, PacketRouteHandler::Route::EntityAction, "Packet19 handleEntityAction");
}
