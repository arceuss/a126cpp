#include "world/entity/monster/Slime.h"

#include "world/level/Level.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/item/Items.h"
#include "util/Mth.h"
#include "nbt/CompoundTag.h"
#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846f
#endif

Slime::Slime(Level &level) : Mob(level)
{
	textureName = u"/mob/slime.png";
	size = 1 << random.nextInt(3);
	heightOffset = 0.0f;
	jumpDelay = random.nextInt(20) + 10;
	setSize(size);
}

void Slime::setSize(int_t var1)
{
	size = var1;
	Entity::setSize(0.6f * var1, 0.6f * var1);
	health = var1 * var1;
	setPos(x, y, z);
}

void Slime::addAdditionalSaveData(CompoundTag &tag)
{
	Mob::addAdditionalSaveData(tag);
	tag.putInt(u"Size", size - 1);
}

void Slime::readAdditionalSaveData(CompoundTag &tag)
{
	Mob::readAdditionalSaveData(tag);
	size = tag.getInt(u"Size") + 1;
}

void Slime::tick()
{
	oSquish = squish;
	bool var1 = onGround;
	Mob::tick();
	if (onGround && !var1)
	{
		for (int_t var2 = 0; var2 < size * 8; var2++)
		{
			float var3 = random.nextFloat() * M_PI * 2.0f;
			float var4 = random.nextFloat() * 0.5f + 0.5f;
			float var5 = Mth::sin(var3) * size * 0.5f * var4;
			float var6 = Mth::cos(var3) * size * 0.5f * var4;
			level.addParticle(u"slime", x + var5, bb.y0, z + var6, 0.0, 0.0, 0.0);
		}

		if (size > 2)
		{
			level.playSound(this, u"mob.slime", getSoundVolume(), ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) / 0.8f);
		}

		squish = -0.5f;
	}

	squish *= 0.6f;
}

void Slime::updateAi()
{
	std::shared_ptr<Player> var1 = level.getNearestPlayer(*this, 16.0);
	if (var1 != nullptr)
	{
		lookAt(*var1, 10.0f);
	}

	if (onGround && jumpDelay-- <= 0)
	{
		jumpDelay = random.nextInt(20) + 10;
		if (var1 != nullptr)
		{
			jumpDelay /= 3;
		}

		jumping = true;
		if (size > 1)
		{
			level.playSound(this, u"mob.slime", getSoundVolume(), ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 0.8f);
		}

		squish = 1.0f;
		xxa = 1.0f - random.nextFloat() * 2.0f;
		yya = 1 * size;
	}
	else
	{
		jumping = false;
		if (onGround)
		{
			xxa = yya = 0.0f;
		}
	}
}

void Slime::remove()
{
	if (size > 1 && health == 0)
	{
		for (int_t var1 = 0; var1 < 4; var1++)
		{
			float var2 = (var1 % 2 - 0.5f) * size / 4.0f;
			float var3 = (var1 / 2 - 0.5f) * size / 4.0f;
			Slime *var4 = new Slime(level);
			var4->setSize(size / 2);
			var4->moveTo(x + var2, y + 0.5, z + var3, random.nextFloat() * 360.0f, 0.0f);
			level.addEntity(std::shared_ptr<Entity>(var4));
		}
	}

	Mob::remove();
}

void Slime::playerTouch(Player &player)
{
	if (size > 1 && canSee(player) && distanceTo(player) < 0.6 * size && player.hurt(this, size))
	{
		level.playSound(this, u"mob.slimeattack", 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
	}
}

jstring Slime::getHurtSound()
{
	return u"mob.slime";
}

jstring Slime::getDeathSound()
{
	return u"mob.slime";
}

int_t Slime::getDeathLoot()
{
	return size == 1 && Items::slimeBall != nullptr ? Items::slimeBall->getShiftedIndex() : 0;
}

bool Slime::canSpawn()
{
	std::shared_ptr<LevelChunk> var1 = level.getChunkAt(Mth::floor(x), Mth::floor(z));
	return (size == 1 || level.difficulty > 0) && random.nextInt(10) == 0 && var1->getRandom(987234911L).nextInt(10) == 0 && y < 16.0;
}

float Slime::getSoundVolume()
{
	return 0.6f;
}
