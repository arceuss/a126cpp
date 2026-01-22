#include "world/item/ItemBoat.h"
#include "world/item/ItemStack.h"
#include "world/entity/player/Player.h"
#include "world/level/Level.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "util/Mth.h"
// TODO: Include Boat entity when available
// #include "world/entity/item/Boat.h"

// Beta: BoatItem(int var1) (BoatItem.java:11-14)
// Alpha: ItemBoat(int var1) (ItemBoat.java:11-14)
ItemBoat::ItemBoat(int_t id) : Item(id)
{
	// Beta: this.maxStackSize = 1 (BoatItem.java:13)
	setMaxStackSize(1);
}

// Beta: use() - places boat (BoatItem.java:17-50)
// Alpha: onItemRightClick() - same logic (ItemBoat.java:16-49)
ItemStack ItemBoat::use(ItemStack &stack, Level &level, Player &player)
{
	// Beta: Calculate raycast from player (BoatItem.java:18-32)
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
	
	// Beta: Raycast to find hit (BoatItem.java:33)
	HitResult hit = level.clip(from, to, true);
	if (hit.type == HitResult::Type::NONE)
		return stack;
	
	if (hit.type == HitResult::Type::TILE)
	{
		int_t hitX = hit.x;
		int_t hitY = hit.y;
		int_t hitZ = hit.z;
		
		// Beta: Spawn boat if not online (BoatItem.java:41-43)
		if (!level.isOnline)
		{
			// TODO: Create and add Boat entity
			// Boat *boat = new Boat(level, hitX + 0.5f, hitY + 1.5f, hitZ + 0.5f);
			// level.addEntity(boat);
		}
		
		// Beta: Decrement stack (BoatItem.java:45)
		stack.stackSize--;
	}
	
	return stack;
}
