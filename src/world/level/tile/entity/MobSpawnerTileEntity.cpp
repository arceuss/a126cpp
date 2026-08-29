#include "world/level/tile/entity/MobSpawnerTileEntity.h"
#include "world/level/Level.h"
#include "nbt/CompoundTag.h"
#include "world/entity/EntityIO.h"
#include "world/entity/Mob.h"
#include "world/phys/AABB.h"
#include "java/Random.h"

// Beta: MobSpawnerTileEntity() - entityId = "Pig", spawnDelay = 20 (MobSpawnerTileEntity.java:16-19)
MobSpawnerTileEntity::MobSpawnerTileEntity() : entityId(u"Pig"), spawnDelay(20)
{
}

bool MobSpawnerTileEntity::isNearPlayer()
{
	if (level == nullptr)
		return false;
	// Direct Alpha transliteration: TileEntityMobSpawner.java:19-20.
	return level->getNearestPlayer(static_cast<double>(x) + 0.5,
		static_cast<double>(y) + 0.5,
		static_cast<double>(z) + 0.5, 16.0) != nullptr;
}

// Beta: tick() - handles spawning logic (MobSpawnerTileEntity.java:34-97)
void MobSpawnerTileEntity::tick()
{
	// Beta: this.oSpin = this.spin (MobSpawnerTileEntity.java:35)
	oSpin = spin;
	
	// Beta: if (this.isNearPlayer()) (MobSpawnerTileEntity.java:36)
	if (isNearPlayer())
	{
		// Beta: double var1 = this.x + this.level.random.nextFloat() (MobSpawnerTileEntity.java:37-39)
		double px = x + level->random.nextFloat();
		double py = y + level->random.nextFloat();
		double pz = z + level->random.nextFloat();
		
		// Beta: this.level.addParticle("smoke", var1, var3, var5, 0.0, 0.0, 0.0) (MobSpawnerTileEntity.java:40)
		level->addParticle(u"smoke", px, py, pz, 0.0, 0.0, 0.0);
		// Beta: this.level.addParticle("flame", var1, var3, var5, 0.0, 0.0, 0.0) (MobSpawnerTileEntity.java:41)
		level->addParticle(u"flame", px, py, pz, 0.0, 0.0, 0.0);
		
		// Beta: for (this.spin = this.spin + 1000.0F / (this.spawnDelay + 200.0F); this.spin > 360.0; this.oSpin -= 360.0) (MobSpawnerTileEntity.java:43-45)
		spin = spin + 1000.0 / (spawnDelay + 200.0);
		while (spin > 360.0)
		{
			oSpin -= 360.0;
			spin -= 360.0;
		}
		
		if (level->isOnline)
			return;
		if (spawnDelay == -1)
			delay();
		if (spawnDelay > 0)
		{
			--spawnDelay;
			return;
		}
		else
		{
			const int_t attempts = 4;
			for (int_t i = 0; i < attempts; ++i)
			{
				// Beta: Mob var9 = (Mob)EntityIO.newEntity(this.entityId, this.level) (MobSpawnerTileEntity.java:57)
				std::shared_ptr<Entity> entity = EntityIO::newEntity(entityId, *level);
				std::shared_ptr<Mob> mob = std::dynamic_pointer_cast<Mob>(entity);
				if (mob == nullptr)
				{
					// Beta: if (var9 == null) return (MobSpawnerTileEntity.java:59)
					return;
				}
				
				// Alpha TileEntityMobSpawner.java:51-55 counts this exact
				// runtime class inside [x,y,z..x+1,y+1,z+1] grown by 8,4,8.
				const AABB spawnBox(x - 8.0, y - 4.0, z - 8.0,
					x + 9.0, y + 5.0, z + 9.0);
				int_t entityCount = 0;
				for (const std::shared_ptr<Entity> &nearby : level->entities)
				{
					if (typeid(*nearby) == typeid(*mob)
						&& nearby->bb.intersects(spawnBox))
						++entityCount;
				}
				if (entityCount >= 6)
				{
					delay();
					return;
				}
				
				// Beta: double var11 = this.x + (this.level.random.nextDouble() - this.level.random.nextDouble()) * 4.0 (MobSpawnerTileEntity.java:73-75)
				double spawnX = x + (level->random.nextDouble() - level->random.nextDouble()) * 4.0;
				double spawnY = y + level->random.nextInt(3) - 1;
				double spawnZ = z + (level->random.nextDouble() - level->random.nextDouble()) * 4.0;
				
				// Beta: var9.moveTo(var11, var13, var15, this.level.random.nextFloat() * 360.0F, 0.0F) (MobSpawnerTileEntity.java:76)
				mob->moveTo(spawnX, spawnY, spawnZ, level->random.nextFloat() * 360.0f, 0.0f);
				
				if (mob->canSpawn())
				{
					level->addEntity(mob);
					for (int_t j = 0; j < 20; ++j)
					{
						double fx = x + 0.5
							+ (level->random.nextFloat() - 0.5) * 2.0;
						double fy = y + 0.5
							+ (level->random.nextFloat() - 0.5) * 2.0;
						double fz = z + 0.5
							+ (level->random.nextFloat() - 0.5) * 2.0;
						level->addParticle(u"smoke", fx, fy, fz, 0.0, 0.0, 0.0);
						level->addParticle(u"flame", fx, fy, fz, 0.0, 0.0, 0.0);
					}
					mob->spawnAnim();
					delay();
				}
			}
			
			// Beta: super.tick() (MobSpawnerTileEntity.java:94)
			TileEntity::tick();
		}
	}
}

// Beta: delay() - sets spawnDelay = 200 + random(600) (MobSpawnerTileEntity.java:99-101)
void MobSpawnerTileEntity::delay()
{
	if (level != nullptr)
	{
		// Beta: this.spawnDelay = 200 + this.level.random.nextInt(600) (MobSpawnerTileEntity.java:100)
		spawnDelay = 200 + level->random.nextInt(600);
	}
	else
	{
		spawnDelay = 200;
	}
}

// Beta: load() - loads EntityId and Delay (MobSpawnerTileEntity.java:104-108)
void MobSpawnerTileEntity::load(CompoundTag &tag)
{
	TileEntity::load(tag);
	// Beta: this.entityId = var1.getString("EntityId") (MobSpawnerTileEntity.java:106)
	entityId = tag.getString(u"EntityId");
	// Beta: this.spawnDelay = var1.getShort("Delay") (MobSpawnerTileEntity.java:107)
	spawnDelay = tag.getShort(u"Delay");
}

// Beta: save() - saves EntityId and Delay (MobSpawnerTileEntity.java:111-115)
void MobSpawnerTileEntity::save(CompoundTag &tag)
{
	TileEntity::save(tag);
	// Beta: var1.putString("EntityId", this.entityId) (MobSpawnerTileEntity.java:113)
	tag.putString(u"EntityId", entityId);
	// Beta: var1.putShort("Delay", (short)this.spawnDelay) (MobSpawnerTileEntity.java:114)
	tag.putShort(u"Delay", static_cast<short_t>(spawnDelay));
}
