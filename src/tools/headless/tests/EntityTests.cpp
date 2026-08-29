// Layer 3: entity save identity and NBT round trips.
//
// Alpha reference:
//   Entity.java:642-648        writes the registry string as "id", refuses to
//                              save when the class has no registry entry
//   Entity.java:687-689        getEntityString is final and delegates to the
//                              registry, so identity follows the exact class
//   EntityList.java:61-78      createEntityFromNBT looks the string up again
//
// The point of these tests is semantic identity: a saved Pig has to come back
// as a Pig, not merely parse.

#include <memory>
#include <string>

#include "nbt/CompoundTag.h"
#include "world/item/Items.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/Entity.h"
#include "world/entity/EntityIO.h"
#include "world/entity/Mob.h"
#include "world/entity/Painting.h"
#include "world/entity/PrimedTnt.h"
#include "world/entity/animal/Chicken.h"
#include "world/entity/animal/Cow.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/animal/Sheep.h"
#include "world/entity/item/Boat.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/item/FallingTile.h"
#include "world/entity/item/Minecart.h"
#include "world/entity/monster/Creeper.h"
#include "world/entity/monster/Ghast.h"
#include "world/entity/monster/Giant.h"
#include "world/entity/monster/Monster.h"
#include "world/entity/monster/PigZombie.h"
#include "world/entity/monster/Skeleton.h"
#include "world/entity/monster/Slime.h"
#include "world/entity/monster/Spider.h"
#include "world/entity/monster/Zombie.h"
#include "world/entity/player/Player.h"
#include "world/entity/projectile/Arrow.h"
#include "world/entity/projectile/Snowball.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"

HEADLESS_TEST(entities, every_registered_type_reports_its_alpha_identity)
{
	headless::initGameRegistries();
	Level level(u"entity-identity", Dimension::Id_Normal, 5LL);

	struct Expectation
	{
		std::shared_ptr<Entity> entity;
		const char16_t *id;
	};

	const Expectation expectations[] = {
		{ std::make_shared<Mob>(level), u"Mob" },
		{ std::make_shared<Monster>(level), u"Monster" },
		{ std::make_shared<Creeper>(level), u"Creeper" },
		{ std::make_shared<Skeleton>(level), u"Skeleton" },
		{ std::make_shared<Spider>(level), u"Spider" },
		{ std::make_shared<Giant>(level), u"Giant" },
		{ std::make_shared<Zombie>(level), u"Zombie" },
		{ std::make_shared<Slime>(level), u"Slime" },
		{ std::make_shared<Ghast>(level), u"Ghast" },
		{ std::make_shared<PigZombie>(level), u"PigZombie" },
		{ std::make_shared<Pig>(level), u"Pig" },
		{ std::make_shared<Sheep>(level), u"Sheep" },
		{ std::make_shared<Cow>(level), u"Cow" },
		{ std::make_shared<Chicken>(level), u"Chicken" },
		{ std::make_shared<Arrow>(level), u"Arrow" },
		{ std::make_shared<Snowball>(level), u"Snowball" },
		{ std::make_shared<EntityItem>(level), u"Item" },
		{ std::make_shared<Painting>(level), u"Painting" },
		{ std::make_shared<PrimedTnt>(level), u"PrimedTnt" },
		{ std::make_shared<FallingTile>(level), u"FallingSand" },
		{ std::make_shared<Minecart>(level), u"Minecart" },
		{ std::make_shared<Boat>(level), u"Boat" },
	};

	for (const Expectation &entry : expectations)
	{
		const jstring expected = entry.id;
		ctx.checkEqual(entry.entity->getEncodeId(), expected,
			"save identity for " + String::toUTF8(expected));
	}
}

HEADLESS_TEST(entities, unregistered_types_have_no_save_identity)
{
	headless::initGameRegistries();
	Level level(u"entity-unregistered", Dimension::Id_Normal, 5LL);

	// Alpha does not register EntityPlayer, so a player is never written into
	// a chunk's entity list. The player is saved separately in level.dat.
	std::shared_ptr<Player> player = std::make_shared<Player>(level);
	ctx.check(player->getEncodeId().empty(), "a player must not have a chunk save identity");

	CompoundTag tag;
	ctx.check(!player->save(tag), "saving a player as a chunk entity must be refused");
}

HEADLESS_TEST(entities, save_writes_the_registry_id)
{
	headless::initGameRegistries();
	Level level(u"entity-save-id", Dimension::Id_Normal, 5LL);

	std::shared_ptr<Pig> pig = std::make_shared<Pig>(level);
	pig->setPos(1.5, 65.0, 2.5);

	CompoundTag tag;
	if (!ctx.check(pig->save(tag), "a Pig must be saveable"))
		return;

	ctx.checkEqual(tag.getString(u"id"), jstring(u"Pig"), "stored entity id");
}

HEADLESS_TEST(entities, round_trip_preserves_subtype)
{
	headless::initGameRegistries();
	Level level(u"entity-roundtrip", Dimension::Id_Normal, 5LL);

	struct Case
	{
		std::shared_ptr<Entity> entity;
		const char16_t *id;
		// Alpha's EntityPainting.readEntityFromNBT recomputes the position
		// from TileX/TileY/TileZ and Dir instead of restoring Pos
		// (EntityPainting.java:198-212), so Pos parity does not apply there.
		bool positionIsDerived;
	};

	const Case cases[] = {
		{ std::make_shared<Pig>(level), u"Pig", false },
		{ std::make_shared<Sheep>(level), u"Sheep", false },
		{ std::make_shared<Cow>(level), u"Cow", false },
		{ std::make_shared<Chicken>(level), u"Chicken", false },
		{ std::make_shared<Creeper>(level), u"Creeper", false },
		{ std::make_shared<Skeleton>(level), u"Skeleton", false },
		{ std::make_shared<Zombie>(level), u"Zombie", false },
		{ std::make_shared<Spider>(level), u"Spider", false },
		{ std::make_shared<PigZombie>(level), u"PigZombie", false },
		{ std::make_shared<Slime>(level), u"Slime", false },
		{ std::make_shared<Ghast>(level), u"Ghast", false },
		{ std::make_shared<Giant>(level), u"Giant", false },
		{ std::make_shared<EntityItem>(level, 0.0, 0.0, 0.0,
			ItemStack(Items::coal->getShiftedIndex(), 7, 3)), u"Item", false },
		{ std::make_shared<Arrow>(level), u"Arrow", false },
		{ std::make_shared<Snowball>(level), u"Snowball", false },
		{ std::make_shared<Boat>(level), u"Boat", false },
		{ std::make_shared<Minecart>(level), u"Minecart", false },
		{ std::make_shared<Painting>(level), u"Painting", true },
		{ std::make_shared<FallingTile>(level), u"FallingSand", false },
		{ std::make_shared<PrimedTnt>(level), u"PrimedTnt", false },
	};

	for (const Case &entry : cases)
	{
		const jstring expected = entry.id;
		const std::string label = String::toUTF8(expected);

		entry.entity->setPos(3.5, 66.0, -4.5);

		CompoundTag tag;
		if (!ctx.check(entry.entity->save(tag), label + " must be saveable"))
			continue;

		std::shared_ptr<Entity> loaded = EntityIO::loadStatic(tag, level);
		if (!ctx.check(loaded != nullptr, label + " must load back"))
			continue;

		ctx.checkEqual(loaded->getEncodeId(), expected, label + " must load back as the same type");

		if (entry.positionIsDerived)
			continue;

		ctx.checkEqualBits(loaded->x, 3.5, label + " x position round trip");
		ctx.checkEqualBits(loaded->y, 66.0, label + " y position round trip");
		ctx.checkEqualBits(loaded->z, -4.5, label + " z position round trip");
	}
}

HEADLESS_TEST(entities, painting_round_trip_preserves_anchor)
{
	headless::initGameRegistries();
	Level level(u"entity-painting", Dimension::Id_Normal, 5LL);

	// Alpha stores the wall anchor and the direction, then derives the
	// position on load (EntityPainting.java:189-212).
	std::shared_ptr<Painting> painting = std::make_shared<Painting>(level, 12, 70, -30, 2);

	CompoundTag tag;
	if (!ctx.check(painting->save(tag), "a Painting must be saveable"))
		return;

	ctx.checkEqual(tag.getInt(u"TileX"), 12, "stored TileX");
	ctx.checkEqual(tag.getInt(u"TileY"), 70, "stored TileY");
	ctx.checkEqual(tag.getInt(u"TileZ"), -30, "stored TileZ");
	ctx.checkEqual(tag.getByte(u"Dir"), 2, "stored Dir");

	std::shared_ptr<Entity> loaded = EntityIO::loadStatic(tag, level);
	if (!ctx.check(loaded != nullptr, "a Painting must load back"))
		return;

	std::shared_ptr<Painting> reloaded = std::dynamic_pointer_cast<Painting>(loaded);
	if (!ctx.check(reloaded != nullptr, "a Painting must load back as a Painting"))
		return;

	ctx.checkEqual(reloaded->xTile, 12, "TileX round trip");
	ctx.checkEqual(reloaded->yTile, 70, "TileY round trip");
	ctx.checkEqual(reloaded->zTile, -30, "TileZ round trip");
	ctx.checkEqual(reloaded->dir, 2, "Dir round trip");
	ctx.checkEqualBits(reloaded->x, painting->x, "derived x must match the original");
	ctx.checkEqualBits(reloaded->y, painting->y, "derived y must match the original");
	ctx.checkEqualBits(reloaded->z, painting->z, "derived z must match the original");
}

HEADLESS_TEST(entities, dropped_item_round_trip_preserves_stack)
{
	headless::initGameRegistries();
	Level level(u"entity-item-stack", Dimension::Id_Normal, 6LL);
	std::shared_ptr<EntityItem> original = std::make_shared<EntityItem>(
		level, 2.5, 70.0, -3.5,
		ItemStack(Items::coal->getShiftedIndex(), 7, 3));
	original->age = 123;

	CompoundTag tag;
	if (!ctx.check(original->save(tag), "valid dropped item must save"))
		return;
	std::shared_ptr<CompoundTag> savedStack = tag.getCompound(u"Item");
	ctx.checkEqual(savedStack->getShort(u"id"), Items::coal->getShiftedIndex(),
		"nested item id");
	ctx.checkEqual(savedStack->getByte(u"Count"), 7, "nested item count");
	ctx.checkEqual(savedStack->getShort(u"Damage"), 3, "nested item damage");

	std::shared_ptr<EntityItem> loaded = std::dynamic_pointer_cast<EntityItem>(
		EntityIO::loadStatic(tag, level));
	if (!ctx.check(loaded != nullptr, "dropped item must load"))
		return;
	ctx.check(loaded->hasValidItem(), "loaded dropped item is renderable");
	ctx.checkEqual(loaded->item.itemID, Items::coal->getShiftedIndex(), "loaded item id");
	ctx.checkEqual(loaded->item.stackSize, 7, "loaded item count");
	ctx.checkEqual(loaded->item.itemDamage, 3, "loaded item damage");
	ctx.checkEqual(loaded->age, 123, "loaded item age");
}

HEADLESS_TEST(entities, malformed_dropped_item_is_not_renderable)
{
	headless::initGameRegistries();
	Level level(u"entity-item-malformed", Dimension::Id_Normal, 7LL);
	EntityItem original(level, 0.5, 65.0, 0.5,
		ItemStack(Items::coal->getShiftedIndex(), 1));
	CompoundTag tag;
	original.save(tag);
	tag.putCompound(u"Item", std::make_unique<CompoundTag>());

	std::shared_ptr<EntityItem> loaded = std::dynamic_pointer_cast<EntityItem>(
		EntityIO::loadStatic(tag, level));
	if (!ctx.check(loaded != nullptr, "malformed dropped item still decodes safely"))
		return;
	ctx.check(!loaded->hasValidItem(), "malformed dropped item is rejected by renderer invariant");
	loaded->tick();
	ctx.check(loaded->removed, "malformed dropped item removes itself on tick");
}
