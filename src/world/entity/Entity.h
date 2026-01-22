#pragma once

#include <memory>

#include "world/phys/AABB.h"
#include "world/entity/EntityPos.h"
#include "world/entity/EntityIO.h"
#include "world/entity/DataWatcher.h"

#include "nbt/CompoundTag.h"
#include "nbt/ListTag.h"
#include "nbt/FloatTag.h"
#include "nbt/DoubleTag.h"

#include "java/Type.h"
#include "java/Random.h"
#include "java/String.h"

#include "util/Memory.h"

class Level;
class Material;
class Player;
class EntityItem;
class ItemStack;

class Entity
{
public:
	virtual jstring getEncodeId() const { return u""; }
	
	// Alpha 1.2.6: Entity.getDataWatcher() - returns DataWatcher for metadata synchronization
	// Java: public DataWatcher getDataWatcher() { return this.dataWatcher; }
	DataWatcher& getDataWatcher() { return dataWatcher; }
	const DataWatcher& getDataWatcher() const { return dataWatcher; }

public:
	static constexpr int_t TOTAL_AIR_SUPPLY = 300;

private:
	static int_t entityCounter;
public:
	int_t entityId = entityCounter++;

	double viewScale = 1.0;
	bool blocksBuilding = false;

	std::shared_ptr<Entity> rider;
	std::shared_ptr<Entity> riding;

	Level &level;

	double xo = 0.0, yo = 0.0, zo = 0.0;
	double x = 0.0, y = 0.0, z = 0.0;
	double xd = 0.0, yd = 0.0, zd = 0.0;

	float yRot = 0.0, xRot = 0.0;
	float yRotO = 0.0, xRotO = 0.0;

	AABB bb = AABB(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	bool onGround = false;
	bool horizontalCollision = false;
	bool verticalCollision = false;
	bool collision = false;
	bool hurtMarked = false;
	bool slide = true;
	bool removed = false;

	float heightOffset = 0.0f;
	
	// Alpha 1.2.6: field_9287_aY - vertical bobbing/offset field (Entity.java:46)
	// Used for movement bobbing calculations, reset to 0.0F in NetClientHandler::handleFlying
	float field_9287_aY = 0.0f;

	float bbWidth = 0.6f, bbHeight = 1.8f;

	float walkDistO = 0.0f;
	float walkDist = 0.0f;

protected:
	bool makeStepSound = true;
	float fallDistance = 0.0f;

private:
	int_t nextStep = 1;

public:
	double xOld = 0.0, yOld = 0.0, zOld = 0.0;

	float ySlideOffset = 0.0f;
	float footSize = 0.0f;

	bool noPhysics = false;

	float pushthrough = 0.0f;

	bool hovered = false;

protected:
	Random random = Random();

public:
	int_t tickCount = 0;

	int_t flameTime = 1;
	int_t onFire = 0;

protected:
	int_t airCapacity = TOTAL_AIR_SUPPLY;
	bool wasInWater = false;
	
public:
	int_t invulnerableTime = 0;
	int_t airSupply = TOTAL_AIR_SUPPLY;

private:
	bool firstTick = true;

public:
	jstring customTextureUrl;
	jstring customTextureUrl2;

protected:
	bool fireImmune = false;
	// Alpha 1.2.6: DataWatcher for entity metadata synchronization
	// Java: protected DataWatcher dataWatcher = new DataWatcher();
	DataWatcher dataWatcher;

private:
	static constexpr int_t DATA_SHARED_FLAGS_ID = 0;

public:
	// Alpha 1.2.6: Shared flags for DataWatcher (accessible for network handlers)
	static constexpr int_t FLAG_ONFIRE = 0;
	static constexpr int_t FLAG_SNEAKING = 1;
	static constexpr int_t FLAG_RIDING = 2;

private:

	double xRideRotA = 0.0, yRideRotA = 0.0;

public:
	bool inChunk = false;
	int_t xChunk = 0, yChunk = 0, zChunk = 0;

	int_t xp = 0, yp = 0, zp = 0;

	// Alpha 1.2.6: Encoded position fields (multiplied by 32) for network synchronization
	// Java: public int field_9303_br; public int field_9302_bs; public int field_9301_bt;
	// These store encoded positions (pos * 32) to accumulate relative movement bytes
	int_t field_9303_br = 0;  // Encoded X position (x * 32)
	int_t field_9302_bs = 0;  // Encoded Y position (y * 32)
	int_t field_9301_bt = 0;  // Encoded Z position (z * 32)

	Entity(Level &level);

	virtual ~Entity() {}

protected:
	// Alpha 1.2.6: Entity.entityInit() - abstract method called from constructor
	// In our codebase, this is defineSynchedData() (matches newb12 naming)
	// Default implementation is empty - subclasses override to define their metadata
	virtual void defineSynchedData() {}

	virtual void resetPos();

public:
	void remove();

protected:
	virtual void setSize(float width, float height);
	void setPos(const EntityPos &pos);
	void setRot(float yRot, float xRot);

public:
	void setPos(double x, double y, double z);
	void turn(float yRot, float xRot);
	void interpolateTurn(float yRot, float xRot);

	virtual void tick();
	virtual void baseTick();

protected:
	void lavaHurt();
	virtual void outOfWorld();

public:
	bool isFree(double xd, double yd, double zd, double skin);
	bool isFree(double xd, double yd, double zd);
	void move(double xd, double yd, double zd);

protected:
	void checkFallDamage(double yd, bool onGround);

public:
	virtual AABB *getCollideBox();

protected:
	void burn(int_t a0);

	virtual void causeFallDamage(float distance);

public:
	bool isInWater();
	bool isUnderLiquid(const Material &material);
	bool isFullySubmerged();

	virtual float getHeadHeight();

	bool isInLava();
	
	// Alpha 1.2.6: Entity.setVelocity() - sets entity velocity (motionX, motionY, motionZ)
	// Java: public void setVelocity(double var1, double var3, double var5) {
	//       this.motionX = var1; this.motionY = var3; this.motionZ = var5; }
	void setVelocity(double motionX, double motionY, double motionZ);
	
	// Alpha 1.2.6: Entity.func_9282_a() - handles entity status from Packet38
	// Java: public void func_9282_a(byte var1) { }
	// Called when Packet38 (Entity Status) is received
	virtual void func_9282_a(byte_t status);
	
	// Alpha 1.2.6: Entity.func_9280_g() - triggers hurt animation
	// Java: public void func_9280_g() { }
	virtual void func_9280_g();
	
	// Beta: isOnFire() - checks if entity is on fire (Entity.java uses onFire > 0)
	// In multiplayer, also check DataWatcher flag since onFire is reset to 0 in baseTick()
	bool isOnFire() const;
	
	// Beta: isRiding() - checks if entity is riding another entity (Entity.java:990-992)
	// Java: public boolean isRiding() { return this.riding != null || this.getSharedFlag(2); }
	bool isRiding() const;

	// Alpha 1.2.6: updatePositionTracking() - updates position tracking fields after positionRider()
	// This is overridden in EntityClientPlayerMP to update multiplayer position tracking
	// Default implementation does nothing (for non-multiplayer entities)
	virtual void updatePositionTracking();

	void moveRelative(float x, float z, float acc);

	float getBrightness(float a);

	void absMoveTo(double x, double y, double z, float yRot, float xRot);
	void moveTo(double x, double y, double z, float yRot, float xRot);

	float distanceTo(const Entity &entity);
	double distanceToSqr(double x, double y, double z);
	double distanceTo(double x, double y, double z);
	double distanceToSqr(const Entity &entity);

	virtual void playerTouch(Player &player);

	void push(Entity &entity);
	void push(double x, double y, double z);

protected:
	void markHurt();

public:
	virtual bool hurt(Entity *source, int_t dmg);

	bool intersects(double x0, double y0, double z0, double x1, double y1, double z1);

	virtual bool isPickable();
	virtual bool isPushable();
	virtual bool isShootable();

	virtual void awardKillScore(Entity &source, int_t dmg);

	bool shouldRender(Vec3 &z);
	virtual bool shouldRenderAtSqrDistance(double distance);

	virtual jstring getTexture();

	virtual bool isCreativeModeAllowed();

	bool save(CompoundTag &tag);
	void saveWithoutId(CompoundTag &tag);
	void load(CompoundTag &tag);

protected:
	virtual void readAdditionalSaveData(CompoundTag &tag);
	virtual void addAdditionalSaveData(CompoundTag &tag);

public:
	float getShadowHeightOffs();

	EntityItem *spawnAtLocation(int_t itemId, int_t count);
	EntityItem *spawnAtLocation(int_t itemId, int_t count, float offset);
	EntityItem *spawnAtLocation(ItemStack &stack, float offset);

	virtual bool isAlive();
	bool isInWall();

	virtual bool interact(Player &player);
	virtual AABB *getCollideAgainstBox(Entity &entity);

	virtual void rideTick();
	void positionRider();
	virtual double getRidingHeight();
	virtual double getRideHeight();

	void ride(std::shared_ptr<Entity> entity);
	
	// Alpha 1.2.6: Entity.mountEntity() - mounts entity on another entity (riding relationship)
	// Java: public void mountEntity(Entity var1) (Entity.java:876-903)
	void mountEntity(Entity* vehicle);

	virtual void lerpTo(double x, double y, double z, float yRot, float xRot, int_t steps);

	virtual float getPickRadius();
	virtual Vec3 *getLookAngle();

	virtual void handleInsidePortal();
	void lerpMotion(double x, double y, double z);

	virtual void handleEntityEvent(byte_t event);
	virtual void animateHurt();
	virtual void prepareCustomTextures();

	// Beta: Entity.getEquipmentSlots() - returns equipment array (null in base Entity) (Entity.java:979-981)
	// Returns pointer to ItemStack array (nullptr in base Entity, subclasses like Mob override this)
	virtual ItemStack* getEquipmentSlots() { return nullptr; }

	void setEquippedSlot(int_t slot, int_t itemId, int_t auxValue);

	bool isRiding();
	virtual bool isSneaking();

	void setSneaking(bool sneaking);

protected:
	bool getSharedFlag(int_t flag) const;
	void setSharedFlag(int_t flag, bool value);

public:
	virtual bool isPlayer() { return false; }
};
