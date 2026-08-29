// Layer 5: Alpha InventoryPlayer merging, slot arithmetic and armor lifecycle.
//
// Alpha reference: InventoryPlayer.java:40-54,79-105,125-174,220-243,
// 282-303. Empty C++ ItemStack values represent Java null slots.

#include <memory>

#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "world/entity/player/Player.h"
#include "world/item/Item.h"
#include "world/item/ItemStack.h"
#include "world/item/Items.h"
#include "world/level/Level.h"
#include "world/level/dimension/Dimension.h"

class InventoryTestSubtypeItem : public Item
{
public:
	InventoryTestSubtypeItem() : Item(1000)
	{
		maxDamage = 0;
		hasSubtypes = true;
	}
};

static Item &subtypeItem()
{
	static InventoryTestSubtypeItem item;
	return item;
}

static ItemStack coalStack(int_t count)
{
	return ItemStack(Items::coal->getShiftedIndex(), count, 0);
}

static void fillWithCoal(InventoryPlayer &inventory)
{
	for (ItemStack &slot : inventory.mainInventory)
		slot = coalStack(64);
}

HEADLESS_TEST(inventory, full_merge_into_empty_slot)
{
	headless::initGameRegistries();
	Level level(u"inventory-empty", Dimension::Id_Normal, 1LL);
	Player player(level);

	ItemStack incoming = coalStack(10);
	ctx.check(player.inventory.addItemStackToInventory(incoming), "empty slot merge must report success");
	ctx.checkEqual(incoming.stackSize, 0, "all incoming items must be consumed");
	ctx.checkEqual(player.inventory.mainInventory[0].stackSize, 10, "first slot count");
	ctx.checkEqual(player.inventory.mainInventory[0].popTime, 5, "pickup animation counter");
}

HEADLESS_TEST(inventory, merge_into_partial_stack)
{
	headless::initGameRegistries();
	Level level(u"inventory-partial", Dimension::Id_Normal, 2LL);
	Player player(level);

	player.inventory.mainInventory[0] = coalStack(60);
	ItemStack incoming = coalStack(4);
	ctx.check(player.inventory.addItemStackToInventory(incoming), "partial stack merge must report success");
	ctx.checkEqual(incoming.stackSize, 0, "partial merge remainder");
	ctx.checkEqual(player.inventory.mainInventory[0].stackSize, 64, "partial stack must stop at 64");
}

HEADLESS_TEST(inventory, merge_across_two_partial_stacks)
{
	headless::initGameRegistries();
	Level level(u"inventory-two-partial", Dimension::Id_Normal, 3LL);
	Player player(level);

	player.inventory.mainInventory[0] = coalStack(63);
	player.inventory.mainInventory[1] = coalStack(63);
	ItemStack incoming = coalStack(2);
	ctx.check(player.inventory.addItemStackToInventory(incoming), "two-stack merge must report success");
	ctx.checkEqual(incoming.stackSize, 0, "two-stack merge remainder");
	ctx.checkEqual(player.inventory.mainInventory[0].stackSize, 64, "first partial stack count");
	ctx.checkEqual(player.inventory.mainInventory[1].stackSize, 64, "second partial stack count");
}

HEADLESS_TEST(inventory, partial_progress_then_no_space_terminates)
{
	headless::initGameRegistries();
	Level level(u"inventory-progress-watchdog", Dimension::Id_Normal, 4LL);
	Player player(level);

	fillWithCoal(player.inventory);
	player.inventory.mainInventory[0] = coalStack(63);
	ItemStack incoming = coalStack(2);

	// Before the fix the first iteration moved one item, the second moved none,
	// and every later iteration compared against the ORIGINAL size. This call
	// never returned. Production now has a 37-iteration invariant watchdog, so
	// the same regression would throw and fail this test instead of hanging.
	const bool result = player.inventory.addItemStackToInventory(incoming);
	ctx.check(!result, "Alpha reports the result of the final no-progress iteration");
	ctx.checkEqual(incoming.stackSize, 1, "one item must remain when no slot is free");
	ctx.checkEqual(player.inventory.mainInventory[0].stackSize, 64, "one item must have merged");
}

HEADLESS_TEST(inventory, full_inventory_makes_no_progress)
{
	headless::initGameRegistries();
	Level level(u"inventory-full", Dimension::Id_Normal, 5LL);
	Player player(level);

	fillWithCoal(player.inventory);
	ItemStack incoming = coalStack(1);
	ctx.check(!player.inventory.addItemStackToInventory(incoming), "full inventory must reject the item");
	ctx.checkEqual(incoming.stackSize, 1, "rejected stack must remain unchanged");
}

HEADLESS_TEST(inventory, non_stackable_item_needs_empty_slot)
{
	headless::initGameRegistries();
	Level level(u"inventory-non-stackable", Dimension::Id_Normal, 6LL);
	Player player(level);

	fillWithCoal(player.inventory);
	ItemStack sword(Items::swordSteel->getShiftedIndex(), 1, 0);
	ctx.check(!player.inventory.addItemStackToInventory(sword), "a sword cannot merge into another item");
	ctx.checkEqual(sword.stackSize, 1, "rejected sword must remain intact");
}

HEADLESS_TEST(inventory, subtype_damage_mismatch_does_not_merge)
{
	headless::initGameRegistries();
	Level level(u"inventory-subtypes", Dimension::Id_Normal, 7LL);
	Player player(level);

	Item &item = subtypeItem();
	player.inventory.mainInventory[0] = ItemStack(item.getShiftedIndex(), 10, 0);
	ItemStack incoming(item.getShiftedIndex(), 5, 1);

	ctx.check(player.inventory.addItemStackToInventory(incoming), "different subtype must use an empty slot");
	ctx.checkEqual(player.inventory.mainInventory[0].stackSize, 10, "original subtype count");
	ctx.checkEqual(player.inventory.mainInventory[1].stackSize, 5, "second subtype count");
	ctx.checkEqual(player.inventory.mainInventory[1].itemDamage, 1, "second subtype damage value");
}

HEADLESS_TEST(inventory, stack_limit_is_64)
{
	headless::initGameRegistries();
	Level level(u"inventory-limit", Dimension::Id_Normal, 8LL);
	Player player(level);

	player.inventory.mainInventory[0] = coalStack(63);
	ItemStack incoming = coalStack(2);
	ctx.check(player.inventory.addItemStackToInventory(incoming), "remainder must move to an empty slot");
	ctx.checkEqual(player.inventory.mainInventory[0].stackSize, 64, "first stack limit");
	ctx.checkEqual(player.inventory.mainInventory[1].stackSize, 1, "overflow must use the next slot");
}

HEADLESS_TEST(inventory, damaged_tool_uses_first_empty_slot)
{
	headless::initGameRegistries();
	Level level(u"inventory-damaged-tool", Dimension::Id_Normal, 9LL);
	Player player(level);

	player.inventory.mainInventory[0] = coalStack(1);
	ItemStack tool(Items::pickaxeSteel->getShiftedIndex(), 1, 3);
	ctx.check(player.inventory.addItemStackToInventory(tool), "damaged tool must be inserted");
	ctx.checkEqual(tool.stackSize, 0, "inserted tool source count");
	ctx.checkEqual(player.inventory.mainInventory[1].itemID,
		Items::pickaxeSteel->getShiftedIndex(), "damaged tool destination");
	ctx.checkEqual(player.inventory.mainInventory[1].itemDamage, 3, "damaged tool durability");
}

HEADLESS_TEST(inventory, generic_slots_include_armor)
{
	headless::initGameRegistries();
	Level level(u"inventory-armor-slot", Dimension::Id_Normal, 10LL);
	Player player(level);

	ItemStack helmet(Items::helmetLeather->getShiftedIndex(), 1, 0);
	player.inventory.setItem(36, helmet);
	ItemStack *stored = player.inventory.getStackInSlot(36);
	if (!ctx.check(stored != nullptr, "slot 36 must map to armor slot 0"))
		return;
	ctx.checkEqual(stored->itemID, helmet.itemID, "generic armor slot item id");

	std::unique_ptr<ItemStack> removed(player.inventory.removeItem(36, 1));
	ctx.check(removed != nullptr, "generic armor slot must be removable");
	ctx.check(player.inventory.getStackInSlot(36) == nullptr, "removed armor slot must be empty");
}

HEADLESS_TEST(inventory, armor_damage_updates_each_piece)
{
	headless::initGameRegistries();
	Level level(u"inventory-armor-damage", Dimension::Id_Normal, 11LL);
	Player player(level);

	player.inventory.armorInventory[0] = ItemStack(Items::helmetLeather->getShiftedIndex(), 1, 0);
	player.inventory.armorInventory[1] = ItemStack(Items::plateLeather->getShiftedIndex(), 1, 0);
	player.inventory.hurtArmor(2);
	ctx.checkEqual(player.inventory.armorInventory[0].itemDamage, 2, "helmet damage");
	ctx.checkEqual(player.inventory.armorInventory[1].itemDamage, 2, "chestplate damage");
}

HEADLESS_TEST(inventory, death_drops_include_armor)
{
	headless::initGameRegistries();
	Level level(u"inventory-death-drops", Dimension::Id_Normal, 12LL);
	Player player(level);
	player.setPos(0.5, 65.0, 0.5);

	player.inventory.mainInventory[0] = coalStack(3);
	player.inventory.armorInventory[0] = ItemStack(Items::helmetLeather->getShiftedIndex(), 1, 0);
	player.inventory.dropAll();

	ctx.checkEqual(static_cast<long long>(level.entities.size()), 2,
		"one main stack and one armor stack must create two drops");
	ctx.check(player.inventory.mainInventory[0].isEmpty(), "main inventory must be cleared");
	ctx.check(player.inventory.armorInventory[0].isEmpty(), "armor inventory must be cleared");
}
