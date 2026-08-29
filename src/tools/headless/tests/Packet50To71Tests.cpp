#include <initializer_list>
#include <vector>

#include "network/Packet50PreChunk.h"
#include "network/Packet51MapChunk.h"
#include "network/Packet52MultiBlockChange.h"
#include "network/Packet53BlockChange.h"
#include "network/Packet54PlayNoteBlock.h"
#include "network/Packet60.h"
#include "network/Packet61DoorChange.h"
#include "network/Packet62Sound.h"
#include "network/Packet63Digging.h"
#include "network/Packet70Bed.h"
#include "network/Packet71Weather.h"
#include "tools/headless/PacketTestUtils.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/Entity.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"

namespace
{
std::vector<byte_t> literalBytes(std::initializer_list<int> values)
{
	std::vector<byte_t> bytes;
	bytes.reserve(values.size());
	for (int value : values)
		bytes.push_back(static_cast<byte_t>(value));
	return bytes;
}

void checkPacketIdentity(headless::TestContext& ctx, Packet& packet, int id, int size)
{
	ctx.checkEqual(packet.getPacketId(), id, "packet id");
	ctx.checkEqual(packet.getPacketSize(), size, "Alpha packet size");
	const std::vector<byte_t> framed = headless::encodeFramedPacket(packet);
	ctx.check(!framed.empty(), "framed packet must contain its id");
	if (!framed.empty())
		ctx.checkEqual(static_cast<ubyte_t>(framed.front()), id, "framed packet id byte");
}
}

HEADLESS_TEST(packet, packet50_pre_chunk_wire)
{
	Packet50PreChunk packet;
	packet.xPosition = 0x01020304;
	packet.yPosition = -2;
	packet.mode = true;

	// Alpha Packet50PreChunk.java CFR 24-27; Vineflower 24-27.
	const std::vector<byte_t> fixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF, 0xFE, 0x01
	});
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), fixture,
		"Packet50 exact payload");
	std::unique_ptr<Packet50PreChunk> decoded =
		headless::decodePacketData<Packet50PreChunk>(fixture);
	ctx.checkEqual(decoded->xPosition, 0x01020304, "Packet50 x");
	ctx.checkEqual(decoded->yPosition, -2, "Packet50 y");
	ctx.check(decoded->mode, "Packet50 mode");
	ctx.check(!decoded->isChunkDataPacket, "Packet50 chunk flag");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet50 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 50, 9);
}

HEADLESS_TEST(packet, packet51_map_chunk_zero_and_nonzero_wire)
{
	// Alpha Packet51MapChunk.java CFR 29-43,53-61; Vineflower 25-40,49-57.
	const std::vector<byte_t> zeroFixture = literalBytes({
		0x10, 0x20, 0x30, 0x40, 0xFF, 0xFE, 0xFE, 0xFD, 0xFC, 0xFC,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	});
	std::unique_ptr<Packet51MapChunk> zero =
		headless::decodePacketData<Packet51MapChunk>(zeroFixture);
	ctx.checkEqual(zero->xPosition, 0x10203040, "Packet51 zero x");
	ctx.checkEqual(zero->yPosition, -2, "Packet51 zero y");
	ctx.checkEqual(zero->zPosition, -0x01020304, "Packet51 zero z");
	ctx.checkEqual(zero->xSize, 1, "Packet51 zero x size");
	ctx.checkEqual(zero->ySize, 1, "Packet51 zero y size");
	ctx.checkEqual(zero->zSize, 1, "Packet51 zero z size");
	ctx.check(zero->chunk == literalBytes({0x00, 0x00}),
		"Packet51 empty compressed input leaves Java's zero-filled output");
	ctx.check(zero->isChunkDataPacket, "Packet51 chunk flag");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*zero), zeroFixture,
		"Packet51 zero decode/re-encode");
	checkPacketIdentity(ctx, *zero, 51, 17);

	const std::vector<byte_t> nonzeroFixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFF, 0xFE, 0xFE, 0xFD, 0xFC, 0xFC,
		0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x17,
		0x78, 0x9C, 0x63, 0x60, 0x64, 0x62, 0x66, 0x61, 0x65, 0x63,
		0xE7, 0xE0, 0xE4, 0xE2, 0xE6, 0xE1, 0xE5, 0x03, 0x00, 0x02,
		0x3F, 0x00, 0x6A
	});
	std::unique_ptr<Packet51MapChunk> nonzero =
		headless::decodePacketData<Packet51MapChunk>(nonzeroFixture);
	ctx.checkEqual(nonzero->xPosition, 0x01020304, "Packet51 nonzero x");
	ctx.checkEqual(nonzero->yPosition, -2, "Packet51 nonzero y");
	ctx.checkEqual(nonzero->zPosition, -0x01020304, "Packet51 nonzero z");
	ctx.checkEqual(nonzero->xSize, 1, "Packet51 nonzero x size");
	ctx.checkEqual(nonzero->ySize, 2, "Packet51 nonzero y size");
	ctx.checkEqual(nonzero->zSize, 3, "Packet51 nonzero z size");
	ctx.check(nonzero->chunk == literalBytes({
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E
	}), "Packet51 exact decompressed bytes");
	checkPacketIdentity(ctx, *nonzero, 51, 17);

	// Alpha never assigns private chunkSize during read. Its decoded re-encode
	// therefore emits the same header with a zero compressed length.
	const std::vector<byte_t> alphaReencode = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFF, 0xFE, 0xFE, 0xFD, 0xFC, 0xFC,
		0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00
	});
	headless::checkPacketBytes(ctx, headless::encodePacketData(*nonzero), alphaReencode,
		"Packet51 Alpha post-read re-encode asymmetry");
}

HEADLESS_TEST(packet, packet52_multi_block_change_zero_and_nonzero_wire)
{
	// Alpha Packet52MultiBlockChange.java CFR 25-47; Vineflower 21-48.
	const std::vector<byte_t> zeroFixture = literalBytes({
		0x10, 0x20, 0x30, 0x40, 0xFE, 0xFD, 0xFC, 0xFC, 0x00, 0x00
	});
	std::unique_ptr<Packet52MultiBlockChange> zero =
		headless::decodePacketData<Packet52MultiBlockChange>(zeroFixture);
	ctx.checkEqual(zero->size, 0, "Packet52 zero count");
	ctx.check(zero->coordinateArray.empty(), "Packet52 zero coordinates");
	ctx.check(zero->typeArray.empty(), "Packet52 zero types");
	ctx.check(zero->metadataArray.empty(), "Packet52 zero metadata");
	ctx.check(zero->isChunkDataPacket, "Packet52 chunk flag");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*zero), zeroFixture,
		"Packet52 zero decode/re-encode");
	checkPacketIdentity(ctx, *zero, 52, 10);

	const std::vector<byte_t> fixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFE, 0xFD, 0xFC, 0xFC, 0x00, 0x02,
		0x1A, 0x2B, 0xFE, 0xDC, 0x80, 0x7F, 0xFF, 0x01
	});
	std::unique_ptr<Packet52MultiBlockChange> decoded =
		headless::decodePacketData<Packet52MultiBlockChange>(fixture);
	ctx.checkEqual(decoded->size, 2, "Packet52 count");
	ctx.check(decoded->coordinateArray == std::vector<short_t>({0x1A2B,
		static_cast<short_t>(0xFEDC)}), "Packet52 packed coordinates");
	ctx.check(decoded->typeArray == literalBytes({0x80, 0x7F}), "Packet52 type array");
	ctx.check(decoded->metadataArray == literalBytes({0xFF, 0x01}),
		"Packet52 metadata array");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet52 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 52, 18);
}

HEADLESS_TEST(packet, packet53_block_change_wire)
{
	Packet53BlockChange packet;
	packet.xPosition = 0x01020304;
	packet.yPosition = 0xFE;
	packet.zPosition = -0x01020304;
	packet.type = 0x80;
	packet.metadata = 0xFF;

	// Alpha Packet53BlockChange.java CFR 24-37; Vineflower 20-34.
	const std::vector<byte_t> fixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFE, 0xFE, 0xFD, 0xFC, 0xFC, 0x80, 0xFF
	});
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), fixture,
		"Packet53 exact payload");
	std::unique_ptr<Packet53BlockChange> decoded =
		headless::decodePacketData<Packet53BlockChange>(fixture);
	ctx.checkEqual(decoded->yPosition, 254, "Packet53 read() y is unsigned int");
	ctx.checkEqual(decoded->type, 128, "Packet53 read() type is unsigned int");
	ctx.checkEqual(decoded->metadata, 255, "Packet53 read() metadata is unsigned int");
	ctx.check(decoded->isChunkDataPacket, "Packet53 chunk flag");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet53 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 53, 11);
}

HEADLESS_TEST(packet, packet54_note_block_wire)
{
	Packet54PlayNoteBlock packet;
	packet.xLocation = 0x01020304;
	packet.yLocation = -2;
	packet.zLocation = -0x01020304;
	packet.instrumentType = 0xFE;
	packet.pitch = 0x80;

	// Alpha Packet54PlayNoteBlock.java CFR 21-35; Vineflower 19-33.
	const std::vector<byte_t> fixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFF, 0xFE, 0xFE, 0xFD, 0xFC, 0xFC, 0xFE, 0x80
	});
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), fixture,
		"Packet54 exact payload");
	std::unique_ptr<Packet54PlayNoteBlock> decoded =
		headless::decodePacketData<Packet54PlayNoteBlock>(fixture);
	ctx.checkEqual(decoded->yLocation, -2, "Packet54 signed short y");
	ctx.checkEqual(decoded->instrumentType, 254, "Packet54 read() instrument");
	ctx.checkEqual(decoded->pitch, 128, "Packet54 read() pitch");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet54 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 54, 12);
}

HEADLESS_TEST(packet, packet60_explosion_zero_and_nonzero_wire)
{
	// Alpha Packet60.java CFR 23-57; Vineflower 21-58.
	const std::vector<byte_t> zeroFixture = literalBytes({
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	});
	std::unique_ptr<Packet60> zero = headless::decodePacketData<Packet60>(zeroFixture);
	ctx.check(zero->field_12237_e.empty(), "Packet60 zero record set");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*zero), zeroFixture,
		"Packet60 zero decode/re-encode");
	checkPacketIdentity(ctx, *zero, 60, 32);

	const std::vector<byte_t> fixture = literalBytes({
		0x3F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xC0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x80, 0x7F, 0xFF
	});
	std::unique_ptr<Packet60> decoded = headless::decodePacketData<Packet60>(fixture);
	ctx.checkEqualBits(decoded->field_12236_a, 1.5, "Packet60 x double");
	ctx.checkEqualBits(decoded->field_12235_b, -2.25, "Packet60 y double");
	ctx.checkEqualBits(decoded->field_12239_c, 3.25, "Packet60 z double");
	ctx.checkEqualBits(decoded->field_12238_d, 4.5f, "Packet60 radius float");
	ctx.checkEqual(decoded->field_12237_e.size(), 1, "Packet60 record count");
	ctx.check(decoded->field_12237_e.count(ChunkCoordinates(-127, 125, 2)) == 1,
		"Packet60 signed relative coordinates");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet60 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 60, 35);
}

HEADLESS_TEST(packet, packet61_door_change_wire)
{
	Packet61DoorChange packet;
	packet.field_28050_a = 0x01020304;
	packet.field_28053_c = -0x01020304;
	packet.field_28052_d = -128;
	packet.field_28051_e = 0x11223344;
	packet.field_28049_b = static_cast<int_t>(0xDEADBEEF);

	// Alpha Packet61DoorChange.java CFR 21-35; Vineflower 19-33.
	const std::vector<byte_t> fixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFE, 0xFD, 0xFC, 0xFC, 0x80,
		0x11, 0x22, 0x33, 0x44, 0xDE, 0xAD, 0xBE, 0xEF
	});
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), fixture,
		"Packet61 exact payload");
	std::unique_ptr<Packet61DoorChange> decoded =
		headless::decodePacketData<Packet61DoorChange>(fixture);
	ctx.checkEqual(decoded->field_28052_d, -128, "Packet61 signed y byte");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet61 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 61, 20);
}

HEADLESS_TEST(packet, packet62_sound_wire)
{
	// Alpha Packet62Sound.java CFR 22-28; Vineflower 20-26. Alpha writePacketData
	// is empty (CFR 31-33; Vineflower 29-31), so this exact fixture is receive-only.
	const std::vector<byte_t> fixture = literalBytes({
		0x00, 0x0B, 0x41, 0xC0, 0x80, 0xC2, 0xA2, 0xED, 0xA0, 0xBD,
		0xED, 0xB8, 0x80,
		0x3F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xC0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x90, 0x00, 0x00, 0xBF, 0x40, 0x00, 0x00
	});
	std::unique_ptr<Packet62Sound> decoded =
		headless::decodePacketData<Packet62Sound>(fixture);
	jstring expectedSound;
	expectedSound.push_back(u'A');
	expectedSound.push_back(u'\0');
	expectedSound.push_back(static_cast<char16_t>(0x00A2));
	expectedSound.push_back(static_cast<char16_t>(0xD83D));
	expectedSound.push_back(static_cast<char16_t>(0xDE00));
	ctx.checkEqual(decoded->sound, expectedSound, "Packet62 modified UTF-8 sound");
	ctx.checkEqualBits(decoded->locX, 1.5, "Packet62 x double");
	ctx.checkEqualBits(decoded->locY, -2.25, "Packet62 y double");
	ctx.checkEqualBits(decoded->locZ, 3.25, "Packet62 z double");
	ctx.checkEqualBits(decoded->f, 4.5f, "Packet62 volume float");
	ctx.checkEqualBits(decoded->f1, -0.75f, "Packet62 pitch float");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), {},
		"Packet62 Alpha empty write");
	checkPacketIdentity(ctx, *decoded, 62, 37);
}

HEADLESS_TEST(packet, packet63_digging_wire)
{
	// Alpha Packet63Digging.java CFR 22-33; Vineflower 20-31. Alpha
	// writePacketData is empty, so this exact fixture is receive-only.
	const std::vector<byte_t> fixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF, 0xFE,
		0xFE, 0xFD, 0xFC, 0xFC, 0x80, 0x3F, 0x00, 0x00, 0x00
	});
	std::unique_ptr<Packet63Digging> decoded =
		headless::decodePacketData<Packet63Digging>(fixture);
	ctx.checkEqual(decoded->x, 0x01020304, "Packet63 x");
	ctx.checkEqual(decoded->y, -2, "Packet63 y");
	ctx.checkEqual(decoded->z, -0x01020304, "Packet63 z");
	ctx.checkEqual(decoded->face, -128, "Packet63 signed face byte");
	ctx.checkEqualBits(decoded->progress, 0.5f, "Packet63 progress");
	ctx.check(decoded->timestamp > 0, "Packet63 read timestamp");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), {},
		"Packet63 Alpha empty write");
	checkPacketIdentity(ctx, *decoded, 63, 17);
}

HEADLESS_TEST(packet, packet70_bed_wire)
{
	Packet70Bed packet;
	packet.field_25019_b = -1;

	// Alpha Packet70Bed.java CFR 17-24; Vineflower 15-22.
	const std::vector<byte_t> fixture = literalBytes({0xFF});
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), fixture,
		"Packet70 exact payload");
	std::unique_ptr<Packet70Bed> decoded = headless::decodePacketData<Packet70Bed>(fixture);
	ctx.checkEqual(decoded->field_25019_b, -1, "Packet70 signed reason byte");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet70 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 70, 1);
}

HEADLESS_TEST(packet, packet71_weather_wire_and_entity_constructor)
{
	Packet71Weather packet;
	packet.field_27054_a = 0x01020304;
	packet.field_27055_e = -1;
	packet.field_27053_b = -0x01020304;
	packet.field_27057_c = 0x11223344;
	packet.field_27056_d = static_cast<int_t>(0xDEADBEEF);

	// Alpha Packet71Weather.java CFR 33-47; Vineflower 27-41.
	const std::vector<byte_t> fixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0xFF, 0xFE, 0xFD, 0xFC, 0xFC,
		0x11, 0x22, 0x33, 0x44, 0xDE, 0xAD, 0xBE, 0xEF
	});
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), fixture,
		"Packet71 exact payload");
	std::unique_ptr<Packet71Weather> decoded =
		headless::decodePacketData<Packet71Weather>(fixture);
	ctx.checkEqual(decoded->field_27055_e, -1, "Packet71 signed weather byte");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), fixture,
		"Packet71 decode/re-encode");
	checkPacketIdentity(ctx, *decoded, 71, 17);

	// Alpha Packet71Weather.java CFR 25-29; Vineflower 18-23.
	headless::initGameRegistries();
	Level level(u"packet71-constructor", Dimension::Id_Normal, 1LL);
	Entity entity(level);
	entity.entityId = 0x01020304;
	entity.x = -0.01;
	entity.y = 1.999;
	entity.z = -1.0;
	Packet71Weather fromEntity(entity);
	ctx.checkEqual(fromEntity.field_27054_a, 0x01020304, "Packet71 constructor entity id");
	ctx.checkEqual(fromEntity.field_27053_b, -1, "Packet71 constructor floor x*32");
	ctx.checkEqual(fromEntity.field_27057_c, 63, "Packet71 constructor floor y*32");
	ctx.checkEqual(fromEntity.field_27056_d, -32, "Packet71 constructor floor z*32");
	ctx.checkEqual(fromEntity.field_27055_e, 0, "Packet71 constructor default weather type");
	const std::vector<byte_t> constructorFixture = literalBytes({
		0x01, 0x02, 0x03, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xE0
	});
	headless::checkPacketBytes(ctx, headless::encodePacketData(fromEntity), constructorFixture,
		"Packet71 Entity constructor payload");
}
