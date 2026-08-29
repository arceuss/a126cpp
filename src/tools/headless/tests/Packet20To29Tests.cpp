#include <limits>
#include <memory>
#include <vector>

#include "network/Packet20NamedEntitySpawn.h"
#include "network/Packet21PickupSpawn.h"
#include "network/Packet22Collect.h"
#include "network/Packet23VehicleSpawn.h"
#include "network/Packet24MobSpawn.h"
#include "network/Packet25EntityPainting.h"
#include "network/Packet27Position.h"
#include "network/Packet28.h"
#include "network/Packet29DestroyEntity.h"
#include "tools/headless/PacketTestUtils.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/Painting.h"
#include "world/entity/WatchableObject.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/player/Player.h"
#include "world/item/ItemStack.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"
#include "world/phys/ChunkCoordinates.h"

namespace
{
void checkFramedId(headless::TestContext &ctx, Packet &packet, int_t expectedId)
{
	const std::vector<byte_t> framed = headless::encodeFramedPacket(packet);
	if (ctx.check(!framed.empty(), "framed packet is not empty"))
		ctx.checkEqual(static_cast<ubyte_t>(framed.front()), expectedId, "framed packet id");
}
}

HEADLESS_TEST(packet, packet20_named_entity_spawn_alpha_wire)
{
	Packet20NamedEntitySpawn packet;
	packet.entityId = 0x12345678;
	packet.name = u"A\u03A9";
	packet.xPosition = -1;
	packet.yPosition = 0x01020304;
	packet.zPosition = (std::numeric_limits<int_t>::min)();
	packet.rotation = -128;
	packet.pitch = 127;
	packet.currentItem = -292;

	// Alpha Packet20NamedEntitySpawn.java:54-62 writes int, UTF-16 string,
	// three ints, two signed bytes, then the current-item short.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x12), static_cast<byte_t>(0x34), static_cast<byte_t>(0x56), static_cast<byte_t>(0x78),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x02), static_cast<byte_t>(0x00), static_cast<byte_t>(0x41),
		static_cast<byte_t>(0x03), static_cast<byte_t>(0xA9), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0x01), static_cast<byte_t>(0x02),
		static_cast<byte_t>(0x03), static_cast<byte_t>(0x04), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x80), static_cast<byte_t>(0x7F),
		static_cast<byte_t>(0xFE), static_cast<byte_t>(0xDC)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet20 Alpha payload");
	auto decoded = headless::decodePacketData<Packet20NamedEntitySpawn>(expected);
	ctx.checkEqual(decoded->entityId, 0x12345678, "Packet20 entity id");
	ctx.checkEqual(decoded->name, jstring(u"A\u03A9"), "Packet20 name");
	ctx.checkEqual(decoded->xPosition, -1, "Packet20 negative x");
	ctx.checkEqual(decoded->currentItem, -292, "Packet20 signed current-item short");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet20 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 28, "Packet20 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 20, "Packet20 id");
	checkFramedId(ctx, packet, 20);

	Level level(u"packet20-constructor", Dimension::Id_Normal, 20LL);
	Player player(level);
	player.entityId = 77;
	player.name = u"Constructor";
	player.x = -1.01;
	player.y = 2.03125;
	player.z = -0.03125;
	player.yRot = 270.0f;
	player.xRot = -90.0f;
	player.inventory.mainInventory[0] = ItemStack(0x1234, 1, 0);
	Packet20NamedEntitySpawn constructed(player);
	ctx.checkEqual(constructed.xPosition, -33, "Packet20 Java floor x");
	ctx.checkEqual(constructed.yPosition, 65, "Packet20 Java floor y");
	ctx.checkEqual(constructed.zPosition, -1, "Packet20 Java floor z");
	ctx.checkEqual(constructed.rotation, -64, "Packet20 Java float-to-byte yaw");
	ctx.checkEqual(constructed.pitch, -64, "Packet20 Java float-to-byte pitch");
	ctx.checkEqual(constructed.currentItem, 0x1234, "Packet20 selected item");
}

HEADLESS_TEST(packet, packet21_pickup_spawn_alpha_wire)
{
	Packet21PickupSpawn packet;
	packet.entityId = static_cast<int_t>(0x89ABCDEFu);
	packet.itemId = -32768;
	packet.count = -128;
	packet.itemDamage = 32767;
	packet.xPosition = -1;
	packet.yPosition = 0x01020304;
	packet.zPosition = (std::numeric_limits<int_t>::min)();
	packet.rotation = -1;
	packet.pitch = 127;
	packet.roll = -128;

	// Alpha Packet21PickupSpawn.java:58-68 writes int, short, byte, short,
	// three position ints, then three signed motion bytes.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x89), static_cast<byte_t>(0xAB), static_cast<byte_t>(0xCD), static_cast<byte_t>(0xEF),
		static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x80), static_cast<byte_t>(0x7F),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x01), static_cast<byte_t>(0x02), static_cast<byte_t>(0x03),
		static_cast<byte_t>(0x04), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0xFF), static_cast<byte_t>(0x7F), static_cast<byte_t>(0x80)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet21 Alpha payload");
	auto decoded = headless::decodePacketData<Packet21PickupSpawn>(expected);
	ctx.checkEqual(decoded->entityId, static_cast<int_t>(0x89ABCDEFu), "Packet21 entity id");
	ctx.checkEqual(decoded->itemId, -32768, "Packet21 signed item short");
	ctx.checkEqual(decoded->count, -128, "Packet21 signed count byte");
	ctx.checkEqual(decoded->itemDamage, 32767, "Packet21 signed damage short");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet21 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 24, "Packet21 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 21, "Packet21 id");
	checkFramedId(ctx, packet, 21);

	Level level(u"packet21-constructor", Dimension::Id_Normal, 21LL);
	EntityItem entity(level);
	entity.entityId = 88;
	entity.item = ItemStack(0x1234, 200, 0x4321);
	entity.x = -0.01;
	entity.y = -1.0;
	entity.z = 2.99;
	entity.xd = -1.0078125;
	entity.yd = 0.9921875;
	entity.zd = 1.0;
	Packet21PickupSpawn constructed(entity);
	ctx.checkEqual(constructed.xPosition, -1, "Packet21 Java floor x");
	ctx.checkEqual(constructed.yPosition, -32, "Packet21 Java floor y");
	ctx.checkEqual(constructed.zPosition, 95, "Packet21 Java floor z");
	ctx.checkEqual(constructed.rotation, 127, "Packet21 Java double-to-byte x motion");
	ctx.checkEqual(constructed.pitch, 127, "Packet21 Java double-to-byte y motion");
	ctx.checkEqual(constructed.roll, -128, "Packet21 Java double-to-byte z motion");
	ctx.checkEqual(constructed.count, 200, "Packet21 constructor keeps Java int count");
}

HEADLESS_TEST(packet, packet22_collect_alpha_wire)
{
	Packet22Collect packet;
	packet.collectedEntityId = 0x01234567;
	packet.collectorEntityId = static_cast<int_t>(0x89ABCDEFu);

	// Alpha Packet22Collect.java:22-25 writes collected id before collector id.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x01), static_cast<byte_t>(0x23), static_cast<byte_t>(0x45), static_cast<byte_t>(0x67),
		static_cast<byte_t>(0x89), static_cast<byte_t>(0xAB), static_cast<byte_t>(0xCD), static_cast<byte_t>(0xEF)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet22 Alpha payload");
	auto decoded = headless::decodePacketData<Packet22Collect>(expected);
	ctx.checkEqual(decoded->collectedEntityId, 0x01234567, "Packet22 collected id");
	ctx.checkEqual(decoded->collectorEntityId, static_cast<int_t>(0x89ABCDEFu), "Packet22 collector id");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet22 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 8, "Packet22 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 22, "Packet22 id");
	checkFramedId(ctx, packet, 22);
}

HEADLESS_TEST(packet, packet23_vehicle_spawn_alpha_wire)
{
	Packet23VehicleSpawn packet;
	packet.entityId = 0x10203040;
	packet.type = -128;
	packet.xPosition = -1;
	packet.yPosition = (std::numeric_limits<int_t>::max)();
	packet.zPosition = (std::numeric_limits<int_t>::min)();
	packet.field_28044_i = 1;
	packet.field_28047_e = -32768;
	packet.field_28046_f = 32767;
	packet.field_28045_g = -1;

	// Alpha Packet23VehicleSpawn.java:40-50 writes the signed type byte and,
	// when field_28044_i > 0, three signed shorts after the six base fields.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x10), static_cast<byte_t>(0x20), static_cast<byte_t>(0x30), static_cast<byte_t>(0x40),
		static_cast<byte_t>(0x80), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x7F), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x01), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x7F),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet23 Alpha payload");
	auto decoded = headless::decodePacketData<Packet23VehicleSpawn>(expected);
	ctx.checkEqual(decoded->type, -128, "Packet23 readByte sign extension");
	ctx.checkEqual(decoded->field_28047_e, -32768, "Packet23 first optional short");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet23 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 6, "Packet23 literal Alpha precedence result");
	ctx.checkEqual(packet.getPacketId(), 23, "Packet23 id");
	checkFramedId(ctx, packet, 23);

	// Alpha lines 31-36 and 46-51 omit velocity shorts for non-positive flags.
	packet.field_28044_i = -1;
	const std::vector<byte_t> noVelocity{
		static_cast<byte_t>(0x10), static_cast<byte_t>(0x20), static_cast<byte_t>(0x30), static_cast<byte_t>(0x40),
		static_cast<byte_t>(0x80), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x7F), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), noVelocity, "Packet23 non-positive flag payload");
	auto noVelocityDecoded = headless::decodePacketData<Packet23VehicleSpawn>(noVelocity);
	headless::checkPacketBytes(ctx, headless::encodePacketData(*noVelocityDecoded), noVelocity,
		"Packet23 non-positive decode/re-encode");
	packet.field_28044_i = (std::numeric_limits<int_t>::max)();
	ctx.checkEqual(packet.getPacketSize(), 0, "Packet23 Java int overflow in size expression");
}

HEADLESS_TEST(packet, packet24_mob_spawn_alpha_wire_and_metadata)
{
	Packet24MobSpawn packet;
	packet.entityId = 0x01020304;
	packet.type = static_cast<byte_t>(0xA6);
	packet.xPosition = -33;
	packet.yPosition = (std::numeric_limits<int_t>::max)();
	packet.zPosition = (std::numeric_limits<int_t>::min)();
	packet.yaw = -64;
	packet.pitch = 64;
	packet.receivedMetadata.push_back(std::make_shared<WatchableObject>(0, 31, byte_t(-128)));
	packet.receivedMetadata.push_back(std::make_shared<WatchableObject>(1, 1, short_t(-2)));
	packet.receivedMetadata.push_back(std::make_shared<WatchableObject>(2, 2, int_t(0x12345678)));
	packet.receivedMetadata.push_back(std::make_shared<WatchableObject>(3, 3, -1.5f));
	packet.receivedMetadata.push_back(std::make_shared<WatchableObject>(4, 4, jstring(u"A\u03A9")));
	packet.receivedMetadata.push_back(std::make_shared<WatchableObject>(5, 5, ItemStack(0x1234, -1, -32768)));
	packet.receivedMetadata.push_back(std::make_shared<WatchableObject>(6, 6,
		ChunkCoordinates(-1, 0x01020304, (std::numeric_limits<int_t>::min)())));

	// Alpha Packet24MobSpawn.java:56-64 writes the seven spawn fields before
	// DataWatcher.java:75-112 metadata entries and the 0x7F sentinel.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x01), static_cast<byte_t>(0x02), static_cast<byte_t>(0x03), static_cast<byte_t>(0x04),
		static_cast<byte_t>(0xA6), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xDF), static_cast<byte_t>(0x7F), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0xC0), static_cast<byte_t>(0x40), static_cast<byte_t>(0x1F),
		static_cast<byte_t>(0x80), static_cast<byte_t>(0x21), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFE),
		static_cast<byte_t>(0x42), static_cast<byte_t>(0x12), static_cast<byte_t>(0x34), static_cast<byte_t>(0x56),
		static_cast<byte_t>(0x78), static_cast<byte_t>(0x63), static_cast<byte_t>(0xBF), static_cast<byte_t>(0xC0),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x84), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x02), static_cast<byte_t>(0x00), static_cast<byte_t>(0x41), static_cast<byte_t>(0x03),
		static_cast<byte_t>(0xA9), static_cast<byte_t>(0xA5), static_cast<byte_t>(0x12), static_cast<byte_t>(0x34),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0xC6),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0x01), static_cast<byte_t>(0x02), static_cast<byte_t>(0x03), static_cast<byte_t>(0x04),
		static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x7F)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet24 Alpha payload");
	auto decoded = headless::decodePacketData<Packet24MobSpawn>(expected);
	ctx.checkEqual(decoded->type, -90, "Packet24 signed mob type");
	ctx.checkEqual(static_cast<long long>(decoded->getMetadata().size()), 7, "Packet24 metadata count");
	ctx.checkEqual(decoded->getMetadata()[0]->getByte(), -128, "Packet24 byte metadata");
	ctx.checkEqual(decoded->getMetadata()[1]->getShort(), -2, "Packet24 short metadata");
	ctx.checkEqual(decoded->getMetadata()[2]->getInt(), 0x12345678, "Packet24 int metadata");
	ctx.checkEqualBits(decoded->getMetadata()[3]->getFloat(), -1.5f, "Packet24 float metadata");
	ctx.checkEqual(decoded->getMetadata()[4]->getString(), jstring(u"A\u03A9"), "Packet24 string metadata");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet24 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 20, "Packet24 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 24, "Packet24 id");
	checkFramedId(ctx, packet, 24);

	Packet24MobSpawn emptyMetadata;
	// Alpha Packet24MobSpawn.java:56-64 and DataWatcher.java:68-72 emit
	// the metadata sentinel immediately after the 19-byte spawn prefix.
	const std::vector<byte_t> emptyExpected{
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x7F)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(emptyMetadata), emptyExpected,
		"Packet24 empty metadata sentinel");
	auto emptyDecoded = headless::decodePacketData<Packet24MobSpawn>(emptyExpected);
	ctx.check(emptyDecoded->getMetadata().empty(), "Packet24 empty metadata list");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*emptyDecoded), emptyExpected,
		"Packet24 empty metadata decode/re-encode");

	Level level(u"packet24-constructor", Dimension::Id_Normal, 24LL);
	Pig pig(level);
	pig.entityId = 99;
	pig.x = -0.01;
	pig.y = 2.03125;
	pig.z = -1.0;
	pig.yRot = 270.0f;
	pig.xRot = -90.0f;
	Packet24MobSpawn constructed(pig);
	ctx.checkEqual(constructed.type, 90, "Packet24 EntityList id cast");
	ctx.checkEqual(constructed.xPosition, -1, "Packet24 Java floor x");
	ctx.checkEqual(constructed.yPosition, 65, "Packet24 Java floor y");
	ctx.checkEqual(constructed.zPosition, -32, "Packet24 Java floor z");
	ctx.checkEqual(constructed.yaw, -64, "Packet24 Java float-to-byte yaw");
	ctx.checkEqual(constructed.pitch, -64, "Packet24 Java float-to-byte pitch");
}

HEADLESS_TEST(packet, packet25_painting_alpha_wire)
{
	Packet25EntityPainting packet;
	packet.entityId = 0x11223344;
	packet.title = u"A\u03A9";
	packet.xPosition = -1;
	packet.yPosition = 0x01020304;
	packet.zPosition = (std::numeric_limits<int_t>::min)();
	packet.direction = 3;

	// Alpha Packet25EntityPainting.java:46-52 writes entity id, UTF-16 title,
	// three position ints, then the direction int.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x11), static_cast<byte_t>(0x22), static_cast<byte_t>(0x33), static_cast<byte_t>(0x44),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x02), static_cast<byte_t>(0x00), static_cast<byte_t>(0x41),
		static_cast<byte_t>(0x03), static_cast<byte_t>(0xA9), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF), static_cast<byte_t>(0x01), static_cast<byte_t>(0x02),
		static_cast<byte_t>(0x03), static_cast<byte_t>(0x04), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x03)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet25 Alpha payload");
	auto decoded = headless::decodePacketData<Packet25EntityPainting>(expected);
	ctx.checkEqual(decoded->title, jstring(u"A\u03A9"), "Packet25 UTF-16 title");
	ctx.checkEqual(decoded->zPosition, (std::numeric_limits<int_t>::min)(), "Packet25 negative z boundary");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet25 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 24, "Packet25 literal Alpha size");
	ctx.checkEqual(packet.getPacketId(), 25, "Packet25 id");
	checkFramedId(ctx, packet, 25);

	Level level(u"packet25-constructor", Dimension::Id_Normal, 25LL);
	Painting painting(level);
	painting.entityId = 101;
	painting.xTile = -10;
	painting.yTile = 64;
	painting.zTile = 20;
	painting.dir = 3;
	painting.motive = Painting::Motive::SkullAndRoses;
	Packet25EntityPainting constructed(painting);
	ctx.checkEqual(constructed.entityId, 101, "Packet25 constructor entity id");
	ctx.checkEqual(constructed.title, jstring(u"SkullAndRoses"), "Packet25 constructor motive title");
	ctx.checkEqual(constructed.xPosition, -10, "Packet25 constructor x");
	ctx.checkEqual(constructed.direction, 3, "Packet25 constructor direction");
}

HEADLESS_TEST(packet, packet27_position_alpha_wire)
{
	Packet27Position packet;
	packet.field_22039_a = -1.5f;
	packet.field_22038_b = 0.0f;
	packet.field_22041_e = (std::numeric_limits<float>::infinity)();
	packet.field_22040_f = -0.0f;
	packet.field_22043_c = true;
	packet.field_22042_d = false;

	// Alpha CFR Packet27Position.java:32-38 writes four floats in field order,
	// followed by two one-byte booleans.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0xBF), static_cast<byte_t>(0xC0), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x7F), static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00),
		static_cast<byte_t>(0x01), static_cast<byte_t>(0x00)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet27 Alpha payload");
	auto decoded = headless::decodePacketData<Packet27Position>(expected);
	ctx.checkEqualBits(decoded->field_22039_a, -1.5f, "Packet27 first float");
	ctx.checkEqualBits(decoded->field_22040_f, -0.0f, "Packet27 signed zero");
	ctx.check(decoded->field_22043_c, "Packet27 first boolean");
	ctx.check(!decoded->field_22042_d, "Packet27 second boolean");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet27 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 18, "Packet27 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 27, "Packet27 id");
	checkFramedId(ctx, packet, 27);
}

HEADLESS_TEST(packet, packet28_velocity_alpha_wire)
{
	Packet28 packet;
	packet.entityId = static_cast<int_t>(0x89ABCDEFu);
	packet.motionX = -32768;
	packet.motionY = 32767;
	packet.motionZ = -1;

	// Alpha CFR Packet28.java:60-64 writes entity int followed by three signed shorts.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x89), static_cast<byte_t>(0xAB), static_cast<byte_t>(0xCD), static_cast<byte_t>(0xEF),
		static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x7F), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet28 Alpha payload");
	auto decoded = headless::decodePacketData<Packet28>(expected);
	ctx.checkEqual(decoded->entityId, static_cast<int_t>(0x89ABCDEFu), "Packet28 entity id");
	ctx.checkEqual(decoded->motionX, -32768, "Packet28 signed x short");
	ctx.checkEqual(decoded->motionY, 32767, "Packet28 signed y short");
	ctx.checkEqual(decoded->motionZ, -1, "Packet28 signed z short");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet28 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 10, "Packet28 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 28, "Packet28 id");
	checkFramedId(ctx, packet, 28);

	Packet28 constructed(44, -4.0, 3.999, -0.0001875);
	ctx.checkEqual(constructed.motionX, -31200, "Packet28 lower clamp before scaling");
	ctx.checkEqual(constructed.motionY, 31200, "Packet28 upper clamp before scaling");
	ctx.checkEqual(constructed.motionZ, -1, "Packet28 Java double-to-int truncation");

	Level level(u"packet28-constructor", Dimension::Id_Normal, 28LL);
	EntityItem entity(level);
	entity.entityId = 45;
	entity.xd = 1.25;
	entity.yd = -2.5;
	entity.zd = 4.5;
	Packet28 fromEntity(entity);
	ctx.checkEqual(fromEntity.entityId, 45, "Packet28 entity constructor id");
	ctx.checkEqual(fromEntity.motionX, 10000, "Packet28 entity constructor x");
	ctx.checkEqual(fromEntity.motionY, -20000, "Packet28 entity constructor y");
	ctx.checkEqual(fromEntity.motionZ, 31200, "Packet28 entity constructor clamped z");
}

HEADLESS_TEST(packet, packet29_destroy_entity_alpha_wire)
{
	Packet29DestroyEntity packet;
	packet.entityId = static_cast<int_t>(0x80000001u);

	// Alpha Packet29DestroyEntity.java:20-21 writes exactly one signed int.
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x80), static_cast<byte_t>(0x00), static_cast<byte_t>(0x00), static_cast<byte_t>(0x01)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected, "Packet29 Alpha payload");
	auto decoded = headless::decodePacketData<Packet29DestroyEntity>(expected);
	ctx.checkEqual(decoded->entityId, static_cast<int_t>(0x80000001u), "Packet29 entity id");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected, "Packet29 decode/re-encode");
	ctx.checkEqual(packet.getPacketSize(), 4, "Packet29 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 29, "Packet29 id");
	checkFramedId(ctx, packet, 29);
}
