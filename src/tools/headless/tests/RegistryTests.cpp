// Layer 2: registry parity.
//
// The reference tables below are transcribed from the Alpha 1.2.6
// decompilation and were verified identical in the CFR and Vineflower output:
//   Packet.java:190-248        packet id, clientbound, serverbound
//   EntityList.java:106-127    entity save name and numeric id
//   TileEntity.java:90-93      tile entity save name
//
// These tests exist so a future edit cannot quietly reintroduce a direction or
// name mismatch.

#include <set>
#include <sstream>
#include <string>

#include "network/Packet.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/Entity.h"
#include "world/entity/EntityIO.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/tile/entity/FurnaceTileEntity.h"
#include "world/level/tile/entity/MobSpawnerTileEntity.h"
#include "world/level/tile/entity/SignTileEntity.h"

struct AlphaPacket
{
	int id;
	bool clientbound;
	bool serverbound;
};

static const AlphaPacket alphaPackets[] = {
	{ 0, true, true },    { 1, true, true },    { 2, true, true },    { 3, true, true },
	{ 4, true, false },   { 5, true, false },   { 6, true, false },   { 7, false, true },
	{ 8, true, false },   { 9, true, true },    { 10, true, true },   { 11, true, true },
	{ 12, true, true },   { 13, true, true },   { 14, false, true },  { 15, false, true },
	{ 16, false, true },  { 17, true, false },  { 18, true, true },   { 19, false, true },
	{ 20, true, false },  { 21, true, false },  { 22, true, false },  { 23, true, false },
	{ 24, true, false },  { 25, true, false },  { 27, false, true },  { 28, true, false },
	{ 29, true, false },  { 30, true, false },  { 31, true, false },  { 32, true, false },
	{ 33, true, false },  { 34, true, false },  { 38, true, false },  { 39, true, false },
	{ 40, true, false },  { 50, true, false },  { 51, true, false },  { 52, true, false },
	{ 53, true, false },  { 54, true, false },  { 60, true, false },  { 61, true, false },
	{ 62, true, false },  { 63, true, false },  { 70, true, false },  { 71, true, false },
	{ 100, true, false }, { 101, true, true },  { 102, false, true }, { 103, true, false },
	{ 104, true, false }, { 105, true, false }, { 106, true, true },  { 130, true, true },
	{ 131, true, false }, { 200, true, false }, { 255, true, true },
};

struct AlphaEntity
{
	const char16_t *name;
	int id;
};

static const AlphaEntity alphaEntities[] = {
	{ u"Arrow", 10 },   { u"Snowball", 11 }, { u"Item", 1 },      { u"Painting", 9 },
	{ u"Mob", 48 },     { u"Monster", 49 },  { u"Creeper", 50 },  { u"Skeleton", 51 },
	{ u"Spider", 52 },  { u"Giant", 53 },    { u"Zombie", 54 },   { u"Slime", 55 },
	{ u"Ghast", 56 },   { u"PigZombie", 57 }, { u"Pig", 90 },     { u"Sheep", 91 },
	{ u"Cow", 92 },     { u"Chicken", 93 },  { u"PrimedTnt", 20 }, { u"FallingSand", 21 },
	{ u"Minecart", 40 }, { u"Boat", 41 },
};

static const int alphaPacketCount = static_cast<int>(sizeof(alphaPackets) / sizeof(alphaPackets[0]));
static const int alphaEntityCount = static_cast<int>(sizeof(alphaEntities) / sizeof(alphaEntities[0]));

HEADLESS_TEST(registry, packet_ids_match_alpha)
{
	headless::initGameRegistries();

	std::set<int> expected;
	for (int i = 0; i < alphaPacketCount; ++i)
		expected.insert(alphaPackets[i].id);

	for (int id : expected)
	{
		if (Packet::packetIdToFactory.find(id) == Packet::packetIdToFactory.end())
			ctx.fail("packet " + std::to_string(id) + " is not registered");
	}

	for (const auto &entry : Packet::packetIdToFactory)
	{
		if (expected.find(entry.first) == expected.end())
			ctx.fail("packet " + std::to_string(entry.first) + " is registered but absent from Alpha");
	}

	ctx.checkEqual(static_cast<long long>(Packet::packetIdToFactory.size()),
		alphaPacketCount, "registered packet count");
}

HEADLESS_TEST(registry, packet_directions_match_alpha)
{
	headless::initGameRegistries();

	for (int i = 0; i < alphaPacketCount; ++i)
	{
		const AlphaPacket &entry = alphaPackets[i];
		const bool clientbound = Packet::clientPacketIdList.count(entry.id) != 0;
		const bool serverbound = Packet::serverPacketIdList.count(entry.id) != 0;

		if (clientbound != entry.clientbound || serverbound != entry.serverbound)
		{
			std::ostringstream text;
			text << "packet " << entry.id << ": expected clientbound=" << entry.clientbound
				<< " serverbound=" << entry.serverbound
				<< ", got clientbound=" << clientbound << " serverbound=" << serverbound;
			ctx.fail(text.str());
		}
	}
}

HEADLESS_TEST(registry, entity_save_names_and_ids_match_alpha)
{
	headless::initGameRegistries();

	Level level(u"registry-entity-ids", Dimension::Id_Normal, 1234LL);

	for (int i = 0; i < alphaEntityCount; ++i)
	{
		const AlphaEntity &entry = alphaEntities[i];
		const jstring name = entry.name;

		std::shared_ptr<Entity> byName = EntityIO::newEntity(name, level);
		if (!ctx.check(byName != nullptr, "no entity registered for name " + String::toUTF8(name)))
			continue;

		ctx.checkEqual(byName->getEncodeId(), name,
			"save identity of entity created from name " + String::toUTF8(name));
		ctx.checkEqual(EntityIO::getEncodeNumericId(*byName), entry.id,
			"numeric id of entity created from name " + String::toUTF8(name));

		std::shared_ptr<Entity> byId = EntityIO::createEntity(entry.id, level);
		if (!ctx.check(byId != nullptr, "no entity registered for id " + std::to_string(entry.id)))
			continue;

		ctx.checkEqual(byId->getEncodeId(), name,
			"save identity of entity created from id " + std::to_string(entry.id));
	}
}

HEADLESS_TEST(registry, tile_entity_save_names_match_alpha)
{
	headless::initGameRegistries();

	ChestTileEntity chest;
	FurnaceTileEntity furnace;
	SignTileEntity sign;
	MobSpawnerTileEntity spawner;

	ctx.checkEqual(chest.getEncodeId(), jstring(u"Chest"), "chest tile entity save name");
	ctx.checkEqual(furnace.getEncodeId(), jstring(u"Furnace"), "furnace tile entity save name");
	ctx.checkEqual(sign.getEncodeId(), jstring(u"Sign"), "sign tile entity save name");
	ctx.checkEqual(spawner.getEncodeId(), jstring(u"MobSpawner"), "mob spawner tile entity save name");
}
