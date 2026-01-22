#include "world/entity/Mob.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/LadderTile.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "world/entity/player/Player.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/ItemStack.h"
#include "world/phys/Vec3.h"
#include "world/phys/HitResult.h"

#include "util/Mth.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI  3.14159265358979323846f
#endif

Mob::Mob(Level &level) : Entity(level)
{
	// Beta: Mob constructor sets footSize = 0.5F (Mob.java:78)
	footSize = 0.5f;
	// Beta: Mob constructor sets blocksBuilding = true (Mob.java:73)
	blocksBuilding = true;
}

void Mob::defineSynchedData()
{
	// Alpha 1.2.6: Mob.defineSynchedData() - defines carried item in DataWatcher for multiplayer synchronization
	// Data ID 1 is typically used for held/carried items in Alpha 1.2.6
	// Initialize with empty ItemStack - will be updated from server via Packet24MobSpawn metadata
	static constexpr int_t DATA_CARRIED_ITEM_ID = 1;
	ItemStack emptyItem;
	dataWatcher.addObject(DATA_CARRIED_ITEM_ID, emptyItem);
}

bool Mob::canSee(const Entity &entity)
{
	// newb12: Mob.canSee() - checks line of sight using level.clip() (Mob.java:85-87)
	// Note: Using const_cast temporarily - getHeadHeight() should ideally be const
	float myHeadHeight = getHeadHeight();
	float entityHeadHeight = const_cast<Entity&>(entity).getHeadHeight();
	
	// Copy Vec3 values before passing to level.clip() - it modifies the from Vec3
	Vec3 *fromPtr = Vec3::newTemp(x, y + myHeadHeight, z);
	Vec3 from = *fromPtr;  // Copy the value
	Vec3 *toPtr = Vec3::newTemp(entity.x, entity.y + entityHeadHeight, entity.z);
	Vec3 to = *toPtr;  // Copy the value
	
	HitResult result = level.clip(from, to);
	return result.type == HitResult::Type::NONE;
}

jstring Mob::getTexture()
{
	// newb12: Mob.getTexture() - returns textureName
	// Reference: newb12/net/minecraft/world/entity/Mob.java:89-92
	return textureName;
}

bool Mob::isPickable()
{
	// newb12: Mob.isPickable() - returns !removed (Mob.java:94-97)
	return !removed;
}

bool Mob::isPushable()
{
	// newb12: Mob.isPushable() - returns !removed (Mob.java:99-102)
	return !removed;
}

float Mob::getHeadHeight()
{
	// newb12: Mob.getHeadHeight() - returns bbHeight * 0.85F (Mob.java:105-107)
	return bbHeight * 0.85f;
}

int_t Mob::getAmbientSoundInterval()
{
	// newb12: Mob.getAmbientSoundInterval() - returns 80 (Mob.java:109-111)
	return 80;
}

void Mob::baseTick()
{
	// newb12: Mob.baseTick() - handles ambient sounds, water damage, and death particles (Mob.java:114-194)
	oAttackAnim = attackAnim;
	Entity::baseTick();
	
	// newb12: Ambient sound logic (Mob.java:117-123)
	if (random.nextInt(1000) < ambientSoundTime++)
	{
		ambientSoundTime = -getAmbientSoundInterval();
		jstring sound = getAmbientSound();
		if (!sound.empty())
			level.playSound(this, sound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
	}
	
	if (isAlive() && isInWall())
		hurt(nullptr, 1);

	if (fireImmune || level.isOnline)
		onFire = 0;

	// newb12: Water damage logic (Mob.java:133-151)
	const Material &waterMat = Material::water;
	if (isAlive() && isUnderLiquid(waterMat) && !isWaterMob())
	{
		airSupply--;
		if (airSupply == -20)
		{
			airSupply = 0;
			
			// newb12: Bubble particles (Mob.java:138-143)
			for (int_t i = 0; i < 8; i++)
			{
				float var2 = random.nextFloat() - random.nextFloat();
				float var3 = random.nextFloat() - random.nextFloat();
				float var4 = random.nextFloat() - random.nextFloat();
				level.addParticle(u"bubble", x + var2, y + var3, z + var4, xd, yd, zd);
			}
			
			hurt(nullptr, 2);
		}
		onFire = 0;
	}
	else
	{
		airSupply = Entity::TOTAL_AIR_SUPPLY;
	}

	oTilt = tilt;
	if (attackTime > 0) attackTime--;
	if (hurtTime > 0) hurtTime--;
	if (invulnerableTime > 0) invulnerableTime--;
	
	if (health <= 0)
	{
		deathTime++;
		if (deathTime > 20)
		{
			beforeRemove();
			remove();
			
			// newb12: Death particles (Mob.java:172-186)
			for (int_t var9 = 0; var9 < 20; var9++)
			{
				// newb12: Three independent calls to nextGaussian() (Mob.java:173-175)
				// Box-Muller transform to replicate Java's nextGaussian()
				double u1 = random.nextDouble();
				if (u1 < 1e-10) u1 = 1e-10;  // Avoid log(0)
				double u2 = random.nextDouble();
				double var10 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2) * 0.02;
				
				u1 = random.nextDouble();
				if (u1 < 1e-10) u1 = 1e-10;
				u2 = random.nextDouble();
				double var11 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2) * 0.02;
				
				u1 = random.nextDouble();
				if (u1 < 1e-10) u1 = 1e-10;
				u2 = random.nextDouble();
				double var6 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2) * 0.02;
				
				level.addParticle(
					u"explode",
					x + random.nextFloat() * bbWidth * 2.0F - bbWidth,
					y + random.nextFloat() * bbHeight,
					z + random.nextFloat() * bbWidth * 2.0F - bbWidth,
					var10, var11, var6
				);
			}
		}
	}

	animStepO = animStep;
	yBodyRotO = yBodyRot;
	yRotO = yRot;
	xRotO = xRot;
}

void Mob::spawnAnim()
{
	// newb12: Mob.spawnAnim() - spawn particles (Mob.java:196-213)
	for (int_t i = 0; i < 20; i++)
	{
		// Box-Muller transform for Gaussian distribution (replaces nextGaussian())
		double u1 = random.nextDouble();
		if (u1 < 1e-10) u1 = 1e-10;  // Avoid log(0)
		double u2 = random.nextDouble();
		double z0 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
		double z1 = std::sqrt(-2.0 * std::log(u1)) * std::sin(2.0 * M_PI * u2);
		double var2 = z0 * 0.02;
		double var4 = z1 * 0.02;
		u1 = random.nextDouble();
		if (u1 < 1e-10) u1 = 1e-10;
		u2 = random.nextDouble();
		z0 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
		double var6 = z0 * 0.02;
		double var8 = 10.0;
		level.addParticle(
			u"explode",
			x + random.nextFloat() * bbWidth * 2.0f - bbWidth - var2 * var8,
			y + random.nextFloat() * bbHeight - var4 * var8,
			z + random.nextFloat() * bbWidth * 2.0f - bbWidth - var6 * var8,
			var2, var4, var6
		);
	}
}

// Beta 1.2: Mob.rideTick() - matches newb12 Mob.java:216-220 exactly
void Mob::rideTick()
{
	// Beta: super.rideTick() (Mob.java:217)
	Entity::rideTick();
	// Beta: this.oRun = this.run; (Mob.java:219)
	oRun = run;
	// Beta: this.run = 0.0F; (Mob.java:220)
	run = 0.0f;
}

void Mob::lerpTo(double x, double y, double z, float yRot, float xRot, int_t steps)
{
	// Beta: RemotePlayer.lerpTo() - resets heightOffset (RemotePlayer.java:38-47)
	// Beta: this.heightOffset = 0.0F; (RemotePlayer.java:40)
	// Alpha 1.2.6: Reset heightOffset to prevent bouncing when landing
	heightOffset = 0.0f;
	
	// Alpha 1.2.6: EntityOtherPlayerMP.setPositionAndRotation2() - stores target position for interpolation
	// Java: this.field_784_bh = var1; this.field_783_bi = var3; this.field_782_bj = var5;
	//       this.field_780_bk = (double)var7; this.field_786_bl = (double)var8; this.field_785_bg = var9;
	// Store target position and rotation for interpolation (Java stores directly, no normalization)
	lx = x;
	ly = y;
	lz = z;
	lyr = static_cast<double>(yRot);
	lxr = static_cast<double>(xRot);
	lSteps = steps;
	
	// Alpha 1.2.6: Reset ySlideOffset when starting new interpolation to prevent sinking
	// Java's Entity.setPosition() uses field_9287_aY (ySlideOffset) in bounding box but doesn't reset it
	// However, for client-side entities that don't run physics, ySlideOffset can accumulate incorrectly
	// Resetting it at the start of interpolation ensures correct position when landing
	// This fixes the 1-block sink issue when landing while moving
	if (field_9343_G || interpolateOnly)
	{
		ySlideOffset = 0.0f;
	}
	
	// Alpha 1.2.6: Normalize yRotO relative to current yRot BEFORE interpolation starts
	// This prevents snapping when rotation wraps around 180/-180 during interpolation
	// Java: Entity.setPositionAndRotation() normalizes prevRotationYaw in absMoveTo()
	// CRITICAL: Java does NOT normalize pitch delta during network interpolation
	// Java's EntityOtherPlayerMP.onLivingUpdate() line 74 interpolates pitch directly:
	//   this.rotationPitch = (float)((double)this.rotationPitch + (this.field_786_bl - (double)this.rotationPitch) / (double)this.field_785_bg);
	// Pitch is clamped to -90 to 90, so when going from 89 to -89, Java just interpolates directly
	// The normalization in EntityLiving (lines 280-286) is for RENDERING interpolation, not network interpolation
	while (this->yRot - yRotO < -180.0f) yRotO -= 360.0f;
	while (this->yRot - yRotO >= 180.0f) yRotO += 360.0f;
	
	// DO NOT normalize pitch here - Java interpolates pitch directly without normalization
	// Pitch values are already clamped to -90 to 90, so direct interpolation works correctly
}

// Alpha 1.2.6: EntityLiving.func_9282_a() - handles entity status from Packet38
// Java: public void func_9282_a(byte var1) {
//       if(var1 == 2) {
//           this.field_704_R = 1.5F;
//           this.field_9306_bj = this.field_9366_o;
//           this.hurtTime = this.field_9332_M = 10;
//           this.field_9331_N = 0.0F;
//           ...
//       } else if(var1 == 3) {
//           this.health = 0;
//           this.onDeath((Entity)null);
//       } }
void Mob::func_9282_a(byte_t status)
{
	if (status == 2)  // Entity hurt
	{
		// Java: this.field_704_R = 1.5F; (bob animation - not needed for client-side)
		// Java: this.field_9306_bj = this.field_9366_o; (health before hurt - not needed for client-side)
		// Java: this.hurtTime = this.field_9332_M = 10;
		hurtTime = hurtDuration = 10;
		// Java: this.field_9331_N = 0.0F; (hurt direction)
		hurtDir = 0.0f;
		
		// Java: if(!this.worldObj.multiplayerWorld) { play sound }
		// Sound is handled server-side in Alpha 1.2.6
		
		// Java: this.canAttackEntity((Entity)null, 0);
		// Not needed for client-side
	}
	else if (status == 3)  // Entity death
	{
		// Java: if(!this.worldObj.multiplayerWorld) { play sound }
		// Sound is handled server-side in Alpha 1.2.6
		
		// Java: this.health = 0;
		health = 0;
		// Java: this.onDeath((Entity)null);
		// Death handling is done in tick() when health <= 0
	}
	else
	{
		// Java: super.func_9282_a(var1);
		Entity::func_9282_a(status);
	}
}

// Alpha 1.2.6: EntityLiving.func_9280_g() - triggers hurt animation
// Java: public void func_9280_g() {
//       this.hurtTime = this.field_9332_M = 10;
//       this.field_9331_N = 0.0F; }
void Mob::func_9280_g()
{
	hurtTime = hurtDuration = 10;
	hurtDir = 0.0f;
}

void Mob::superTick()
{
	Entity::tick();
}

void Mob::tick()
{
	Entity::tick();
	
	aiStep();

	double fxd = x - xo;
	double fzd = z - zo;
	float fd = Mth::sqrt(fxd * fxd + fzd * fzd);

	float targetBodyRot = yBodyRot;
	float speedAnimStep = 0.0f;

	oRun = run;

	float targetRun = 0.0f;
	if (fd > 0.05f)
	{
		targetRun = 1.0f;
		speedAnimStep = fd * 3.0f;
		targetBodyRot = std::atan2(fzd, fxd) * 180.0f / Mth::PI - 90.0f;
	}

	if (attackAnim > 0.0f)
		targetBodyRot = yRot;

	if (!onGround)
		targetRun = 0.0f;

	run += (targetRun - run) * 0.3f;

	float deltaBodyRot = targetBodyRot - yBodyRot;
	while (deltaBodyRot < -180.0f) deltaBodyRot += 360.0f;
	while (deltaBodyRot >= 180.0f) deltaBodyRot -= 360.0f;

	yBodyRot += deltaBodyRot * 0.3f;

	float deltaYRot = yRot - yBodyRot;
	while (deltaYRot < -180.0f) deltaYRot += 360.0f;
	while (deltaYRot >= 180.0f) deltaYRot -= 360.0f;

	bool rotAtLimit = deltaYRot < -90.0f || deltaYRot >= 90.0f;
	if (deltaYRot < -75.0f) deltaYRot = -75.0f;
	if (deltaYRot >= 75.0f) deltaYRot = 75.0f;

	yBodyRot = yRot - deltaYRot;
	if (deltaYRot * deltaYRot > 2500.0f)
		yBodyRot += deltaYRot * 0.2f;

	if (rotAtLimit)
		speedAnimStep *= -1.0f;

	while (yRot - yRotO < -180.0f) yRotO -= 360.0f;
	while (yRot - yRotO >= 180.0f) yRotO += 360.0f;

	while (yBodyRot - yBodyRotO < -180.0f) yBodyRotO -= 360.0f;
	while (yBodyRot - yBodyRotO >= 180.0f) yBodyRotO += 360.0f;

	while (xRot - xRotO < -180.0f) xRotO -= 360.0f;
	while (xRot - xRotO >= 180.0f) xRotO += 360.0f;

	animStep += speedAnimStep;
}

void Mob::setSize(float width, float height)
{

}

void Mob::heal(int_t heal)
{
	// newb12: Mob.heal() - increases health up to 20 (Mob.java:334-343)
	if (health > 0)
	{
		health += heal;
		if (health > 20)
			health = 20;
		
		invulnerableTime = invulnerableDuration / 2;
	}
}

bool Mob::hurt(Entity *source, int_t dmg)
{
	// Beta: Mob.hurt(Entity var1, int var2) - handles damage with invulnerability (Mob.java:316-374)
	if (level.isOnline)  // Beta: if (this.level.isOnline) (Mob.java:317)
		return false;  // Beta: return false (Mob.java:318)
	
	noActionTime = 0;  // Beta: this.noActionTime = 0 (Mob.java:320)
	if (health <= 0)  // Beta: if (this.health <= 0) (Mob.java:321)
		return false;  // Beta: return false (Mob.java:322)
	
	walkAnimSpeed = 1.5f;  // Beta: this.walkAnimSpeed = 1.5F (Mob.java:324)
	bool playEffects = true;  // Beta: boolean var3 = true (Mob.java:325)
	
	// Beta: Handle invulnerability window (Mob.java:326-340)
	if ((float)invulnerableTime > (float)invulnerableDuration / 2.0f)  // Beta: if ((float)this.invulnerableTime > (float)this.invulnerableDuration / 2.0F) (Mob.java:326)
	{
		if (dmg <= lastHurt)  // Beta: if (var2 <= this.lastHurt) (Mob.java:327)
			return false;  // Beta: return false (Mob.java:328)
		
		actuallyHurt(dmg - lastHurt);  // Beta: this.actuallyHurt(var2 - this.lastHurt) (Mob.java:331)
		lastHurt = dmg;  // Beta: this.lastHurt = var2 (Mob.java:332)
		playEffects = false;  // Beta: var3 = false (Mob.java:333)
	}
	else
	{
		lastHurt = dmg;  // Beta: this.lastHurt = var2 (Mob.java:335)
		lastHealth = health;  // Beta: this.lastHealth = this.health (Mob.java:336)
		invulnerableTime = invulnerableDuration;  // Beta: this.invulnerableTime = this.invulnerableDuration (Mob.java:337)
		actuallyHurt(dmg);  // Beta: this.actuallyHurt(var2) (Mob.java:338)
		hurtTime = hurtDuration = 10;  // Beta: this.hurtTime = this.hurtDuration = 10 (Mob.java:339)
	}
	
	hurtDir = 0.0f;  // Beta: this.hurtDir = 0.0F (Mob.java:342)
	if (playEffects)  // Beta: if (var3) (Mob.java:343)
	{
		// Beta: Broadcast hurt event (Mob.java:344)
		// TODO: Find this entity in level.entities to get shared_ptr for broadcastEntityEvent
		// For now, skip since broadcastEntityEvent is empty
		// level.broadcastEntityEvent(shared_from_this(), 2);
		markHurt();  // Beta: this.markHurt() (Mob.java:345)
		
		if (source != nullptr)  // Beta: if (var1 != null) (Mob.java:346)
		{
			double dx = source->x - x;  // Beta: double var4 = var1.x - this.x (Mob.java:347)
			double dz;  // Beta: double var6 (Mob.java:349)
			
			// Beta: Calculate direction vector, ensure it's not too small (Mob.java:380-382)
			for (dz = source->z - z; dx * dx + dz * dz < 1.0E-4; dz = (random.nextFloat() - random.nextFloat()) * 0.01)  // Beta: for (var6 = var1.z - this.z; var4 * var4 + var6 * var6 < 1.0E-4D; var6 = (Math.random() - Math.random()) * 0.01D) (Mob.java:380-381)
				dx = (random.nextFloat() - random.nextFloat()) * 0.01;  // Beta: var4 = (Math.random() - Math.random()) * 0.01D (Mob.java:381)
			
			hurtDir = (float)(std::atan2(dz, dx) * 180.0 / Mth::PI) - yRot;  // Beta: this.hurtDir = (float)(Math.atan2(var6, var4) * 180.0D / (double)((float)Math.PI)) - this.yRot (Mob.java:384)
			knockback(*source, dmg, dx, dz);  // Beta: this.knockback(var1, var2, var4, var6) (Mob.java:385)
		}
		else
		{
			hurtDir = (float)((int)(random.nextFloat() * 2.0f) * 180);  // Beta: this.hurtDir = (float)((int)(Math.random() * 2.0D) * 180) (Mob.java:387)
		}
	}
	
	// Beta: Handle death (Mob.java:391-399)
	if (health <= 0)  // Beta: if (this.health <= 0) (Mob.java:391)
	{
		if (playEffects)  // Beta: if (var3) (Mob.java:392)
		{
			jstring deathSound = getDeathSound();  // Beta: this.getDeathSound() (Mob.java:393)
			if (!deathSound.empty())  // Beta: Check if sound exists
				level.playSound(this, deathSound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);  // Beta: this.level.playSound(this, this.getDeathSound(), this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2F + 1.0F) (Mob.java:393)
		}
		
		die(source);  // Beta: this.die(var1) (Mob.java:396)
	}
	else if (playEffects)  // Beta: else if (var3) (Mob.java:397)
	{
		jstring hurtSound = getHurtSound();  // Beta: this.getHurtSound() (Mob.java:398)
		if (!hurtSound.empty())  // Beta: Check if sound exists
			level.playSound(this, hurtSound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);  // Beta: this.level.playSound(this, this.getHurtSound(), this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2F + 1.0F) (Mob.java:398)
	}
	
	return true;  // Beta: return true (Mob.java:401)
}

void Mob::animateHurt()
{
	// newb12: Mob.animateHurt() - sets hurt animation (Mob.java:407-410)
	hurtTime = hurtDuration = 10;
	hurtDir = 0.0f;
}

void Mob::actuallyHurt(int_t dmg)
{
	// Beta: Mob.actuallyHurt(int var1) - reduces health (Mob.java:412-414)
	health -= dmg;  // Beta: this.health -= var1 (Mob.java:413)
}

float Mob::getSoundVolume()
{
	// Beta: Mob.getSoundVolume() - returns 1.0F (Mob.java:416-418)
	return 1.0f;  // Beta: return 1.0F (Mob.java:417)
}

jstring Mob::getAmbientSound()
{
	return u"";
}

jstring Mob::getHurtSound()
{
	// Beta: Mob.getHurtSound() - returns "random.hurt" (Mob.java:424-426)
	return u"random.hurt";  // Beta: return "random.hurt" (Mob.java:425)
}

jstring Mob::getDeathSound()
{
	// Beta: Mob.getDeathSound() - returns "random.hurt" (Mob.java:428-430)
	return u"random.hurt";  // Beta: return "random.hurt" (Mob.java:429)
}

void Mob::knockback(Entity &source, int_t unknown, double x, double z)
{
	// Beta: Mob.knockback(Entity var1, int var2, double var3, double var5) - applies knockback (Mob.java:432-444)
	float dist = Mth::sqrt(x * x + z * z);  // Beta: float var7 = Mth.sqrt(var3 * var3 + var5 * var5) (Mob.java:433)
	float strength = 0.4f;  // Beta: float var8 = 0.4F (Mob.java:434)
	
	xd /= 2.0;  // Beta: this.xd /= 2.0 (Mob.java:435)
	yd /= 2.0;  // Beta: this.yd /= 2.0 (Mob.java:436)
	zd /= 2.0;  // Beta: this.zd /= 2.0 (Mob.java:437)
	
	xd -= x / dist * strength;  // Beta: this.xd -= var3 / var7 * var8 (Mob.java:438)
	yd += 0.4f;  // Beta: this.yd += 0.4F (Mob.java:439)
	zd -= z / dist * strength;  // Beta: this.zd -= var5 / var7 * var8 (Mob.java:440)
	
	if (yd > 0.4f)  // Beta: if (this.yd > 0.4F) (Mob.java:441)
		yd = 0.4f;  // Beta: this.yd = 0.4F (Mob.java:442)
}

void Mob::die(Entity *source)
{
	// Beta: Mob.die(Entity var1) - handles death (Mob.java:446-457)
	if (deathScore > 0 && source != nullptr)  // Beta: if (this.deathScore > 0 && var1 != null) (Mob.java:447)
		source->awardKillScore(*this, deathScore);  // Beta: var1.awardKillScore(this, this.deathScore) (Mob.java:448)
	
	dead = true;  // Beta: this.dead = true (Mob.java:451)
	if (!level.isOnline)  // Beta: if (!this.level.isOnline) (Mob.java:452)
		dropDeathLoot();  // Beta: this.dropDeathLoot() (Mob.java:453)
	
	// Beta: Broadcast death event (Mob.java:456)
	// TODO: Find this entity in level.entities to get shared_ptr for broadcastEntityEvent
	// For now, skip since broadcastEntityEvent is empty
	// level.broadcastEntityEvent(shared_from_this(), 3);
}

void Mob::dropDeathLoot()
{
	// newb12: Mob.dropDeathLoot() - spawns death loot items (Mob.java:459-468)
	int_t lootId = getDeathLoot();
	if (lootId > 0)
	{
		int_t count = random.nextInt(3);
		for (int_t i = 0; i < count; i++)
		{
			// newb12: Spawn item entity at mob location (similar to Tile::spawnResources)
			float spread = 0.7f;
			double offsetX = (double)(random.nextFloat() * spread) + (double)(1.0f - spread) * 0.5;
			double offsetY = (double)(random.nextFloat() * spread) + (double)(1.0f - spread) * 0.5;
			double offsetZ = (double)(random.nextFloat() * spread) + (double)(1.0f - spread) * 0.5;
			
			ItemStack dropStack(lootId, 1);
			double entityX = x + offsetX;
			double entityY = y + offsetY;
			double entityZ = z + offsetZ;
			auto itemEntity = std::make_shared<EntityItem>(level, entityX, entityY, entityZ, dropStack);
			level.addEntity(itemEntity);
		}
	}
}

int_t Mob::getDeathLoot()
{
	return 0;
}

void Mob::causeFallDamage(float distance)
{
	// Beta: Mob.causeFallDamage(float var1) - applies fall damage and plays step sound (Mob.java:445-456)
	int_t dmg = (int_t)std::ceil(distance - 3.0f);  // Beta: int var2 = (int)Math.ceil((double)(var1 - 3.0F)) (Mob.java:446)
	if (dmg > 0)  // Beta: if (var2 > 0) (Mob.java:447)
	{
		hurt(nullptr, dmg);  // Beta: this.hurt((Entity)null, var2) (Mob.java:448)
		
		// Beta: Play step sound on fall damage (Mob.java:449-453)
		int_t tile = level.getTile(Mth::floor(x), Mth::floor(y - 0.2f - heightOffset), Mth::floor(z));  // Beta: int var3 = this.level.getTile(Mth.floor(this.x), Mth.floor(this.y - (double)0.2F - (double)this.heightOffset), Mth.floor(this.z)) (Mob.java:449)
		if (tile > 0)  // Beta: if (var3 > 0) (Mob.java:450)
		{
			const Tile::SoundType *soundType = Tile::tiles[tile]->getSoundType();  // Beta: Tile.SoundType var4 = Tile.tiles[var3].soundType (Mob.java:451)
			level.playSound(this, soundType->getStepSound(), soundType->getVolume() * 0.5f, soundType->getPitch() * 0.75f);  // newb12: this.level.playSound(this, var4.getStepSound(), var4.getVolume() * 0.5F, var4.getPitch() * 0.75F) (Mob.java:482)
		}
	}
}

void Mob::travel(float x, float z)
{
	if (isInWater())
	{
		double oy = y;

		moveRelative(x, z, 0.02f);
		move(xd, yd, zd);

		xd *= 0.8;
		yd *= 0.8;
		zd *= 0.8;
		yd -= 0.02;

		// Beta: Only boost up if there's free space above (no blocks, no liquid) when hitting a wall (Mob.java:496)
		if (horizontalCollision && isFree(xd, yd + 0.6 - y + oy, zd))
			yd = 0.3;
	}
	else if (isInLava())
	{
		double oy = y;

		moveRelative(x, z, 0.02f);
		move(xd, yd, zd);

		xd *= 0.5;
		yd *= 0.5;
		zd *= 0.5;
		yd -= 0.02;

		if (horizontalCollision && isFree(xd, yd + 0.6 - y + oy, zd))
			yd = 0.3;
	}
	else
	{
		float friction = 0.91f;
		if (onGround)
		{
			friction = 0.546f;
			int_t tile = level.getTile(Mth::floor(x), Mth::floor(bb.y0) - 1, Mth::floor(z));
			if (tile > 0)
				friction = Tile::tiles[tile]->friction * 0.91f;
		}

		float acceleration = 0.16277136f / (friction * friction * friction);
		moveRelative(x, z, onGround ? (0.1f * acceleration) : 0.02f);

		friction = 0.91f;
		if (onGround)
		{
			friction = 0.546f;
			int_t tile = level.getTile(Mth::floor(x), Mth::floor(bb.y0) - 1, Mth::floor(z));
			if (tile > 0)
				friction = Tile::tiles[tile]->friction * 0.91f;
		}

		if (onLadder())
		{
			fallDistance = 0.0f;
			if (yd < -0.15) yd = -0.15;
		}

		move(xd, yd, zd);

		if (horizontalCollision && onLadder())
			yd = 0.2;

		yd -= 0.08;
		yd *= 0.98;
		xd *= friction;
		zd *= friction;
	}

	walkAnimSpeedO = walkAnimSpeed;
	double fdx = this->x - this->xo;
	double fdz = this->z - this->zo;
	float targetWalkAnimSpeed = Mth::sqrt(fdx * fdx + fdz * fdz) * 4.0f;
	if (targetWalkAnimSpeed > 1.0f) targetWalkAnimSpeed = 1.0f;
	walkAnimSpeed += (targetWalkAnimSpeed - walkAnimSpeed) * 0.4f;
	walkAnimPos += walkAnimSpeed;
}

bool Mob::onLadder()
{
	// Beta: Mob.onLadder() - checks if entity is on a ladder (Mob.java:561-565)
	int_t var1 = Mth::floor(x);  // Beta: int var1 = Mth.floor(this.x) (Mob.java:562)
	int_t var2 = Mth::floor(bb.y0);  // Beta: int var2 = Mth.floor(this.bb.y0) (Mob.java:563)
	int_t var3 = Mth::floor(z);  // Beta: int var3 = Mth.floor(this.z) (Mob.java:564)
	
	// Beta: Check if tile at current position or one block above is a ladder (Mob.java:565)
	return level.getTile(var1, var2, var3) == Tile::ladder.id || level.getTile(var1, var2 + 1, var3) == Tile::ladder.id;
}

bool Mob::isShootable()
{
	// newb12: Mob.isShootable() - returns true (Mob.java:568-571)
	return true;
}

void Mob::addAdditionalSaveData(CompoundTag &tag)
{

}

void Mob::readAdditionalSaveData(CompoundTag &tag)
{

}

bool Mob::isAlive()
{
	// newb12: Mob.isAlive() - returns !removed && health > 0 (Mob.java:594-596)
	return !removed && health > 0;
}

bool Mob::isWaterMob()
{
	return false;
}

void Mob::aiStep()
{
	if (lSteps > 0)
	{
		// Alpha 1.2.6: EntityOtherPlayerMP.onLivingUpdate() - interpolate position from network updates
		// Java: double var1 = this.posX + (this.field_784_bh - this.posX) / (double)this.field_785_bg;
		//       double var3 = this.posY + (this.field_783_bi - this.posY) / (double)this.field_785_bg;
		//       double var5 = this.posZ + (this.field_782_bj - this.posZ) / (double)this.field_785_bg;
		// Java does NOT blend with velocity - it uses pure interpolation
		// This ensures accurate position following, including proper jump height
		double var1 = x + (lx - x) / static_cast<double>(lSteps);
		double var3 = y + (ly - y) / static_cast<double>(lSteps);
		double var5 = z + (lz - z) / static_cast<double>(lSteps);

		// Alpha 1.2.6: EntityOtherPlayerMP.onLivingUpdate() - normalize rotation delta
		// Java: for(var7 = this.field_780_bk - (double)this.rotationYaw; var7 < -180.0D; var7 += 360.0D) {}
		//       while(var7 >= 180.0D) { var7 -= 360.0D; }
		double var7 = lyr - static_cast<double>(yRot);
		while (var7 < -180.0) var7 += 360.0;
		while (var7 >= 180.0) var7 -= 360.0;

		// Alpha 1.2.6: Update rotation with normalized delta
		// Java: this.rotationYaw = (float)((double)this.rotationYaw + var7 / (double)this.field_785_bg);
		// Java: this.rotationPitch = (float)((double)this.rotationPitch + (this.field_786_bl - (double)this.rotationPitch) / (double)this.field_785_bg);
		// CRITICAL: Java does NOT normalize pitch delta - it interpolates directly
		// Pitch is clamped to -90 to 90, so we need to handle wrap-around differently
		// However, Java's EntityLiving normalizes prevRotationPitch AFTER interpolation (lines 280-286)
		// So we should normalize xRotO BEFORE interpolation, not the delta
		yRot = static_cast<float>(static_cast<double>(yRot) + var7 / static_cast<double>(lSteps));
		
		// Alpha 1.2.6: Interpolate pitch directly (Java does NOT normalize pitch delta)
		// Java: this.rotationPitch = (float)((double)this.rotationPitch + (this.field_786_bl - (double)this.rotationPitch) / (double)this.field_785_bg);
		// Java does NOT normalize pitch delta - it interpolates directly
		// CRITICAL: Interpolate pitch BEFORE clamping to ensure smooth transitions
		// When going from 89 to -89, Java interpolates directly: rotationPitch + (-89 - rotationPitch) / steps
		// This smoothly transitions from 89 down to -89 without snapping
		xRot = static_cast<float>(static_cast<double>(xRot) + (lxr - static_cast<double>(xRot)) / static_cast<double>(lSteps));

		// Alpha 1.2.6: Clamp pitch to -90 to 90 (Java: Entity.java lines 142-147 for player input)
		// This ensures pitch stays within valid Minecraft range even if packets send values outside
		// CRITICAL: Clamp AFTER interpolation to allow smooth transitions through the range
		if (xRot < -90.0f) xRot = -90.0f;
		if (xRot > 90.0f) xRot = 90.0f;
		
		// Alpha 1.2.6: Normalize xRotO relative to NEW xRot AFTER interpolation
		// This ensures smooth rendering interpolation by keeping xRotO within 180 degrees of xRot
		// Java: EntityLiving normalizes prevRotationPitch after rotationPitch changes (lines 280-286)
		while (xRot - xRotO < -180.0f) xRotO -= 360.0f;
		while (xRot - xRotO >= 180.0f) xRotO += 360.0f;

		--lSteps;

		// Alpha 1.2.6: Set position and rotation (Java: this.setPosition(var1, var3, var5); this.setRotation(this.rotationYaw, this.rotationPitch);)
		// CRITICAL ORDER: In Java's EntityOtherPlayerMP.onLivingUpdate():
		// 1. Interpolation happens (updates posX, posY, posZ)
		// 2. setPosition() is called (just sets posX, posY, posZ - doesn't update prevPosX/Y/Z)
		// 3. prevPosX/Y/Z are updated in Entity.onUpdate() -> super.onUpdate() at the START of the NEXT tick
		// So during interpolation: prevPosY is from previous tick, posY is new interpolated position
		// This is correct - yo was set in baseTick() at start of current tick (from previous tick)
		// CRITICAL: Java does NOT update onGround during interpolation - it's only calculated in Entity.move()
		// which doesn't run for client-side entities. Updating onGround during interpolation causes jitter/bouncing.
		setPos(var1, var3, var5);
		setRot(yRot, xRot);
		
		// Alpha 1.2.6: Update onGround when interpolation completes (lSteps reaches 0)
		// Java's EntityOtherPlayerMP uses onGround for animations (lines 87-93) but doesn't set it during interpolation
		// We infer onGround from Y position change when interpolation completes to avoid jitter during interpolation
		if (lSteps == 0 && (field_9343_G || interpolateOnly))
		{
			// Calculate onGround from Y position change between the last two network updates
			// yo = position from previous tick (set in baseTick() at start of current tick)
			// y = final interpolated position (just set above)
			double dy = y - yo;
			
			// If Y decreased significantly, entity likely landed (onGround = true)
			// If Y increased significantly, entity is jumping/rising (onGround = false)
			// Use a threshold to avoid false positives from small position adjustments
			if (dy < -0.001)  // Y decreased (landed)
			{
				onGround = true;
			}
			else if (dy > 0.001)  // Y increased (jumping/rising)
			{
				onGround = false;
			}
			// If dy is very small, maintain previous onGround state
		}
		
		// Alpha 1.2.6: Normalize yRotO relative to yRot for rendering interpolation
		// Java: EntityLiving normalizes prevRotationYaw (lines 264-270)
		// This prevents snapping when rotation wraps around 180/-180
		// CRITICAL: Normalize AFTER interpolation to ensure smooth rendering
		// Note: xRotO was already normalized above after pitch interpolation
		while (yRot - yRotO < -180.0f) yRotO -= 360.0f;
		while (yRot - yRotO >= 180.0f) yRotO += 360.0f;
	}

	if (health <= 0)
	{
		jumping = false;
		xxa = 0.0f;
		yya = 0.0f;
		yRotA = 0.0f;
	}
	else if (!field_9343_G && !interpolateOnly)
	{
		// Alpha 1.2.6: EntityLiving.onLivingUpdate() - only run AI if not client-side
		// Java: } else if(!this.field_9343_G) {
		//     this.func_418_b_();  // AI update
		// }
		updateAi();
	}

	// Alpha 1.2.6: EntityLiving.onLivingUpdate() - jump logic (lines 578-588)
	// Java: boolean var9 = this.handleWaterMovement();
	//       boolean var2 = this.handleLavaMovement();
	//       if(this.isJumping) {
	//           if(var9) { this.motionY += 0.04F; }
	//           else if(var2) { this.motionY += 0.04F; }
	//           else if(this.onGround) { this.func_424_C(); }
	//       }
	// Note: For client-side entities (field_9343_G = true), isJumping stays false
	// because AI doesn't run (line 574: else if(!this.field_9343_G) { this.func_418_b_(); }),
	// so this block doesn't execute. Jumping visuals come from server position updates via interpolation.
	// Java does NOT infer jumping for client-side entities - it just lets isJumping stay false.
	bool var9 = isInWater();
	bool var2 = isInLava();
	
	if (jumping)
	{
		if (var9)
		{
			yd += 0.04F;
		}
		else if (var2)
		{
			yd += 0.04F;
		}
		else if (onGround)
		{
			jumpFromGround();
		}
	}

	// Movement
	// newb12: Movement decay (Mob.java:645-648)
	xxa *= 0.98F;
	yya *= 0.98F;
	yRotA *= 0.9F;
	travel(xxa, yya);
	
	// newb12: Entity collision (Mob.java:649-657)
	const auto &entities = level.getEntities(this, *bb.grow(0.2F, 0.0, 0.2F));
	if (!entities.empty())
	{
		for (const auto &entity : entities)
		{
			if (entity->isPushable())
				entity->push(*this);
		}
	}
}

void Mob::jumpFromGround()
{
	// newb12: Mob.jumpFromGround() - sets jump velocity (Mob.java:660-662)
	yd = 0.42F;
}

void Mob::updateAi()
{
	// newb12: Mob.updateAi() - handles player tracking, looking, and random movement (Mob.java:664-717)
	noActionTime++;
	
	// newb12: Get nearest player (Mob.java:666)
	std::shared_ptr<Player> player = level.getNearestPlayer(*this, -1.0);
	if (player != nullptr)
	{
		double dx = player->x - x;
		double dy = player->y - y;
		double dz = player->z - z;
		double distSqr = dx * dx + dy * dy + dz * dz;
		
		// newb12: Remove if too far (Mob.java:672-674)
		if (distSqr > 16384.0)
		{
			remove();
		}
		
		// newb12: Check noActionTime and remove if inactive (Mob.java:676-682)
		if (noActionTime > 600 && random.nextInt(800) == 0)
		{
			if (distSqr < 1024.0)
			{
				noActionTime = 0;
			}
			else
			{
				remove();
			}
		}
	}
	
	// newb12: Reset movement inputs (Mob.java:685-686)
	xxa = 0.0F;
	yya = 0.0F;
	
	// newb12: Random chance to look at nearby player (Mob.java:687-696)
	float var11 = 8.0F;
	if (random.nextFloat() < 0.02F)
	{
		player = level.getNearestPlayer(*this, var11);
		if (player != nullptr)
		{
			lookingAt = std::static_pointer_cast<Entity>(player);
			lookTime = 10 + random.nextInt(20);
		}
		else
		{
			yRotA = (random.nextFloat() - 0.5F) * 20.0F;
		}
	}
	
	// newb12: Handle looking at entity (Mob.java:698-702)
	if (lookingAt != nullptr)
	{
		lookAt(*lookingAt, 10.0F);
		if (lookTime-- <= 0 || lookingAt->removed || lookingAt->distanceToSqr(*this) > var11 * var11)
		{
			lookingAt = nullptr;
		}
	}
	else
	{
		// newb12: Random rotation when not looking (Mob.java:704-710)
		if (random.nextFloat() < 0.05F)
		{
			yRotA = (random.nextFloat() - 0.5F) * 20.0F;
		}
		
		yRot = yRot + yRotA;
		xRot = defaultLookAngle;
	}
	
	// newb12: Jump in water/lava (Mob.java:712-716)
	bool var3 = isInWater();
	bool var12 = isInLava();
	if (var3 || var12)
	{
		jumping = random.nextFloat() < 0.8F;
	}
}

void Mob::lookAt(Entity &entity, float speed)
{
	// newb12: Mob.lookAt() - smoothly rotates to look at entity (Mob.java:719-735)
	double dx = entity.x - x;
	double dz = entity.z - z;
	double dy;
	
	// newb12: Calculate target Y based on entity type (Mob.java:723-728)
	if (dynamic_cast<Mob*>(&entity) != nullptr)
	{
		Mob *mob = static_cast<Mob*>(&entity);
		dy = mob->y + mob->getHeadHeight() - (y + getHeadHeight());
	}
	else
	{
		dy = (entity.bb.y0 + entity.bb.y1) / 2.0 - (y + getHeadHeight());
	}
	
	double dist = Mth::sqrt(dx * dx + dz * dz);
	float targetYRot = (float)(std::atan2(dz, dx) * 180.0 / Mth::PI) - 90.0f;
	float targetXRot = (float)(std::atan2(dy, dist) * 180.0 / Mth::PI);
	
	xRot = -rotlerp(xRot, targetXRot, speed);
	yRot = rotlerp(yRot, targetYRot, speed);
}

float Mob::rotlerp(float from, float to, float speed)
{
	// newb12: Mob.rotlerp() - smooth rotation interpolation with clamping (Mob.java:737-757)
	float delta = to - from;
	
	while (delta < -180.0f)
		delta += 360.0f;
	
	while (delta >= 180.0f)
		delta -= 360.0f;
	
	if (delta > speed)
		delta = speed;
	
	if (delta < -speed)
		delta = -speed;
	
	return from + delta;
}

void Mob::beforeRemove()
{

}

bool Mob::canSpawn()
{
	// newb12: Mob.canSpawn() - checks if mob can spawn at current position
	// Reference: newb12/net/minecraft/world/entity/Mob.java:762-764
	return level.isUnobstructed(bb) && level.getCubes(*this, bb).size() == 0 && !level.containsAnyLiquid(bb);
}

void Mob::outOfWorld()
{
	hurt(nullptr, 4);
}

float Mob::getAttackAnim(float a)
{
	float f = attackAnim - oAttackAnim;
	if (f < 0.0f) f++;
	return oAttackAnim + f * a;
}

Vec3 *Mob::getPos(float a)
{
	if (a == 1.0f)
		return Vec3::newTemp(x, y, z);

	double xd = xo + (x - xo) * a;
	double yd = yo + (y - yo) * a;
	double zd = zo + (z - zo) * a;
	return Vec3::newTemp(xd, yd, zd);
}

Vec3 *Mob::getLookAngle()
{
	return getViewVector(1.0f);
}

Vec3 *Mob::getViewVector(float a)
{
	if (a == 1.0f)
	{
		float cy = Mth::cos(-yRot * Mth::DEGRAD - Mth::PI);
		float sy = Mth::sin(-yRot * Mth::DEGRAD - Mth::PI);
		float cx = -Mth::cos(-xRot * Mth::DEGRAD);
		float sx = Mth::sin(-xRot * Mth::DEGRAD);
		return Vec3::newTemp(sy * cx, sx, cy * cx);
	}
	else
	{
		float xa = yRotO + (yRot - yRotO) * a;
		float ya = xRotO + (xRot - xRotO) * a;
		float cy = Mth::cos(-xa * Mth::DEGRAD - Mth::PI);
		float sy = Mth::sin(-xa * Mth::DEGRAD - Mth::PI);
		float cx = -Mth::cos(-ya * Mth::DEGRAD);
		float sx = Mth::sin(-ya * Mth::DEGRAD);
		return Vec3::newTemp(sy * cx, sx, cy * cx);
	}
}

HitResult Mob::pick(float length, float a)
{
	Vec3 *pos = getPos(a);
	Vec3 *look = getLookAngle();
	Vec3 *to = pos->add(look->x * length, look->y * length, look->z * length);
	return level.clip(*pos, *to);
}

int_t Mob::getMaxSpawnClusterSize()
{
	return 4;
}

ItemStack *Mob::getCarriedItem()
{
	// newb12: Mob.getCarriedItem() - returns null (Mob.java:825-827)
	return nullptr;
}

void Mob::handleEntityEvent(byte_t event)
{

}
