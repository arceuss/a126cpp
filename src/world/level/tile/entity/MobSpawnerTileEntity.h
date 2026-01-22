#pragma once

#include "world/level/tile/entity/TileEntity.h"
#include "java/String.h"

// Beta 1.2 MobSpawnerTileEntity
// Reference: newb12/net/minecraft/world/level/tile/entity/MobSpawnerTileEntity.java
class MobSpawnerTileEntity : public TileEntity
{
private:
	// Beta: MAX_DIST = 16 (MobSpawnerTileEntity.java:10)
	static constexpr int_t MAX_DIST = 16;
	
	// Beta: private String entityId (MobSpawnerTileEntity.java:12)
	jstring entityId;
	
public:
	// Beta: spawnDelay = -1 (MobSpawnerTileEntity.java:11)
	int_t spawnDelay = -1;
	
	// Beta: double spin, oSpin = 0.0 (MobSpawnerTileEntity.java:13-14)
	double spin = 0.0;
	double oSpin = 0.0;

public:
	MobSpawnerTileEntity();
	
	virtual jstring getEncodeId() const override { return u"MobSpawner"; }
	
	// Beta: getEntityId() - returns entityId (MobSpawnerTileEntity.java:21-23)
	jstring getEntityId() const { return entityId; }
	
	// Beta: setEntityId() - sets entityId (MobSpawnerTileEntity.java:25-27)
	void setEntityId(const jstring &id) { entityId = id; }
	
	// Beta: isNearPlayer() - checks if player is within 16 blocks (MobSpawnerTileEntity.java:29-31)
	bool isNearPlayer();
	
	virtual void tick() override;
	
	virtual void load(CompoundTag &tag) override;
	virtual void save(CompoundTag &tag) override;

private:
	// Beta: delay() - sets spawnDelay = 200 + random(600) (MobSpawnerTileEntity.java:99-101)
	void delay();
};
