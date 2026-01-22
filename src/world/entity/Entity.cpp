#include "world/entity/Entity.h"
#include "world/entity/item/EntityItem.h"
#include "world/item/ItemStack.h"

#include "world/level/Level.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/FluidTile.h"
#include "world/entity/player/Player.h"

#include "util/Mth.h"
#include <cmath>

int_t Entity::entityCounter = 0;

Entity::Entity(Level &level) : level(level), dataWatcher()
{
	// Alpha 1.2.6: Entity constructor - initialize DataWatcher with shared flags
	// Java: this.dataWatcher.addObject(0, Byte.valueOf((byte)0));
	// DATA_SHARED_FLAGS_ID = 0, initial value is 0 (no flags set)
	dataWatcher.addObject(0, static_cast<byte_t>(0));
	setPos(0.0, 0.0, 0.0);
	
	// Alpha 1.2.6: Entity constructor calls entityInit() (Entity.java:78)
	// In our codebase, this is defineSynchedData() (matches newb12 naming)
	defineSynchedData();
}

void Entity::resetPos()
{
	while (y > 0.0)
	{
		setPos(x, y, z);
		if (level.getCubes(*this, bb).empty())
			break;
		y++;
	}

	xd = yd = zd = 0.0;
	xRot = 0.0f;
}

void Entity::remove()
{
	removed = true;
}

void Entity::setSize(float width, float height)
{
	bbWidth = width;
	bbHeight = height;
}

void Entity::setPos(const EntityPos &pos)
{
	if (pos.move)
		setPos(pos.x, pos.y, pos.z);
	else
		setPos(x, y, z);

	if (pos.rot)
		setRot(pos.yRot, pos.xRot);
	else
		setRot(yRot, xRot);
}

void Entity::setRot(float yRot, float xRot)
{
	this->yRot = yRot;
	this->xRot = xRot;
}

// Alpha 1.2.6: Entity.setVelocity() - sets entity velocity (motionX, motionY, motionZ)
// Java: public void setVelocity(double var1, double var3, double var5) {
//       this.motionX = var1; this.motionY = var3; this.motionZ = var5; }
void Entity::setVelocity(double motionX, double motionY, double motionZ)
{
	this->xd = motionX;
	this->yd = motionY;
	this->zd = motionZ;
}

// Alpha 1.2.6: Entity.func_9282_a() - handles entity status from Packet38
// Java: public void func_9282_a(byte var1) { }
void Entity::func_9282_a(byte_t status)
{
	// Base Entity does nothing (Java: empty method)
}

// Alpha 1.2.6: Entity.func_9280_g() - triggers hurt animation
// Java: public void func_9280_g() { }
void Entity::func_9280_g()
{
	// Base Entity does nothing (Java: empty method)
}

void Entity::setPos(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;

	float hw = bbWidth / 2.0f;
	float h = bbHeight;
	bb.set(x - hw, y - heightOffset + ySlideOffset, z - hw, x + hw, y - heightOffset + ySlideOffset + h, z + hw);
}

void Entity::turn(float yRot, float xRot)
{
	float ox = this->xRot;
	float oy = this->yRot;

	this->yRot += yRot * 0.15;
	this->xRot -= xRot * 0.15;
	if (this->xRot < -90.0f) this->xRot = -90.0f;
	if (this->xRot > 90.0f) this->xRot = 90.0f;

	xRotO += this->xRot - ox;
	yRotO += this->yRot - oy;
}

void Entity::interpolateTurn(float yRot, float xRot)
{
	this->yRot = this->yRot + yRot * 0.15;
	this->xRot = this->xRot - xRot * 0.15;
	if (this->xRot < -90.0f) this->xRot = -90.0f;
	if (this->xRot > 90.0f) this->xRot = 90.0f;
}

void Entity::tick()
{
	baseTick();
}

void Entity::baseTick()
{
	if (riding != nullptr && riding->removed)
		riding = nullptr;

	tickCount++;

	// Setup interpolation
	walkDistO = walkDist;
	xo = x;
	yo = y;
	zo = z;
	
	// Alpha 1.2.6: Update previous rotations to current values
	// Java: EntityLiving.onUpdate() line 172: this.prevRotationPitch = this.rotationPitch;
	// Java sets prevRotationPitch = rotationPitch at the start of the tick
	// Normalization happens LATER in onLivingUpdate() (lines 280-286) AFTER interpolation
	// CRITICAL: Do NOT normalize here - normalization happens after interpolation in aiStep()
	yRotO = yRot;
	xRotO = xRot;

	// Water check
	if (isInWater())
	{
		if (!wasInWater && !firstTick)
		{
			// Beta: Splash sound and particles (Entity.java:223-242)
			float splashVolume = Mth::sqrt(xd * xd * 0.2f + yd * yd + zd * zd * 0.2f) * 0.2f;
			if (splashVolume > 1.0f)
				splashVolume = 1.0f;
			
			// Beta: Play splash sound (Entity.java:229)
			level.playSound(this, u"random.splash", splashVolume, 1.0f + (random.nextFloat() - random.nextFloat()) * 0.4f);
			
			// Beta: Add bubble and splash particles (Entity.java:230-242)
			float waterY = Mth::floor(bb.y0);
			for (int_t i = 0; i < 1.0f + bbWidth * 20.0f; i++)
			{
				float offsetX = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
				float offsetZ = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
				level.addParticle(u"bubble", x + offsetX, waterY + 1.0f, z + offsetZ, xd, yd - random.nextFloat() * 0.2f, zd);
			}
			
			for (int_t i = 0; i < 1.0f + bbWidth * 20.0f; i++)
			{
				float offsetX = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
				float offsetZ = (random.nextFloat() * 2.0f - 1.0f) * bbWidth;
				level.addParticle(u"splash", x + offsetX, waterY + 1.0f, z + offsetZ, xd, yd, zd);
			}
		}

		fallDistance = 0.0f;
		wasInWater = true;
		onFire = 0;
	}
	else
	{
		wasInWater = false;
	}

	// Beta: Fire check (Entity.java:252-267)
	if (level.isOnline)
	{
		onFire = 0;
	}
	else
	{
		if (onFire > 0)
		{
			if (fireImmune)
			{
				onFire -= 4;
				if (onFire < 0) onFire = 0;
			}
			else
			{
				if (onFire % 20 == 0)
					hurt(nullptr, 1);
				onFire--;
			}
		}
	}

	// Beta: Check for fire tiles and apply damage (Entity.java:499-514)
	bool inWater = isInWater();
	if (level.containsFireTile(bb))
	{
		burn(1);  // Beta: this.burn(1) (line 500)
		if (!inWater)
		{
			onFire++;  // Beta: this.onFire++ (line 502)
			if (onFire == 0)
			{
				onFire = 300;  // Beta: this.onFire = 300 (line 504)
			}
		}
	}
	else if (onFire <= 0)
	{
		onFire = -flameTime;  // Beta: this.onFire = -this.flameTime (line 508)
	}

	// Beta: Extinguish fire in water (Entity.java:511-514)
	if (inWater && onFire > 0)
	{
		level.playSound(this, u"random.fizz", 0.7f, 1.6f + (random.nextFloat() - random.nextFloat()) * 0.4f);
		onFire = -flameTime;  // Beta: this.onFire = -this.flameTime (line 513)
	}

	// Lava check
	if (isInLava())
		lavaHurt();

	// Out of world check
	if (y < -64.0)
		outOfWorld();

	// Flag check
	if (!level.isOnline)
	{
		setSharedFlag(FLAG_ONFIRE, onFire > 0);
		setSharedFlag(FLAG_RIDING, riding != nullptr);
	}

	firstTick = false;
}

void Entity::lavaHurt()
{
	if (!fireImmune)
	{
		hurt(nullptr, 4);
		onFire = 600;
	}
}

// Beta: burn() - damages entity when in fire (Entity.java:533-537)
void Entity::burn(int_t damage)
{
	if (!fireImmune)
	{
		hurt(nullptr, damage);
	}
}

void Entity::outOfWorld()
{
	remove();
}

bool Entity::isFree(double xd, double yd, double zd, double skin)
{
	AABB *tbb = bb.grow(skin, skin, skin)->cloneMove(xd, yd, zd);
	auto cubes = level.getCubes(*this, *tbb);
	return !cubes.empty() ? false : !level.containsAnyLiquid(*tbb);
}

bool Entity::isFree(double xd, double yd, double zd)
{
	AABB *tbb = bb.cloneMove(xd, yd, zd);
	auto cubes = level.getCubes(*this, *tbb);
	return !cubes.empty() ? false : !level.containsAnyLiquid(*tbb);
}

void Entity::move(double xd, double yd, double zd)
{
	if (noPhysics)
	{
		bb.move(xd, yd, zd);
		x = (bb.x0 + bb.x1) / 2.0;
		y = bb.y0 + heightOffset - ySlideOffset;
		z = (bb.z0 + bb.z1) / 2.0;
		return;
	}

	// xd *= 20.0f;
	// yd *= 6.0f;
	// zd *= 20.0f;

	double ox = x;
	double oz = z;
	double oxd = xd;
	double oyd = yd;
	double ozd = zd;

	AABB *oldBb = bb.copy();

	// Keep us from moving off platforms when sneaking
	bool sneaking = onGround && isSneaking();
	if (sneaking)
	{
		double safeStep = 0.05;

		while (xd != 0.0 && level.getCubes(*this, *bb.cloneMove(xd, -1.0, 0.0)).empty())
		{
			if (xd < safeStep && xd >= -safeStep)
				xd = 0.0;
			else if (xd > 0.0)
				xd -= safeStep;
			else
				xd += safeStep;
			oxd = xd;
		}

		while (zd != 0.0 && level.getCubes(*this, *bb.cloneMove(0.0, -1.0, zd)).empty())
		{
			if (zd < safeStep && zd >= -safeStep)
				zd = 0.0;
			else if (zd > 0.0)
				zd -= safeStep;
			else
				zd += safeStep;
			ozd = zd;
		}
	}

	// Clip against collisions
	const auto &cubes = level.getCubes(*this, *bb.expand(xd, yd, zd));

	for (auto &cube : cubes)
		yd = cube->clipYCollide(bb, yd);
	bb.move(0.0, yd, 0.0);
	if (!slide && oyd != yd)
		xd = yd = zd = 0;
	bool grounded = onGround || (oyd != yd && oyd < 0.0);

	for (auto &cube : cubes)
		xd = cube->clipXCollide(bb, xd);
	bb.move(xd, 0.0, 0.0);
	if (!slide && oxd != xd)
		xd = yd = zd = 0;

	for (auto &cube : cubes)
		zd = cube->clipZCollide(bb, zd);
	bb.move(0.0, 0.0, zd);
	if (!slide && ozd != zd)
		xd = yd = zd = 0;

	// Check for steps
	if (footSize > 0.0f && grounded && ySlideOffset < 0.05f && (oxd != xd || ozd != zd))
	{
		double ooxd = xd;
		double ooyd = yd;
		double oozd = zd;

		xd = oxd;
		yd = footSize;
		zd = ozd;

		AABB *obb = bb.copy();  // Save current bb for comparison later
		bb.set(*oldBb);  // Beta: Reset to original bb before any movement (Entity.java:390: this.bb.set(var17))

		const auto &cubes = level.getCubes(*this, *bb.expand(xd, yd, zd));

		for (auto &cube : cubes)
			yd = cube->clipYCollide(bb, yd);
		bb.move(0.0, yd, 0.0);
		if (!slide && oyd != yd)
			xd = yd = zd = 0;

		for (auto &cube : cubes)
			xd = cube->clipXCollide(bb, xd);
		bb.move(xd, 0.0, 0.0);
		if (!slide && oxd != xd)
			xd = yd = zd = 0;

		for (auto &cube : cubes)
			zd = cube->clipZCollide(bb, zd);
		bb.move(0.0, 0.0, zd);
		if (!slide && ozd != zd)
			xd = yd = zd = 0;

		if (ooxd * ooxd + oozd * oozd >= xd * xd + zd * zd)
		{
			xd = ooxd;
			yd = ooyd;
			zd = oozd;
			bb.set(*obb);
		}
		else
		{
			ySlideOffset += 0.5;
		}
	}

	// Update body
	x = (bb.x0 + bb.x1) / 2.0;
	y = bb.y0 + heightOffset - ySlideOffset;
	z = (bb.z0 + bb.z1) / 2.0;

	horizontalCollision = oxd != xd || ozd != zd;
	verticalCollision = oyd != yd;
	onGround = oyd != yd && oyd < 0.0;
	collision = horizontalCollision || verticalCollision;
	
	// Beta: checkFallDamage() - track fall distance and apply damage on landing (Entity.java:443)
	checkFallDamage(oyd, onGround);
	
	if (oxd != xd)
		this->xd = 0.0;
	if (oyd != yd)
		this->yd = 0.0;
	if (ozd != zd)
		this->zd = 0.0;

	// Footsteps
	float fdx = x - ox;
	float fdz = z - oz;

	if (makeStepSound && !sneaking)
	{
		walkDist += Mth::sqrt(fdx * fdx + fdz * fdz) * 0.6;

		int_t sx = Mth::floor(x);
		int_t sy = Mth::floor(y - 0.2 - heightOffset);
		int_t sz = Mth::floor(z);

		int_t stile = level.getTile(sx, sy, sz);
		if (walkDist > nextStep && stile > 0)
		{
			nextStep++;

			// Beta: Play step sounds (Entity.java:458-472)
			Tile *tile = Tile::tiles[stile];
			if (tile != nullptr)
			{
				const Tile::SoundType *soundType = tile->getSoundType();
				
				// Beta: Check for top snow (snow layer) (Entity.java:467-469)
				// Note: topSnow might not exist yet in Alpha 1.2.6, so check if it exists first
				int_t tileAbove = level.getTile(sx, sy + 1, sz);
				if (tileAbove > 0 && tileAbove < 256 && Tile::tiles[tileAbove] != nullptr)
				{
					Tile *aboveTile = Tile::tiles[tileAbove];
					// TODO: Check for topSnow when it exists
					// For now, just use the current tile's sound
				}
				
				if (!tile->material.isLiquid() && soundType != nullptr)
				{
					// Alpha 1.2.6: Only play step sounds in single-player (Entity.java:433-440)
					// In multiplayer, the server sends step sounds via Packet62Sound
					if (!level.isOnline)
					{
						jstring stepSoundName = soundType->getStepSound();
						level.playSound(this, stepSoundName, soundType->getVolume() * 0.15f, soundType->getPitch());
					}
				}

				tile->stepOn(level, sx, sy, sz, *this);
			}
		}
	}

	// Check tile collisions
	int_t x0 = Mth::floor(bb.x0);
	int_t x1 = Mth::floor(bb.x1);
	int_t y0 = Mth::floor(bb.y0);
	int_t y1 = Mth::floor(bb.y1);
	int_t z0 = Mth::floor(bb.z0);
	int_t z1 = Mth::floor(bb.z1);

	if (level.hasChunksAt(x0, y0, z0, x1, y1, z1))
	{
		for (int_t x = x0; x <= x1; x++)
		{
			for (int_t y = y0; y <= y1; y++)
			{
				for (int_t z = z0; z <= z1; z++)
				{
					int_t tile = level.getTile(x, y, z);
					if (tile > 0)
						Tile::tiles[tile]->entityInside(level, x, y, z, *this);
				}
			}
		}
	}

	ySlideOffset *= 0.4f;

	// Fire collisions
	
	// xd /= 20.0f;
	// yd /= 6.0f;
	// zd /= 20.0f;
}

// Beta: checkFallDamage() - tracks fall distance and applies damage on landing (Entity.java:518-527)
void Entity::checkFallDamage(double yd, bool onGround)
{
	if (onGround)  // Beta: if (var3) (Entity.java:519)
	{
		if (fallDistance > 0.0f)  // Beta: if (this.fallDistance > 0.0F) (Entity.java:520)
		{
			causeFallDamage(fallDistance);  // Beta: this.causeFallDamage(this.fallDistance) (Entity.java:521)
			fallDistance = 0.0f;  // Beta: this.fallDistance = 0.0F (Entity.java:522)
		}
	}
	else if (yd < 0.0)  // Beta: else if (var1 < 0.0) (Entity.java:524)
	{
		fallDistance = (float)(fallDistance - yd);  // Beta: this.fallDistance = (float)(this.fallDistance - var1) (Entity.java:525)
	}
}

AABB *Entity::getCollideBox()
{
	return nullptr;
}

// Beta: causeFallDamage() - applies fall damage (Entity.java:539-540) - empty default implementation
void Entity::causeFallDamage(float distance)
{
	// Beta: Default implementation is empty - Mob overrides this
}

bool Entity::isInWater()
{
	// Beta: Entity.isInWater() - checks if entity is in water (Entity.java:542-544)
	AABB *checkBb = bb.grow(0.0, -0.4, 0.0);
	const Material &waterMat = Material::water;
	return level.checkAndHandleWater(*checkBb, waterMat, this);
}

bool Entity::isUnderLiquid(const Material &material)
{
	// Beta: Entity.isUnderLiquid() - checks if head is below liquid surface (Entity.java:546-559)
	double headY = y + getHeadHeight();
	int_t blockX = Mth::floor(this->x);
	int_t blockY = Mth::floor((float)Mth::floor(headY));  // Beta: Mth.floor((float)Mth.floor(var2)) (Entity.java:549)
	int_t blockZ = Mth::floor(this->z);
	
	int_t tile = level.getTile(blockX, blockY, blockZ);
	if (tile == 0)
		return false;
	
	Tile *tilePtr = Tile::tiles[tile];
	if (tilePtr == nullptr || &tilePtr->material != &material)
		return false;
	
	// Beta: Check if head is below water surface (Entity.java:553-555)
	float liquidHeight = FluidTile::getHeight(level.getData(blockX, blockY, blockZ)) - 0.11111111f;  // Beta: -0.11111111F
	float surfaceY = blockY + 1 - liquidHeight;  // Beta: var5 + 1 - var8
	return headY < surfaceY;  // Beta: return var2 < var9 (Entity.java:555)
}

float Entity::getHeadHeight()
{
	return 0.0f;
}

bool Entity::isInLava()
{
	// Beta: Entity.isInLava() - checks if entity is in lava (similar to isInWater)
	AABB *checkBb = bb.grow(0.0, -0.4, 0.0);
	const Material &lavaMat = Material::lava;
	return level.checkAndHandleWater(*checkBb, lavaMat, this);
}

bool Entity::isFullySubmerged()
{
	// Check if the player's entire bounding box is in water (fully submerged)
	// This is stricter than isInWater() which shrinks the bounding box
	const Material &waterMat = Material::water;
	return level.checkAndHandleWater(bb, waterMat, this);
}

void Entity::moveRelative(float x, float z, float acc)
{
	float targetSqr = Mth::sqrt(x * x + z * z);
	if (!(targetSqr < 0.01F))
	{
		if (targetSqr < 1.0F) targetSqr = 1.0F;

		targetSqr = acc / targetSqr;
		x *= targetSqr;
		z *= targetSqr;

		float s = Mth::sin(yRot * Mth::PI / 180.0F);
		float c = Mth::cos(yRot * Mth::PI / 180.0F);
		xd += x * c - z * s;
		zd += z * c + x * s;
	}
}

float Entity::getBrightness(float a)
{
	int_t x = Mth::floor(this->x);
	double byo = (bb.y1 - bb.y0) * 0.66;
	int_t y = Mth::floor(this->y - heightOffset + byo);
	int_t z = Mth::floor(this->z);
	return level.hasChunksAt(Mth::floor(bb.x0), Mth::floor(bb.y0), Mth::floor(bb.z0), Mth::floor(bb.x1), Mth::floor(bb.y1), Mth::floor(bb.z1)) ? level.getBrightness(x, y, z) : 0.0f;
}

void Entity::absMoveTo(double x, double y, double z, float yRot, float xRot)
{
	xo = this->x = x;
	yo = this->y = y + heightOffset;
	zo = this->z = z;
	yRotO = this->yRot = yRot;
	xRotO = this->xRot = xRot;
	ySlideOffset = 0.0f;

	// Alpha 1.2.6: Normalize yRotO and xRotO relative to new yRot/xRot
	// Java: Entity.absMoveTo() normalizes prevRotationYaw (lines 599-603)
	// Java: EntityLiving also normalizes prevRotationPitch (lines 280-286)
	double dyRot = (yRotO - yRot);
	if (dyRot < -180.0)
		yRotO += 360.0;
	if (dyRot >= 180.0)
		yRotO -= 360.0;
	
	double dxRot = (xRotO - xRot);
	if (dxRot < -180.0)
		xRotO += 360.0;
	if (dxRot >= 180.0)
		xRotO -= 360.0;

	setPos(x, y, z);
	setRot(yRot, xRot);
}

void Entity::moveTo(double x, double y, double z, float yRot, float xRot)
{
	// Alpha 1.2.6: Entity.setPositionAndRotation() - sets position directly without heightOffset
	// Java: public void setPositionAndRotation(double var1, double var3, double var5, float var7, float var8) {
	//     this.prevPosX = this.posX = var1;
	//     this.prevPosY = this.posY = var3;  // NO heightOffset added!
	//     this.prevPosZ = this.posZ = var5;
	//     ...
	//     this.setPosition(this.posX, this.posY, this.posZ);
	// }
	// Note: heightOffset is only used in bounding box calculations, not in position storage
	xOld = xo = this->x = x;
	yOld = yo = this->y = y;  // Don't add heightOffset - it's only for bounding box
	zOld = zo = this->z = z;
	
	// Alpha 1.2.6: Entity.setPositionAndRotation() normalizes prevRotationYaw (lines 576-583)
	// Java: double var9 = (double)(this.prevRotationYaw - var7);
	//       if(var9 < -180.0D) { this.prevRotationYaw += 360.0F; }
	//       if(var9 >= 180.0D) { this.prevRotationYaw -= 360.0F; }
	// This prevents snapping when rotation wraps around 180/-180
	yRotO = this->yRot;
	xRotO = this->xRot;
	this->yRot = yRot;
	this->xRot = xRot;
	
	// Alpha 1.2.6: Normalize yRotO and xRotO relative to new yRot/xRot
	// Java: Entity.setPositionAndRotation() normalizes prevRotationYaw (lines 576-583)
	// Java: EntityLiving also normalizes prevRotationPitch (lines 280-286)
	double dyRot = (yRotO - yRot);
	if (dyRot < -180.0)
		yRotO += 360.0f;
	if (dyRot >= 180.0)
		yRotO -= 360.0f;
	
	double dxRot = (xRotO - xRot);
	if (dxRot < -180.0)
		xRotO += 360.0f;
	if (dxRot >= 180.0)
		xRotO -= 360.0f;
	
	setPos(x, y, z);
}

float Entity::distanceTo(const Entity &entity)
{
	float dx = x - entity.x;
	float dy = y - entity.y;
	float dz = z - entity.z;
	return Mth::sqrt(dx * dx + dy * dy + dz * dz);
}

double Entity::distanceToSqr(double x, double y, double z)
{
	float dx = this->x - x;
	float dy = this->y - y;
	float dz = this->z - z;
	return dx * dx + dy * dy + dz * dz;
}

double Entity::distanceTo(double x, double y, double z)
{
	float dx = this->x - x;
	float dy = this->y - y;
	float dz = this->z - z;
	return Mth::sqrt(dx * dx + dy * dy + dz * dz);
}

double Entity::distanceToSqr(const Entity &entity)
{
	float dx = x - entity.x;
	float dy = y - entity.y;
	float dz = z - entity.z;
	return dx * dx + dy * dy + dz * dz;
}

void Entity::playerTouch(Player &player)
{

}

void Entity::push(Entity &entity)
{

}

void Entity::push(double x, double y, double z)
{

}

void Entity::markHurt()
{

}

bool Entity::hurt(Entity *source, int_t dmg)
{
	return false;
}

bool Entity::intersects(double x0, double y0, double z0, double x1, double y1, double z1)
{
	return false;
}

bool Entity::isPickable()
{
	return false;
}

bool Entity::isPushable()
{
	return false;
}

bool Entity::isShootable()
{
	return false;
}

void Entity::awardKillScore(Entity &source, int_t dmg)
{

}

bool Entity::shouldRender(Vec3 &v)
{
	double dx = x - v.x;
	double dy = y - v.y;
	double dz = z - v.z;
	double d = dx * dx + dy * dy + dz * dz;
	return shouldRenderAtSqrDistance(d);
}

bool Entity::shouldRenderAtSqrDistance(double distance)
{
	double size = bb.getSize();
	size *= 64 * viewScale;
	return distance < (size * size);
}

jstring Entity::getTexture()
{
	return u"";
}

bool Entity::isCreativeModeAllowed()
{
	return false;
}

bool Entity::save(CompoundTag &tag)
{
	jstring id = getEncodeId();
	if (removed || id.empty())
		return false;

	tag.putString(u"id", id);
	saveWithoutId(tag);
	return true;
}

void Entity::saveWithoutId(CompoundTag &tag)
{
	std::shared_ptr<ListTag> listTag = Util::make_shared<ListTag>();
	listTag->add(Util::make_shared<DoubleTag>(x));
	listTag->add(Util::make_shared<DoubleTag>(y));
	listTag->add(Util::make_shared<DoubleTag>(z));
	tag.put(u"Pos", listTag);

	listTag = Util::make_shared<ListTag>();
	listTag->add(Util::make_shared<DoubleTag>(xd));
	listTag->add(Util::make_shared<DoubleTag>(yd));
	listTag->add(Util::make_shared<DoubleTag>(zd));
	tag.put(u"Motion", listTag);

	listTag = Util::make_shared<ListTag>();
	listTag->add(Util::make_shared<FloatTag>(yRot));
	listTag->add(Util::make_shared<FloatTag>(xRot));
	tag.put(u"Rotation", listTag);

	tag.putFloat(u"FallDistance", fallDistance);
	tag.putShort(u"Fire", onFire);
	tag.putShort(u"Air", airSupply);
	tag.putBoolean(u"OnGround", onGround);

	addAdditionalSaveData(tag);
}

void Entity::load(CompoundTag &tag)
{
	std::shared_ptr<ListTag> posTag = tag.getList(u"Pos");
	std::shared_ptr<ListTag> motionTag = tag.getList(u"Motion");
	std::shared_ptr<ListTag> rotationTag = tag.getList(u"Rotation");

	setPos(0.0, 0.0, 0.0);
	xd = (reinterpret_cast<DoubleTag*>(motionTag->get(0).get()))->data;
	yd = (reinterpret_cast<DoubleTag*>(motionTag->get(1).get()))->data;
	zd = (reinterpret_cast<DoubleTag*>(motionTag->get(2).get()))->data;
	xo = xOld = x = (reinterpret_cast<DoubleTag*>(posTag->get(0).get()))->data;
	yo = yOld = y = (reinterpret_cast<DoubleTag*>(posTag->get(1).get()))->data;
	zo = zOld = z = (reinterpret_cast<DoubleTag*>(posTag->get(2).get()))->data;
	yRotO = yRot = (reinterpret_cast<FloatTag*>(rotationTag->get(0).get()))->data;
	xRotO = xRot = (reinterpret_cast<FloatTag*>(rotationTag->get(1).get()))->data;

	fallDistance = tag.getFloat(u"FallDistance");
	onFire = tag.getShort(u"Fire");
	airSupply = tag.getShort(u"Air");
	onGround = tag.getBoolean(u"OnGround");

	setPos(x, y, z);

	readAdditionalSaveData(tag);
}

void Entity::readAdditionalSaveData(CompoundTag &tag)
{

}

void Entity::addAdditionalSaveData(CompoundTag &tag)
{

}

float Entity::getShadowHeightOffs()
{
	return bbHeight / 2.0f;
}

EntityItem *Entity::spawnAtLocation(int_t itemId, int_t count)
{
	return spawnAtLocation(itemId, count, 0.0f);
}

EntityItem *Entity::spawnAtLocation(int_t itemId, int_t count, float offset)
{
	ItemStack stack(itemId, count);
	return spawnAtLocation(stack, offset);
}

EntityItem *Entity::spawnAtLocation(ItemStack &stack, float offset)
{
	EntityItem *item = new EntityItem(level, x, y + offset, z, stack);
	item->throwTime = 10;
	level.addEntity(std::shared_ptr<Entity>(item));
	return item;
}

bool Entity::isAlive()
{
	return false;
}

// Beta: Entity.isRiding() - checks if entity is riding another entity (Entity.java:990-992)
// Java: public boolean isRiding() { return this.riding != null || this.getSharedFlag(2); }
bool Entity::isRiding() const
{
	// Beta: return this.riding != null || this.getSharedFlag(2);
	// For now, just check riding - getSharedFlag(2) is for DataWatcher synchronization in multiplayer
	return riding != nullptr;
}

bool Entity::isInWall()
{
	return false;
}

bool Entity::interact(Player &player)
{
	return false;
}

AABB *Entity::getCollideAgainstBox(Entity &entity)
{
	return nullptr;
}

// Alpha 1.2.6: Entity.mountEntity() - mounts entity on another entity (riding relationship)
// Java: public void mountEntity(Entity var1) (Entity.java:876-903)
void Entity::mountEntity(Entity* vehicle)
{
	// Java: this.minecartType = 0.0D; (line 877)
	// Java: this.field_667_e = 0.0D; (line 878)
	// Note: minecartType and field_667_e don't exist in our C++ codebase (were used for minecart rotation interpolation)
	
	// Java: if(var1 == null) { (line 879)
	if (vehicle == nullptr)
	{
		// Java: if(this.ridingEntity != null) { (line 880)
		if (riding != nullptr)
		{
			// Java: this.setLocationAndAngles(this.ridingEntity.posX, this.ridingEntity.boundingBox.minY + (double)this.ridingEntity.height, this.ridingEntity.posZ, this.rotationYaw, this.rotationPitch); (line 881)
			// absMoveTo sets position and rotation (equivalent to setLocationAndAngles)
			absMoveTo(riding->x, riding->bb.y0 + static_cast<double>(riding->bbHeight), riding->z, yRot, xRot);
			
			// Java: this.ridingEntity.riddenByEntity = null; (line 882)
			riding->rider = nullptr;
		}
		
		// Java: this.ridingEntity = null; (line 885)
		riding = nullptr;
	}
	// Java: else if(this.ridingEntity == var1) { (line 886)
	else if (riding != nullptr && riding.get() == vehicle)
	{
		// Java: this.ridingEntity.riddenByEntity = null; (line 887)
		// Java: this.ridingEntity = null; (line 888)
		// Java: this.setLocationAndAngles(var1.posX, var1.boundingBox.minY + (double)var1.height, var1.posZ, this.rotationYaw, this.rotationPitch); (line 889)
			riding->rider = nullptr;
			absMoveTo(vehicle->x, vehicle->bb.y0 + static_cast<double>(vehicle->bbHeight), vehicle->z, yRot, xRot);
		riding = nullptr;
	}
	// Java: else { (line 890)
	else
	{
		// Java: if(this.ridingEntity != null) { (line 891)
		//     this.ridingEntity.riddenByEntity = null; (line 892)
		// }
		if (riding != nullptr)
		{
			riding->rider = nullptr;
		}
		
		// Java: if(var1.riddenByEntity != null) { (line 895)
		//     var1.riddenByEntity.ridingEntity = null; (line 896)
		// }
		if (vehicle->rider != nullptr)
		{
			vehicle->rider->riding = nullptr;
		}
		
		// Java: this.ridingEntity = var1; (line 899)
		// Java: var1.riddenByEntity = this; (line 900)
		// Need to get shared_ptr for vehicle - find it in level's entity set
		// Since we can't convert raw pointer to shared_ptr safely, we'll search the level's entities
		// For now, create a non-owning shared_ptr (similar to TODO pattern) - entities are owned by level anyway
		riding = std::shared_ptr<Entity>(vehicle, [](Entity*) {});  // Non-owning shared_ptr
		// Find the actual shared_ptr for vehicle from level's entities
		// Search through level entities to find matching entity
		std::shared_ptr<Entity> vehiclePtr = nullptr;
		for (const auto& entity : level.entities)
		{
			if (entity.get() == vehicle)
			{
				vehiclePtr = entity;
				break;
			}
		}
		
		// Find the actual shared_ptr for this entity (rider) from level's entities
		std::shared_ptr<Entity> riderPtr = nullptr;
		for (const auto& entity : level.entities)
		{
			if (entity.get() == this)
			{
				riderPtr = entity;
				break;
			}
		}
		
		// If we found both, use them; otherwise fall back to non-owning shared_ptrs
		if (vehiclePtr != nullptr && riderPtr != nullptr)
		{
			riding = vehiclePtr;
			vehicle->rider = riderPtr;
		}
		else
		{
			// Fallback: non-owning shared_ptrs (entities are owned by level anyway)
			riding = std::shared_ptr<Entity>(vehicle, [](Entity*) {});  // Non-owning shared_ptr
			vehicle->rider = std::shared_ptr<Entity>(this, [](Entity*) {});  // Non-owning shared_ptr for rider
		}
		// Note: positionRider() is NOT called here - it's only called from rideTick() (newb12 line 862)
	}
}

// Beta: Entity.rideTick() - handles entity riding behavior (Entity.java:854-906)
void Entity::rideTick()
{
	// Beta: if (this.riding.removed) { this.riding = null; } (Entity.java:855-857)
	if (riding->removed)
	{
		riding = nullptr;
	}
	else
	{
		// Beta: this.xd = 0.0; this.yd = 0.0; this.zd = 0.0; (Entity.java:858-860)
		xd = 0.0;
		yd = 0.0;
		zd = 0.0;
		
		// Beta: this.tick(); (Entity.java:861)
		tick();
		
		// Beta: this.riding.positionRider(); (Entity.java:862)
		riding->positionRider();
		
		// Beta: Rotation interpolation for smooth riding (Entity.java:863-904)
		// Beta: this.yRideRotA = this.yRideRotA + (this.riding.yRot - this.riding.yRotO); (Entity.java:863)
		yRideRotA = yRideRotA + (riding->yRot - riding->yRotO);
		// Beta: this.xRideRotA = this.xRideRotA + (this.riding.xRot - this.riding.xRotO); (Entity.java:864)
		xRideRotA = xRideRotA + (riding->xRot - riding->xRotO);
		
		// Beta: Normalize yRideRotA to [-180, 180) (Entity.java:866-872)
		while (yRideRotA >= 180.0)
		{
			yRideRotA -= 360.0;
		}
		
		while (yRideRotA < -180.0)
		{
			yRideRotA += 360.0;
		}
		
		// Beta: Normalize xRideRotA to [-180, 180) (Entity.java:874-880)
		while (xRideRotA >= 180.0)
		{
			xRideRotA -= 360.0;
		}
		
		while (xRideRotA < -180.0)
		{
			xRideRotA += 360.0;
		}
		
		// Beta: Apply rotation smoothing with limits (Entity.java:882-904)
		double var1 = yRideRotA * 0.5;  // Beta: double var1 = this.yRideRotA * 0.5; (Entity.java:882)
		double var3 = xRideRotA * 0.5;  // Beta: double var3 = this.xRideRotA * 0.5; (Entity.java:883)
		float var5 = 10.0f;  // Beta: float var5 = 10.0F; (Entity.java:884)
		
		// Beta: Clamp rotation deltas to [-10, 10] (Entity.java:885-899)
		if (var1 > var5)
		{
			var1 = var5;  // Beta: if (var1 > var5) { var1 = var5; } (Entity.java:885-887)
		}
		
		if (var1 < -var5)
		{
			var1 = -var5;  // Beta: if (var1 < -var5) { var1 = -var5; } (Entity.java:889-891)
		}
		
		if (var3 > var5)
		{
			var3 = var5;  // Beta: if (var3 > var5) { var3 = var5; } (Entity.java:893-895)
		}
		
		if (var3 < -var5)
		{
			var3 = -var5;  // Beta: if (var3 < -var5) { var3 = -var5; } (Entity.java:897-899)
		}
		
		// Beta: Apply rotation and reduce accumulated rotation (Entity.java:901-904)
		yRideRotA -= var1;  // Beta: this.yRideRotA -= var1; (Entity.java:901)
		xRideRotA -= var3;  // Beta: this.xRideRotA -= var3; (Entity.java:902)
		yRot = (float)(yRot + var1);  // Beta: this.yRot = (float)(this.yRot + var1); (Entity.java:903)
		xRot = (float)(xRot + var3);  // Beta: this.xRot = (float)(this.xRot + var3); (Entity.java:904)
	}
}

// Beta: Entity.positionRider() - positions rider on vehicle (Entity.java:908-910)
void Entity::positionRider()
{
	// Beta: this.rider.setPos(this.x, this.y + this.getRideHeight() + this.rider.getRidingHeight(), this.z); (Entity.java:909)
	// Note: newb12 doesn't check for null - assumes rider is not null when called
	rider->setPos(x, y + getRideHeight() + rider->getRidingHeight(), z);
}

// Beta: Entity.getRidingHeight() - returns rider's height offset (Entity.java:912-914)
double Entity::getRidingHeight()
{
	// Beta: return this.heightOffset; (Entity.java:913)
	return static_cast<double>(heightOffset);
}

// Beta: Entity.getRideHeight() - returns vehicle's ride height (Entity.java:916-918)
double Entity::getRideHeight()
{
	// Beta: return this.bbHeight * 0.75D; (Entity.java:917)
	return static_cast<double>(bbHeight) * 0.75;
}

// Beta 1.2: Entity.isRiding() - matches newb12 Entity.java:990-992 exactly
bool Entity::isRiding()
{
	// Beta: return this.riding != null || this.getSharedFlag(2); (Entity.java:991)
	return riding != nullptr || getSharedFlag(FLAG_RIDING);
}

// Alpha 1.2.6: Entity.updatePositionTracking() - default implementation does nothing
// Overridden in EntityClientPlayerMP to update multiplayer position tracking
void Entity::updatePositionTracking()
{
	// Default implementation: do nothing (for non-multiplayer entities)
}

// Beta: Entity.ride() - mounts entity on another entity using shared_ptr (Entity.java:920-946)
void Entity::ride(std::shared_ptr<Entity> vehicle)
{
	// Beta: this.xRideRotA = 0.0D; this.yRideRotA = 0.0D; (Entity.java:921-922)
	xRideRotA = 0.0;
	yRideRotA = 0.0;
	
	// Beta: if(var1 == null) { ... } (Entity.java:923)
	if (vehicle == nullptr)
	{
		// Beta: if(this.riding != null) { ... } (Entity.java:924)
		if (riding != nullptr)
		{
			// Beta: this.moveTo(this.riding.x, this.riding.bb.y0 + this.riding.bbHeight, this.riding.z, this.yRot, this.xRot); (Entity.java:925)
			// Alpha 1.2.6: Only teleport if vehicle is valid (not removed) and has valid coordinates
			// If vehicle is destroyed (removed) or has invalid coordinates, stay where we are to prevent anti-cheat kicks
			if (!riding->removed && 
			    std::isfinite(riding->x) && std::isfinite(riding->y) && std::isfinite(riding->z) &&
			    std::isfinite(riding->bb.y0) && std::isfinite(riding->bbHeight))
			{
				moveTo(riding->x, riding->bb.y0 + static_cast<double>(riding->bbHeight), riding->z, yRot, xRot);
			}
			// Beta: this.riding.rider = null; (Entity.java:926)
			riding->rider = nullptr;
		}
		
		// Beta: this.riding = null; (Entity.java:929)
		riding = nullptr;
		
		// Alpha 1.2.6: Reset velocity when dismounting to prevent speed kick
		// When dismounting, velocity should be zeroed to prevent server from thinking we're moving too fast
		xd = 0.0;
		yd = 0.0;
		zd = 0.0;
	}
	// Beta: else if(this.riding == var1) { ... } (Entity.java:930)
	else if (riding == vehicle)
	{
		// Beta: this.riding.rider = null; (Entity.java:931)
		// Beta: this.riding = null; (Entity.java:932)
		// Beta: this.moveTo(var1.x, var1.bb.y0 + (double)var1.bbHeight, var1.z, this.yRot, this.xRot); (Entity.java:933)
		riding->rider = nullptr;
		riding = nullptr;
		// Alpha 1.2.6: Only teleport if vehicle is valid (not removed) to prevent anti-cheat kicks
		// Check that coordinates are valid (finite numbers) before teleporting
		if (!vehicle->removed && 
		    std::isfinite(vehicle->x) && std::isfinite(vehicle->y) && std::isfinite(vehicle->z) &&
		    std::isfinite(vehicle->bb.y0) && std::isfinite(vehicle->bbHeight))
		{
			moveTo(vehicle->x, vehicle->bb.y0 + static_cast<double>(vehicle->bbHeight), vehicle->z, yRot, xRot);
		}
		
		// Alpha 1.2.6: Reset velocity when dismounting to prevent speed kick
		// When dismounting, velocity should be zeroed to prevent server from thinking we're moving too fast
		xd = 0.0;
		yd = 0.0;
		zd = 0.0;
	}
	// Beta: else { ... } (Entity.java:934)
	else
	{
		// Beta: if(this.riding != null) { this.riding.rider = null; } (Entity.java:935-936)
		if (riding != nullptr)
		{
			riding->rider = nullptr;
		}
		
		// Beta: if(var1.rider != null) { var1.rider.riding = null; } (Entity.java:939-940)
		if (vehicle->rider != nullptr)
		{
			vehicle->rider->riding = nullptr;
		}
		
		// Beta: this.riding = var1; var1.rider = this; (Entity.java:943-944)
		riding = vehicle;
		vehicle->rider = std::shared_ptr<Entity>(this, [](Entity*) {});  // Non-owning shared_ptr for rider
		// Note: positionRider() is NOT called here - it's only called from rideTick() (newb12 line 862)
	}
}

void Entity::lerpTo(double x, double y, double z, float yRot, float xRot, int_t steps)
{
	// Alpha 1.2.6: Entity.setPositionAndRotation2() - base implementation just sets position directly
	// Java: public void setPositionAndRotation2(double var1, double var3, double var5, float var7, float var8, int var9) {
	//     this.setPosition(var1, var3, var5);
	//     this.setRotation(var7, var8);
	// }
	// For non-living entities, setPositionAndRotation2 just sets position directly (no interpolation)
	// Living entities (Mob) override this to do interpolation
	// Update old positions for rendering interpolation before setting new position
	xOld = xo = this->x;
	yOld = yo = this->y;
	zOld = zo = this->z;
	yRotO = this->yRot;
	xRotO = this->xRot;
	
	// Alpha 1.2.6: Normalize yRotO and xRotO relative to new yRot/xRot to prevent snapping
	// This matches Entity.absMoveTo() behavior (lines 599-603)
	// Java: EntityLiving also normalizes prevRotationPitch (lines 280-286)
	// Must normalize BEFORE setting new rotation to prevent 180-degree snaps
	double dyRot = (yRotO - yRot);
	if (dyRot < -180.0)
		yRotO += 360.0f;
	if (dyRot >= 180.0)
		yRotO -= 360.0f;
	
	double dxRot = (xRotO - xRot);
	if (dxRot < -180.0)
		xRotO += 360.0f;
	if (dxRot >= 180.0)
		xRotO -= 360.0f;
	
	setPos(x, y, z);
	setRot(yRot, xRot);
}

float Entity::getPickRadius()
{
	return 0.0f;
}

Vec3 *Entity::getLookAngle()
{
	return nullptr;
}

void Entity::handleInsidePortal()
{

}

void Entity::lerpMotion(double x, double y, double z)
{
	xd = x;
	yd = y;
	zd = z;
	if (xRotO == 0.0f && yRotO == 0.0f)
	{
		float var7 = Mth::sqrt(x * x + z * z);
		yRotO = yRot = (float)(atan2(x, z) * 180.0 / Mth::PI);
		xRotO = xRot = (float)(atan2(y, var7) * 180.0 / Mth::PI);
	}
}

void Entity::handleEntityEvent(byte_t event)
{

}

void Entity::animateHurt()
{

}

void Entity::prepareCustomTextures()
{

}

// Beta: Entity.getEquipmentSlots() - base implementation returns nullptr (Entity.java:979-981)
// Subclasses like Mob override this to return actual equipment array
// Implementation is in header (returns nullptr)

void Entity::setEquippedSlot(int_t slot, int_t itemId, int_t auxValue)
{

}

// This method signature seems unused - the const version in Entity.h is used
// Removed unused non-const isOnFire() - use the const version in Entity.h instead

// Alpha 1.2.6: Entity.isSneaking() - reads sneaking flag from DataWatcher
// Java: public boolean isSneaking() {
//       return this.getEntityFlag(1); }
// Java: getEntityFlag() reads from DataWatcher shared flags
bool Entity::isSneaking()
{
	return getSharedFlag(FLAG_SNEAKING);
}

// Alpha 1.2.6: Entity.setSneaking() - writes sneaking flag to DataWatcher
// Java: public void setSneaking(boolean var1) {
//       this.setEntityFlag(1, var1); }
// Java: setEntityFlag() writes to DataWatcher shared flags
void Entity::setSneaking(bool sneaking)
{
	setSharedFlag(FLAG_SNEAKING, sneaking);
}

// Alpha 1.2.6: Entity.getSharedFlag() - reads flag from DataWatcher
// Java: public boolean getSharedFlag(int var1) {
//       return (this.dataWatcher.getWatchableObjectByte(0) & 1 << var1) != 0; }
bool Entity::getSharedFlag(int_t flag) const
{
	if (flag < 0 || flag > 7)
		return false;
	
	try
	{
		byte_t flags = dataWatcher.getWatchableObjectByte(DATA_SHARED_FLAGS_ID);
		return (flags & (1 << flag)) != 0;
	}
	catch (...)
	{
		return false;
	}
}

// Beta: isOnFire() - checks if entity is on fire (Entity.java uses onFire > 0)
// In multiplayer, also check DataWatcher flag since onFire is reset to 0 in baseTick()
bool Entity::isOnFire() const
{
	if (level.isOnline)
	{
		// In multiplayer, check DataWatcher flag (server controls this)
		return getSharedFlag(FLAG_ONFIRE);
	}
	return onFire > 0;
}

// Alpha 1.2.6: Entity.setSharedFlag() - writes flag to DataWatcher
// Java: public void setSharedFlag(int var1, boolean var2) {
//       byte var3 = this.dataWatcher.getWatchableObjectByte(0);
//       this.dataWatcher.updateObject(0, Byte.valueOf((byte)(var3 | 1 << var1))); }
void Entity::setSharedFlag(int_t flag, bool value)
{
	if (flag < 0 || flag > 7)
		return;
	
	try
	{
		byte_t flags = dataWatcher.getWatchableObjectByte(DATA_SHARED_FLAGS_ID);
		if (value)
		{
			flags |= (1 << flag);
		}
		else
		{
			flags &= ~(1 << flag);
		}
		dataWatcher.updateObject(DATA_SHARED_FLAGS_ID, flags);
	}
	catch (...)
	{
		// DataWatcher might not have the object yet, add it
		dataWatcher.addObject(DATA_SHARED_FLAGS_ID, static_cast<byte_t>(value ? (1 << flag) : 0));
	}
}