#include "world/entity/Painting.h"

#include "world/level/Level.h"
#include "world/item/Items.h"
#include "world/item/ItemStack.h"
#include "world/entity/item/EntityItem.h"
#include "world/level/material/Material.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"
#include "java/Random.h"
#include <vector>
#include <algorithm>

Painting::MotiveData Painting::motiveData[] = {
	{u"Kebab", 16, 16, 0, 0},
	{u"Aztec", 16, 16, 16, 0},
	{u"Alban", 16, 16, 32, 0},
	{u"Aztec2", 16, 16, 48, 0},
	{u"Bomb", 16, 16, 64, 0},
	{u"Plant", 16, 16, 80, 0},
	{u"Wasteland", 16, 16, 96, 0},
	{u"Pool", 32, 16, 0, 32},
	{u"Courbet", 32, 16, 32, 32},
	{u"Sea", 32, 16, 64, 32},
	{u"Sunset", 32, 16, 96, 32},
	{u"Creebet", 32, 16, 128, 32},
	{u"Wanderer", 16, 32, 0, 64},
	{u"Graham", 16, 32, 16, 64},
	{u"Match", 32, 32, 0, 128},
	{u"Bust", 32, 32, 32, 128},
	{u"Stage", 32, 32, 64, 128},
	{u"Void", 32, 32, 96, 128},
	{u"SkullAndRoses", 32, 32, 128, 128},
	{u"Fighters", 64, 32, 0, 96},
	{u"Pointer", 64, 64, 0, 192},
	{u"Pigscene", 64, 64, 64, 192},
	{u"BurningSkull", 64, 64, 128, 192},
	{u"Skeleton", 64, 48, 192, 64},
	{u"DonkeyKong", 64, 48, 192, 112}
};

Painting::Painting(Level &level) : Entity(level)
{
	heightOffset = 0.0f;
	setSize(0.5f, 0.5f);
}

Painting::Painting(Level &level, int_t x, int_t y, int_t z, int_t dir) : Painting(level)
{
	xTile = x;
	yTile = y;
	zTile = z;
	std::vector<Motive> var6;

	for (int_t i = 0; i < 25; i++)
	{
		motive = static_cast<Motive>(i);
		setDir(dir);
		if (survives())
		{
			var6.push_back(static_cast<Motive>(i));
		}
	}

	if (var6.size() > 0)
	{
		motive = var6[random.nextInt(var6.size())];
	}

	setDir(dir);
}

Painting::Painting(Level &level, int_t x, int_t y, int_t z, int_t dir, const jstring &motiveName) : Painting(level)
{
	xTile = x;
	yTile = y;
	zTile = z;

	for (int_t i = 0; i < 25; i++)
	{
		if (motiveData[i].name == motiveName)
		{
			motive = static_cast<Motive>(i);
			break;
		}
	}

	setDir(dir);
}

void Painting::defineSynchedData()
{
}

void Painting::setDir(int_t var1)
{
	dir = var1;
	yRotO = yRot = var1 * 90;
	MotiveData &m = Painting::getMotiveData(motive);
	float var2 = m.w;
	float var3 = m.h;
	float var4 = m.w;
	if (var1 != 0 && var1 != 2)
	{
		var2 = 0.5f;
	}
	else
	{
		var4 = 0.5f;
	}

	var2 /= 32.0f;
	var3 /= 32.0f;
	var4 /= 32.0f;
	float var5 = xTile + 0.5f;
	float var6 = yTile + 0.5f;
	float var7 = zTile + 0.5f;
	float var8 = 0.5625f;
	if (var1 == 0)
	{
		var7 -= var8;
	}

	if (var1 == 1)
	{
		var5 -= var8;
	}

	if (var1 == 2)
	{
		var7 += var8;
	}

	if (var1 == 3)
	{
		var5 += var8;
	}

	if (var1 == 0)
	{
		var5 -= offs(m.w);
	}

	if (var1 == 1)
	{
		var7 += offs(m.w);
	}

	if (var1 == 2)
	{
		var5 += offs(m.w);
	}

	if (var1 == 3)
	{
		var7 -= offs(m.w);
	}

	var6 += offs(m.h);
	setPos(var5, var6, var7);
	float var9 = -0.00625f;
	bb.set(var5 - var2 - var9, var6 - var3 - var9, var7 - var4 - var9, var5 + var2 + var9, var6 + var3 + var9, var7 + var4 + var9);
}

float Painting::offs(int_t var1)
{
	if (var1 == 32)
	{
		return 0.5f;
	}
	else
	{
		return var1 == 64 ? 0.5f : 0.0f;
	}
}

void Painting::tick()
{
	if (checkInterval++ == 100 && !level.isOnline)
	{
		checkInterval = 0;
		if (!survives())
		{
			remove();
			if (Items::painting != nullptr)
			{
				ItemStack stack(Items::painting->getShiftedIndex(), 1);
				EntityItem *item = new EntityItem(level, x, y, z, stack);
				level.addEntity(std::shared_ptr<Entity>(item));
			}
		}
	}
}

bool Painting::survives()
{
	if (level.getCubes(*this, bb).size() > 0)
	{
		return false;
	}
	else
	{
		MotiveData &m = Painting::getMotiveData(motive);
		int_t var1 = m.w / 16;
		int_t var2 = m.h / 16;
		int_t var3 = xTile;
		int_t var4 = yTile;
		int_t var5 = zTile;
		if (dir == 0)
		{
			var3 = Mth::floor(x - m.w / 32.0f);
		}

		if (dir == 1)
		{
			var5 = Mth::floor(z - m.w / 32.0f);
		}

		if (dir == 2)
		{
			var3 = Mth::floor(x - m.w / 32.0f);
		}

		if (dir == 3)
		{
			var5 = Mth::floor(z - m.w / 32.0f);
		}

		var4 = Mth::floor(y - m.h / 32.0f);

		for (int_t var6 = 0; var6 < var1; var6++)
		{
			for (int_t var7 = 0; var7 < var2; var7++)
			{
				const Material *var8;
				if (dir != 0 && dir != 2)
				{
					var8 = &level.getMaterial(xTile, var4 + var7, var5 + var6);
				}
				else
				{
					var8 = &level.getMaterial(var3 + var6, var4 + var7, zTile);
				}

				if (!var8->isSolid())
				{
					return false;
				}
			}
		}

		std::vector<std::shared_ptr<Entity>> var10 = level.getEntities(this, bb);

		for (size_t var11 = 0; var11 < var10.size(); var11++)
		{
			if (dynamic_cast<Painting*>(var10[var11].get()) != nullptr)
			{
				return false;
			}
		}

		return true;
	}
}

bool Painting::isPickable()
{
	return true;
}

bool Painting::hurt(Entity *var1, int_t var2)
{
	if (!removed && !level.isOnline)
	{
		remove();
		markHurt();
		if (Items::painting != nullptr)
		{
			ItemStack stack(Items::painting->getShiftedIndex(), 1);
			EntityItem *item = new EntityItem(level, x, y, z, stack);
			level.addEntity(std::shared_ptr<Entity>(item));
		}
	}

	return true;
}

void Painting::addAdditionalSaveData(CompoundTag &tag)
{
	tag.putByte(u"Dir", (byte_t)dir);
	MotiveData &m = Painting::getMotiveData(motive);
	tag.putString(u"Motive", m.name);
	tag.putInt(u"TileX", xTile);
	tag.putInt(u"TileY", yTile);
	tag.putInt(u"TileZ", zTile);
}

void Painting::readAdditionalSaveData(CompoundTag &tag)
{
	dir = tag.getByte(u"Dir");
	xTile = tag.getInt(u"TileX");
	yTile = tag.getInt(u"TileY");
	zTile = tag.getInt(u"TileZ");
	jstring var2 = tag.getString(u"Motive");

	motive = Motive::Kebab;  // Default
	for (int_t i = 0; i < 25; i++)
	{
		if (motiveData[i].name == var2)
		{
			motive = static_cast<Motive>(i);
			break;
		}
	}

	setDir(dir);
}

Painting::Motive Painting::randomMotive()
{
	static Random staticRandom;
	return static_cast<Motive>(staticRandom.nextInt(25));
}

Painting::MotiveData &Painting::getMotiveData(Motive motive)
{
	return motiveData[static_cast<int_t>(motive)];
}
