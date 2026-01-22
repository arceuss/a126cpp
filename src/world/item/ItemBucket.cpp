#include "world/item/ItemBucket.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidFlowingTile.h"
#include "world/level/tile/FluidStationaryTile.h"
#include "world/level/material/Material.h"
#include "world/item/Items.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "util/Mth.h"
#include <memory>

// Beta: BucketItem(int var1, int var2) (BucketItem.java:15-20)
// Alpha: ItemBucket(int var1, int var2) (ItemBucket.java:6-11)
ItemBucket::ItemBucket(int_t id, int_t content) : Item(id), content(content)
{
	// Beta: this.maxStackSize = 1 (BucketItem.java:17)
	setMaxStackSize(1);
	// Beta: this.maxDamage = 64 (BucketItem.java:18)
	setMaxDamage(64);
}

// Beta: use() - picks up or places liquid (BucketItem.java:23-110)
// Alpha: onItemRightClick() - same logic (ItemBucket.java:13-102)
ItemStack ItemBucket::use(ItemStack &stack, Level &level, Player &player)
{
	// Beta: Calculate raycast from player (BucketItem.java:24-38)
	float partialTick = 1.0f;
	float pitch = player.xRotO + (player.xRot - player.xRotO) * partialTick;
	float yaw = player.yRotO + (player.yRot - player.yRotO) * partialTick;
	double px = player.xo + (player.x - player.xo) * partialTick;
	double py = player.yo + (player.y - player.yo) * partialTick + 1.62 - player.heightOffset;
	double pz = player.zo + (player.z - player.zo) * partialTick;
	
	Vec3 *fromPtr = Vec3::newTemp(px, py, pz);
	Vec3 from = *fromPtr;
	constexpr double PI = 3.14159265358979323846;
	float cosYaw = Mth::cos(-yaw * (float)(PI / 180.0) - (float)PI);
	float sinYaw = Mth::sin(-yaw * (float)(PI / 180.0) - (float)PI);
	float cosPitch = -Mth::cos(-pitch * (float)(PI / 180.0));
	float sinPitch = Mth::sin(-pitch * (float)(PI / 180.0));
	float dirX = sinYaw * cosPitch;
	float dirZ = cosYaw * cosPitch;
	double reach = 5.0;
	Vec3 *toPtr = from.add(dirX * reach, sinPitch * reach, dirZ * reach);
	Vec3 to = *toPtr;
	
	// Beta: Raycast to find hit (BucketItem.java:39)
	HitResult hit = level.clip(from, to, content == 0);
	if (hit.type == HitResult::Type::NONE)
		return stack;
	
	if (hit.type == HitResult::Type::TILE)
	{
		int_t hitX = hit.x;
		int_t hitY = hit.y;
		int_t hitZ = hit.z;
		
		// Beta: Check if player can interact (BucketItem.java:47-49)
		auto playerPtr = std::make_shared<Player>(player);
		if (!level.mayInteract(playerPtr, hitX, hitY, hitZ))
			return stack;
		
		// Beta: Empty bucket - pick up liquid (BucketItem.java:51-60)
		// Beta: Checks Material.water/lava and data == 0 (source block)
		// Can pick up both flowing (ID 8/10) and stationary (ID 9/11) blocks if they have metadata 0
		if (content == 0)
		{
			const Material &mat = level.getMaterial(hitX, hitY, hitZ);
			int_t data = level.getData(hitX, hitY, hitZ);
			// Beta: Pick up water source block (Material.water && data == 0)
			if (mat == Material::water && data == 0)
			{
				level.setTile(hitX, hitY, hitZ, 0);
				return ItemStack(Items::bucketWater->getShiftedIndex(), 1);
			}
			// Beta: Pick up lava source block (Material.lava && data == 0)
			if (mat == Material::lava && data == 0)
			{
				level.setTile(hitX, hitY, hitZ, 0);
				return ItemStack(Items::bucketLava->getShiftedIndex(), 1);
			}
		}
		else
		{
			// Beta: Full bucket - place liquid (BucketItem.java:61-102)
			if (content < 0)
			{
				// Special case: milk bucket
				return ItemStack(Items::bucketEmpty->getShiftedIndex(), 1);
			}
			
			// Beta: Adjust position based on hit face (BucketItem.java:66-88)
			if (hit.f == Facing::DOWN)
				hitY--;
			else if (hit.f == Facing::UP)
				hitY++;
			else if (hit.f == Facing::NORTH)
				hitZ--;
			else if (hit.f == Facing::SOUTH)
				hitZ++;
			else if (hit.f == Facing::WEST)
				hitX--;
			else if (hit.f == Facing::EAST)
				hitX++;
			
			// Beta: Check if can place (BucketItem.java:90)
			if (level.isEmptyTile(hitX, hitY, hitZ) || !level.getMaterial(hitX, hitY, hitZ).isSolid())
			{
				// Beta: Check if water in nether (BucketItem.java:91-97)
				// Water fizzes and doesn't place in nether (ultraWarm dimension)
				if (content == Tile::water.id && level.dimension->ultraWarm)
				{
					level.playSound(px + 0.5, py + 0.5, pz + 0.5, u"random.fizz", 0.5f, 2.6f + (level.random.nextFloat() - level.random.nextFloat()) * 0.8f);
					for (int_t i = 0; i < 8; i++)
					{
						level.addParticle(u"largesmoke", hitX + level.random.nextDouble(), hitY + level.random.nextDouble(), hitZ + level.random.nextDouble(), 0.0, 0.0, 0.0);
					}
				}
				else
				{
					// Beta: Place liquid block (BucketItem.java:98)
					level.setTileAndData(hitX, hitY, hitZ, content, 0);
				}
				
				return ItemStack(Items::bucketEmpty->getShiftedIndex(), 1);
			}
		}
	}
	else if (content == 0 && hit.type == HitResult::Type::ENTITY)
	{
		// Beta: Milk cow (BucketItem.java:104-105)
		// TODO: Check if entity is Cow and return milk bucket
		// if (hit.entity instanceof Cow)
		//     return new ItemStack(Items::bucketMilk->getShiftedIndex(), 1);
	}
	
	return stack;
}
