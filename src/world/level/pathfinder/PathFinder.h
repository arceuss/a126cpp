#pragma once

#include "world/level/LevelSource.h"
#include "world/level/pathfinder/BinaryHeap.h"
#include "world/level/pathfinder/Path.h"
#include "world/level/pathfinder/Node.h"
#include "util/IntHashMap.h"
#include "world/entity/Entity.h"
#include <memory>
#include <vector>

// newb12: PathFinder - A* pathfinding algorithm
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java
class PathFinder
{
private:
	LevelSource* level;
	BinaryHeap openSet;
	IntHashMap<std::shared_ptr<pathfinder::Node>> nodes;
	std::vector<std::shared_ptr<pathfinder::Node>> neighbors;

	std::shared_ptr<pathfinder::Node> getNode(int_t x, int_t y, int_t z);
	std::shared_ptr<pathfinder::Node> getNode(Entity& entity, int_t x, int_t y, int_t z, std::shared_ptr<pathfinder::Node> size, int_t stepHeight);
	int_t isFree(Entity& entity, int_t x, int_t y, int_t z, std::shared_ptr<pathfinder::Node> size);
	int_t getNeighbors(Entity& entity, std::shared_ptr<pathfinder::Node> node, std::shared_ptr<pathfinder::Node> size, std::shared_ptr<pathfinder::Node> target, float maxDistance);
	std::unique_ptr<pathfinder::Path> findPath(Entity& entity, double targetX, double targetY, double targetZ, float maxDistance);
	std::unique_ptr<pathfinder::Path> findPath(Entity& entity, std::shared_ptr<pathfinder::Node> start, std::shared_ptr<pathfinder::Node> end, std::shared_ptr<pathfinder::Node> size, float maxDistance);
	std::unique_ptr<pathfinder::Path> reconstruct_path(std::shared_ptr<pathfinder::Node> start, std::shared_ptr<pathfinder::Node> end);

public:
	PathFinder(LevelSource* level);

	std::unique_ptr<pathfinder::Path> findPath(Entity& entity, Entity& target, float maxDistance);
	std::unique_ptr<pathfinder::Path> findPath(Entity& entity, int_t x, int_t y, int_t z, float maxDistance);
};