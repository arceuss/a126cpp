#pragma once

#include "world/entity/Mob.h"
#include "world/entity/Entity.h"
#include <memory>

// Forward declaration for Path class
namespace pathfinder { class Path; }

// newb12: PathfinderMob - base class for mobs with pathfinding
// Reference: newb12/net/minecraft/world/entity/PathfinderMob.java
class PathfinderMob : public Mob
{
protected:
	static constexpr int_t MAX_TURN = 30;  // newb12: PathfinderMob.MAX_TURN = 30 (PathfinderMob.java:9)
	
	std::unique_ptr<pathfinder::Path> path;  // newb12: PathfinderMob.path (PathfinderMob.java:10)
	std::shared_ptr<Entity> attackTarget;  // newb12: PathfinderMob.attackTarget (PathfinderMob.java:11)
	bool holdGround = false;  // newb12: PathfinderMob.holdGround (PathfinderMob.java:12)

public:
	PathfinderMob(Level &level);

protected:
	virtual void updateAi() override;

	// newb12: PathfinderMob.checkHurtTarget()
	virtual void checkHurtTarget(Entity &target, float distance);

	// newb12: PathfinderMob.getWalkTargetValue()
	virtual float getWalkTargetValue(int_t x, int_t y, int_t z);

	// newb12: PathfinderMob.findAttackTarget()
	virtual std::shared_ptr<Entity> findAttackTarget();

public:
	virtual bool canSpawn() override;
};
