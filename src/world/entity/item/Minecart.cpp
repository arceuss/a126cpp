#include "world/entity/item/Minecart.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/player/Player.h"
#include "world/entity/Mob.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "nbt/StringTag.h"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

// Simple Gaussian approximation using Box-Muller transform
static float nextGaussian(Random &random)
{
	// Box-Muller transform
	float u1 = random.nextFloat();
	float u2 = random.nextFloat();
	if (u1 == 0.0f) u1 = 0.0001f; // Avoid log(0)
	return sqrt(-2.0f * log(u1)) * cos(2.0f * M_PI * u2);
}

Minecart::Minecart(Level &level) : Entity(level), items(36, nullptr)
{
	blocksBuilding = true;
	setSize(0.98f, 0.7f);
	heightOffset = bbHeight / 2.0f;
	makeStepSound = false;
}

Minecart::Minecart(Level &level, double x, double y, double z, int_t type) : Minecart(level)
{
	setPos(x, y + heightOffset, z);
	xd = 0.0;
	yd = 0.0;
	zd = 0.0;
	xo = x;
	yo = y;
	zo = z;
	this->type = type;
}

void Minecart::defineSynchedData()
{
}

AABB *Minecart::getCollideAgainstBox(Entity &entity)
{
	return &entity.bb;
}

AABB *Minecart::getCollideBox()
{
	return nullptr;
}

bool Minecart::isPushable()
{
	return true;
}

double Minecart::getRideHeight()
{
	return bbHeight * 0.0 - 0.3f;
}

bool Minecart::hurt(Entity *var1, int_t var2)
{
	if (!level.isOnline && !removed)
	{
		hurtDir = -hurtDir;
		hurtTime = 10;
		markHurt();
		damage += var2 * 10;
		if (damage > 40)
		{
			spawnAtLocation(Items::minecartEmpty->getShiftedIndex(), 1, 0.0f);
			if (type == 1)
			{
				spawnAtLocation(54, 1, 0.0f);  // Tile.chest.id = 54
			}
			else if (type == 2)
			{
				spawnAtLocation(61, 1, 0.0f);  // Tile.furnace.id = 61
			}

			remove();
		}

		return true;
	}
	else
	{
		return true;
	}
}

void Minecart::animateHurt()
{
	std::cout << "Animating hurt" << std::endl;
	hurtDir = -hurtDir;
	hurtTime = 10;
	damage = damage + damage * 10;
}

bool Minecart::isPickable()
{
	return !removed;
}

void Minecart::remove()
{
	for (int_t var1 = 0; var1 < getContainerSize(); var1++)
	{
		std::shared_ptr<ItemStack> var2 = getItem(var1);
		if (var2 != nullptr)
		{
			float var3 = random.nextFloat() * 0.8f + 0.1f;
			float var4 = random.nextFloat() * 0.8f + 0.1f;
			float var5 = random.nextFloat() * 0.8f + 0.1f;

			while (var2->stackSize > 0)
			{
				int_t var6 = random.nextInt(21) + 10;
				if (var6 > var2->stackSize)
				{
					var6 = var2->stackSize;
				}

				var2->stackSize -= var6;
				ItemStack stack(var2->itemID, var6, var2->getAuxValue());
				EntityItem *var7 = new EntityItem(level, x + var3, y + var4, z + var5, stack);
				float var8 = 0.05f;
				var7->xd = nextGaussian(random) * var8;
				var7->yd = nextGaussian(random) * var8 + 0.2f;
				var7->zd = nextGaussian(random) * var8;
				level.addEntity(std::shared_ptr<Entity>(var7));
			}
		}
	}

	Entity::remove();
}

void Minecart::tick()
{
	if (hurtTime > 0)
	{
		hurtTime--;
	}

	if (damage > 0)
	{
		damage--;
	}

	if (level.isOnline && lSteps > 0)
	{
		if (lSteps > 0)
		{
			double var41 = x + (lx - x) / lSteps;
			double var42 = y + (ly - y) / lSteps;
			double var5 = z + (lz - z) / lSteps;
			double var43 = lyr - yRot;

			while (var43 < -180.0)
			{
				var43 += 360.0;
			}

			while (var43 >= 180.0)
			{
				var43 -= 360.0;
			}

			yRot = (float)(yRot + var43 / lSteps);
			xRot = (float)(xRot + (lxr - xRot) / lSteps);
			lSteps--;
			setPos(var41, var42, var5);
			setRot(yRot, xRot);
		}
		else
		{
			setPos(x, y, z);
			setRot(yRot, xRot);
		}
	}
	else
	{
		xo = x;
		yo = y;
		zo = z;
		yd -= 0.04f;
		int_t var1 = Mth::floor(x);
		int_t var2 = Mth::floor(y);
		int_t var3 = Mth::floor(z);
		if (level.getTile(var1, var2 - 1, var3) == 66)  // Tile.rail.id = 66
		{
			var2--;
		}

		double var4 = 0.4;
		bool var6 = false;
		double var7 = 0.0078125;
		if (level.getTile(var1, var2, var3) == 66)  // Tile.rail.id = 66
		{
			Vec3 *var9 = getPos(x, y, z);
			int_t var10 = level.getData(var1, var2, var3);
			y = var2;
			if (var10 >= 2 && var10 <= 5)
			{
				y = var2 + 1;
			}

			if (var10 == 2)
			{
				xd -= var7;
			}

			if (var10 == 3)
			{
				xd += var7;
			}

			if (var10 == 4)
			{
				zd += var7;
			}

			if (var10 == 5)
			{
				zd -= var7;
			}

			const int (*var11)[3] = EXITS[var10];
			double var12 = var11[1][0] - var11[0][0];
			double var14 = var11[1][2] - var11[0][2];
			double var16 = Mth::sqrt(var12 * var12 + var14 * var14);
			double var18 = xd * var12 + zd * var14;
			if (var18 < 0.0)
			{
				var12 = -var12;
				var14 = -var14;
			}

			double var20 = Mth::sqrt(xd * xd + zd * zd);
			xd = var20 * var12 / var16;
			zd = var20 * var14 / var16;
			double var22 = 0.0;
			double var24 = var1 + 0.5 + var11[0][0] * 0.5;
			double var26 = var3 + 0.5 + var11[0][2] * 0.5;
			double var28 = var1 + 0.5 + var11[1][0] * 0.5;
			double var30 = var3 + 0.5 + var11[1][2] * 0.5;
			var12 = var28 - var24;
			var14 = var30 - var26;
			if (var12 == 0.0)
			{
				x = var1 + 0.5;
				var22 = z - var3;
			}
			else if (var14 == 0.0)
			{
				z = var3 + 0.5;
				var22 = x - var1;
			}
			else
			{
				double var32 = x - var24;
				double var34 = z - var26;
				double var36 = (var32 * var12 + var34 * var14) * 2.0;
				var22 = var36;
			}

			x = var24 + var12 * var22;
			z = var26 + var14 * var22;
			setPos(x, y + heightOffset, z);
			double var52 = xd;
			double var53 = zd;
			if (rider != nullptr)
			{
				var52 *= 0.75;
				var53 *= 0.75;
			}

			if (var52 < -var4)
			{
				var52 = -var4;
			}

			if (var52 > var4)
			{
				var52 = var4;
			}

			if (var53 < -var4)
			{
				var53 = -var4;
			}

			if (var53 > var4)
			{
				var53 = var4;
			}

			move(var52, 0.0, var53);
			if (var11[0][1] != 0 && Mth::floor(x) - var1 == var11[0][0] && Mth::floor(z) - var3 == var11[0][2])
			{
				setPos(x, y + var11[0][1], z);
			}
			else if (var11[1][1] != 0 && Mth::floor(x) - var1 == var11[1][0] && Mth::floor(z) - var3 == var11[1][2])
			{
				setPos(x, y + var11[1][1], z);
			}

			if (rider != nullptr)
			{
				xd *= 0.997f;
				yd *= 0.0;
				zd *= 0.997f;
			}
			else
			{
				if (type == 2)
				{
					double var54 = Mth::sqrt(xPush * xPush + zPush * zPush);
					if (var54 > 0.01)
					{
						var6 = true;
						xPush /= var54;
						zPush /= var54;
						double var38 = 0.04;
						xd *= 0.8f;
						yd *= 0.0;
						zd *= 0.8f;
						xd = xd + xPush * var38;
						zd = zd + zPush * var38;
					}
					else
					{
						xd *= 0.9f;
						yd *= 0.0;
						zd *= 0.9f;
					}
				}

				xd *= 0.96f;
				yd *= 0.0;
				zd *= 0.96f;
			}

			Vec3 *var55 = getPos(x, y, z);
			if (var55 != nullptr && var9 != nullptr)
			{
				double var37 = (var9->y - var55->y) * 0.05;
				var20 = Mth::sqrt(xd * xd + zd * zd);
				if (var20 > 0.0)
				{
					xd = xd / var20 * (var20 + var37);
					zd = zd / var20 * (var20 + var37);
				}

				setPos(x, var55->y, z);
			}

			int_t var56 = Mth::floor(x);
			int_t var57 = Mth::floor(z);
			if (var56 != var1 || var57 != var3)
			{
				var20 = Mth::sqrt(xd * xd + zd * zd);
				xd = var20 * (var56 - var1);
				zd = var20 * (var57 - var3);
			}

			if (type == 2)
			{
				double var39 = Mth::sqrt(xPush * xPush + zPush * zPush);
				if (var39 > 0.01 && xd * xd + zd * zd > 0.001)
				{
					xPush /= var39;
					zPush /= var39;
					if (xPush * xd + zPush * zd < 0.0)
					{
						xPush = 0.0;
						zPush = 0.0;
					}
					else
					{
						xPush = xd;
						zPush = zd;
					}
				}
			}
		}
		else
		{
			if (xd < -var4)
			{
				xd = -var4;
			}

			if (xd > var4)
			{
				xd = var4;
			}

			if (zd < -var4)
			{
				zd = -var4;
			}

			if (zd > var4)
			{
				zd = var4;
			}

			if (onGround)
			{
				xd *= 0.5;
				yd *= 0.5;
				zd *= 0.5;
			}

			move(xd, yd, zd);
			if (!onGround)
			{
				xd *= 0.95f;
				yd *= 0.95f;
				zd *= 0.95f;
			}
		}

		// Beta 1.2: Minecart.tick() - check if rider is removed (Minecart.java:433-435)
		if (rider != nullptr && rider->removed)
		{
			rider = nullptr;
		}

		xRot = 0.0f;
		double var44 = xo - x;
		double var45 = zo - z;
		if (var44 * var44 + var45 * var45 > 0.001)
		{
			yRot = (float)(std::atan2(var45, var44) * 180.0 / M_PI);
			if (flipped)
			{
				yRot += 180.0f;
			}
		}

		double var13 = yRot - yRotO;

		while (var13 >= 180.0)
		{
			var13 -= 360.0;
		}

		while (var13 < -180.0)
		{
			var13 += 360.0;
		}

		if (var13 < -170.0 || var13 >= 170.0)
		{
			yRot += 180.0f;
			flipped = !flipped;
		}

		setRot(yRot, xRot);
		AABB expanded = *bb.grow(0.2f, 0.0, 0.2f);
		std::vector<std::shared_ptr<Entity>> var15 = level.getEntities(this, expanded);
		if (var15.size() > 0)
		{
			for (size_t var48 = 0; var48 < var15.size(); var48++)
			{
				Entity *var17 = var15[var48].get();
				if (var17 != rider.get() && var17->isPushable() && dynamic_cast<Minecart*>(var17) != nullptr)
				{
					var17->push(*this);
				}
			}
		}

		if (rider != nullptr && rider->removed)
		{
			rider = nullptr;
		}

		if (var6 && random.nextInt(4) == 0)
		{
			fuel--;
			if (fuel < 0)
			{
				xPush = zPush = 0.0;
			}

			level.addParticle(u"largesmoke", x, y + 0.8, z, 0.0, 0.0, 0.0);
		}
	}
}

Vec3 *Minecart::getPosOffs(double var1, double var3, double var5, double var7)
{
	int_t var9 = Mth::floor(var1);
	int_t var10 = Mth::floor(var3);
	int_t var11 = Mth::floor(var5);
	if (level.getTile(var9, var10 - 1, var11) == 66)  // Tile.rail.id = 66
	{
		var10--;
	}

	if (level.getTile(var9, var10, var11) == 66)  // Tile.rail.id = 66
	{
		int_t var12 = level.getData(var9, var10, var11);
		var3 = var10;
		if (var12 >= 2 && var12 <= 5)
		{
			var3 = var10 + 1;
		}

		const int (*var13)[3] = EXITS[var12];
		double var14 = var13[1][0] - var13[0][0];
		double var16 = var13[1][2] - var13[0][2];
		double var18 = Mth::sqrt(var14 * var14 + var16 * var16);
		var14 /= var18;
		var16 /= var18;
		var1 += var14 * var7;
		var5 += var16 * var7;
		if (var13[0][1] != 0 && Mth::floor(var1) - var9 == var13[0][0] && Mth::floor(var5) - var11 == var13[0][2])
		{
			var3 += var13[0][1];
		}
		else if (var13[1][1] != 0 && Mth::floor(var1) - var9 == var13[1][0] && Mth::floor(var5) - var11 == var13[1][2])
		{
			var3 += var13[1][1];
		}

		return getPos(var1, var3, var5);
	}
	else
	{
		return nullptr;
	}
}

Vec3 *Minecart::getPos(double var1, double var3, double var5)
{
	int_t var7 = Mth::floor(var1);
	int_t var8 = Mth::floor(var3);
	int_t var9 = Mth::floor(var5);
	if (level.getTile(var7, var8 - 1, var9) == 66)  // Tile.rail.id = 66
	{
		var8--;
	}

	if (level.getTile(var7, var8, var9) == 66)  // Tile.rail.id = 66
	{
		int_t var10 = level.getData(var7, var8, var9);
		var3 = var8;
		if (var10 >= 2 && var10 <= 5)
		{
			var3 = var8 + 1;
		}

		const int (*var11)[3] = EXITS[var10];
		double var12 = 0.0;
		double var14 = var7 + 0.5 + var11[0][0] * 0.5;
		double var16 = var8 + 0.5 + var11[0][1] * 0.5;
		double var18 = var9 + 0.5 + var11[0][2] * 0.5;
		double var20 = var7 + 0.5 + var11[1][0] * 0.5;
		double var22 = var8 + 0.5 + var11[1][1] * 0.5;
		double var24 = var9 + 0.5 + var11[1][2] * 0.5;
		double var26 = var20 - var14;
		double var28 = (var22 - var16) * 2.0;
		double var30 = var24 - var18;
		if (var26 == 0.0)
		{
			var1 = var7 + 0.5;
			var12 = var5 - var9;
		}
		else if (var30 == 0.0)
		{
			var5 = var9 + 0.5;
			var12 = var1 - var7;
		}
		else
		{
			double var32 = var1 - var14;
			double var34 = var5 - var18;
			double var36 = (var32 * var26 + var34 * var30) * 2.0;
			var12 = var36;
		}

		var1 = var14 + var26 * var12;
		var3 = var16 + var28 * var12;
		var5 = var18 + var30 * var12;
		if (var28 < 0.0)
		{
			var3++;
		}

		if (var28 > 0.0)
		{
			var3 += 0.5;
		}

		return Vec3::newTemp(var1, var3, var5);
	}
	else
	{
		return nullptr;
	}
}

void Minecart::addAdditionalSaveData(CompoundTag &tag)
{
	tag.putInt(u"Type", type);
	if (type == 2)
	{
		tag.putDouble(u"PushX", xPush);
		tag.putDouble(u"PushZ", zPush);
		tag.putShort(u"Fuel", (short_t)fuel);
	}
	else if (type == 1)
	{
		std::shared_ptr<ListTag> var2 = std::make_shared<ListTag>();

		for (int_t var3 = 0; var3 < items.size(); var3++)
		{
			if (items[var3] != nullptr)
			{
				std::shared_ptr<CompoundTag> var4 = std::make_shared<CompoundTag>();
				var4->putByte(u"Slot", (byte_t)var3);
				items[var3]->save(*var4);
				var2->add(var4);
			}
		}

		tag.put(u"Items", var2);
	}
}

void Minecart::readAdditionalSaveData(CompoundTag &tag)
{
	type = tag.getInt(u"Type");
	if (type == 2)
	{
		xPush = tag.getDouble(u"PushX");
		zPush = tag.getDouble(u"PushZ");
		fuel = tag.getShort(u"Fuel");
	}
	else if (type == 1)
	{
		std::shared_ptr<ListTag> var2 = tag.getList(u"Items");
		items.clear();
		items.resize(getContainerSize(), nullptr);

		for (int_t var3 = 0; var3 < var2->size(); var3++)
		{
			std::shared_ptr<CompoundTag> var4 = std::static_pointer_cast<CompoundTag>(var2->get(var3));
			int_t var5 = var4->getByte(u"Slot") & 255;
			if (var5 >= 0 && var5 < items.size())
			{
				items[var5] = std::make_shared<ItemStack>(*var4);
			}
		}
	}
}

float Minecart::getShadowHeightOffs()
{
	return 0.0f;
}

void Minecart::push(Entity &var1)
{
	if (!level.isOnline)
	{
		if (rider == nullptr || rider.get() != &var1)
		{
			Mob *mob = dynamic_cast<Mob*>(&var1);
			if (mob != nullptr && dynamic_cast<Player*>(&var1) == nullptr && type == 0 && xd * xd + zd * zd > 0.01 && rider == nullptr && var1.riding == nullptr)
			{
				// Find this minecart in the level's entities to get a shared_ptr
				std::shared_ptr<Entity> minecartPtr = nullptr;
				for (const auto &e : level.getAllEntities())
				{
					if (e.get() == this)
					{
						minecartPtr = e;
						break;
					}
				}
				if (minecartPtr)
				{
					var1.ride(minecartPtr);
				}
			}

			double var2 = var1.x - x;
			double var4 = var1.z - z;
			double var6 = var2 * var2 + var4 * var4;
			if (var6 >= 1.0E-4f)
			{
				var6 = Mth::sqrt(var6);
				var2 /= var6;
				var4 /= var6;
				double var8 = 1.0 / var6;
				if (var8 > 1.0)
				{
					var8 = 1.0;
				}

				var2 *= var8;
				var4 *= var8;
				var2 *= 0.1f;
				var4 *= 0.1f;
				var2 *= 1.0f - pushthrough;
				var4 *= 1.0f - pushthrough;
				var2 *= 0.5;
				var4 *= 0.5;
				Minecart *minecart = dynamic_cast<Minecart*>(&var1);
				if (minecart != nullptr)
				{
					double var10 = var1.xd + xd;
					double var12 = var1.zd + zd;
					if (minecart->type == 2 && type != 2)
					{
						xd *= 0.2f;
						zd *= 0.2f;
						push(var1.xd - var2, 0.0, var1.zd - var4);
						var1.xd *= 0.7f;
						var1.zd *= 0.7f;
					}
					else if (minecart->type != 2 && type == 2)
					{
						var1.xd *= 0.2f;
						var1.zd *= 0.2f;
						var1.push(xd + var2, 0.0, zd + var4);
						xd *= 0.7f;
						zd *= 0.7f;
					}
					else
					{
						var10 /= 2.0;
						var12 /= 2.0;
						xd *= 0.2f;
						zd *= 0.2f;
						push(var10 - var2, 0.0, var12 - var4);
						var1.xd *= 0.2f;
						var1.zd *= 0.2f;
						var1.push(var10 + var2, 0.0, var12 + var4);
					}
				}
				else
				{
					push(-var2, 0.0, -var4);
					var1.push(var2 / 4.0, 0.0, var4 / 4.0);
				}
			}
		}
	}
}

int_t Minecart::getContainerSize()
{
	return 27;
}

std::shared_ptr<ItemStack> Minecart::getItem(int_t slot)
{
	if (slot >= 0 && slot < items.size())
		return items[slot];
	return nullptr;
}

std::shared_ptr<ItemStack> Minecart::removeItem(int_t slot, int_t count)
{
	if (slot >= 0 && slot < items.size() && items[slot] != nullptr)
	{
		if (items[slot]->stackSize <= count)
		{
			std::shared_ptr<ItemStack> var4 = items[slot];
			items[slot] = nullptr;
			return var4;
		}
		else
		{
			ItemStack removed = items[slot]->remove(count);
			if (items[slot]->isEmpty())
			{
				items[slot] = nullptr;
			}

			return std::make_shared<ItemStack>(removed);
		}
	}
	else
	{
		return nullptr;
	}
}

void Minecart::setItem(int_t slot, std::shared_ptr<ItemStack> itemStack)
{
	if (slot >= 0 && slot < items.size())
	{
		items[slot] = itemStack;
		if (itemStack != nullptr && itemStack->stackSize > getMaxStackSize())
		{
			itemStack->stackSize = getMaxStackSize();
		}
	}
}

jstring Minecart::getName()
{
	return u"Minecart";
}

int_t Minecart::getMaxStackSize()
{
	return 64;
}

void Minecart::setChanged()
{
}

bool Minecart::interact(Player &var1)
{
	// Beta 1.2: Minecart.interact() - matches newb12/net/minecraft/world/entity/item/Minecart.java:711-739 exactly
	if (type == 0)
	{
		// Beta: if (this.rider != null && this.rider instanceof Player && this.rider != var1) { return true; }
		if (rider != nullptr && dynamic_cast<Player*>(rider.get()) != nullptr && rider.get() != &var1)
		{
			return true;
		}

		// Beta: if (!this.level.isOnline) { var1.ride(this); }
		if (!level.isOnline)
		{
			// Find this minecart in the level's entities to get a shared_ptr for ride()
			std::shared_ptr<Entity> minecartPtr = nullptr;
			for (const auto &e : level.getAllEntities())
			{
				if (e.get() == this)
				{
					minecartPtr = e;
					break;
				}
			}
			if (minecartPtr)
			{
				var1.ride(minecartPtr);
			}
		}
	}
	else if (type == 1)
	{
		// Beta: if (!this.level.isOnline) { var1.openContainer(this); }
		if (!level.isOnline)
		{
			// TODO: Implement openContainer for Minecart (chest minecart)
			// This would need to open a container screen for the minecart's inventory
		}
	}
	else if (type == 2)
	{
		// Beta: ItemInstance var2 = var1.inventory.getSelected();
		ItemStack *var2 = var1.getSelectedItem();
		// Beta: if (var2 != null && var2.id == Item.coal.id) {
		if (var2 != nullptr && var2->itemID == Items::coal->getShiftedIndex())
		{
			// Beta: if (--var2.count == 0) { var1.inventory.setItem(var1.inventory.selected, null); }
			if (--var2->stackSize == 0)
			{
				var1.removeSelectedItem();
			}

			// Beta: this.fuel += 1200;
			fuel += 1200;
		}

		// Beta: this.xPush = this.x - var1.x;
		// Beta: this.zPush = this.z - var1.z;
		xPush = x - var1.x;
		zPush = z - var1.z;
	}

	// Beta: return true;
	return true;
}

float Minecart::getLootContent()
{
	int_t var1 = 0;

	for (size_t var2 = 0; var2 < items.size(); var2++)
	{
		if (items[var2] != nullptr)
		{
			var1++;
		}
	}

	return (float)var1 / items.size();
}

void Minecart::lerpTo(double var1, double var3, double var5, float var7, float var8, int_t var9)
{
	lx = var1;
	ly = var3;
	lz = var5;
	lyr = var7;
	lxr = var8;
	lSteps = var9 + 2;
	xd = lxd;
	yd = lyd;
	zd = lzd;
}

void Minecart::lerpMotion(double var1, double var3, double var5)
{
	lxd = xd = var1;
	lyd = yd = var3;
	lzd = zd = var5;
}

bool Minecart::stillValid(Player &var1)
{
	return removed ? false : !(var1.distanceToSqr(x, y, z) > 64.0);
}
