#include "world/level/pathfinder/PathFinder.h"
#include "world/level/material/Material.h"
#include "world/level/material/LiquidMaterial.h"
#include "util/Mth.h"
#include <limits>

// newb12: PathFinder constructor
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:15-17
PathFinder::PathFinder(LevelSource* level)
	: level(level)
{
	neighbors.resize(32);
}

// newb12: PathFinder.findPath(Entity, Entity, float)
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:19-21
std::unique_ptr<pathfinder::Path> PathFinder::findPath(Entity& entity, Entity& target, float maxDistance)
{
	return findPath(entity, target.x, target.bb.y0, target.z, maxDistance);
}

// newb12: PathFinder.findPath(Entity, int, int, int, float)
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:23-25
std::unique_ptr<pathfinder::Path> PathFinder::findPath(Entity& entity, int_t x, int_t y, int_t z, float maxDistance)
{
	return findPath(entity, (double)(x + 0.5F), (double)(y + 0.5F), (double)(z + 0.5F), maxDistance);
}

// newb12: PathFinder.findPath(Entity, double, double, double, float) - private helper
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:27-34
std::unique_ptr<pathfinder::Path> PathFinder::findPath(Entity& entity, double targetX, double targetY, double targetZ, float maxDistance)
{
	openSet.clear();
	nodes.clear();
	std::shared_ptr<pathfinder::Node> start = getNode(Mth::floor(entity.bb.x0), Mth::floor(entity.bb.y0), Mth::floor(entity.bb.z0));
	std::shared_ptr<pathfinder::Node> end = getNode(Mth::floor(targetX - entity.bbWidth / 2.0F), Mth::floor(targetY), Mth::floor(targetZ - entity.bbWidth / 2.0F));
	std::shared_ptr<pathfinder::Node> size = std::make_shared<pathfinder::Node>(Mth::floor(entity.bbWidth + 1.0F), Mth::floor(entity.bbHeight + 1.0F), Mth::floor(entity.bbWidth + 1.0F));
	return findPath(entity, start, end, size, maxDistance);
}

// newb12: PathFinder.findPath(Entity, Node, Node, Node, float) - main A* algorithm
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:36-75
std::unique_ptr<pathfinder::Path> PathFinder::findPath(Entity& entity, std::shared_ptr<pathfinder::Node> start, std::shared_ptr<pathfinder::Node> end, std::shared_ptr<pathfinder::Node> size, float maxDistance)
{
	start->g = 0.0F;
	start->h = start->distanceTo(end);
	start->f = start->h;
	openSet.clear();
	openSet.insert(start);
	std::shared_ptr<pathfinder::Node> closest = start;

	while (!openSet.isEmpty())
	{
		std::shared_ptr<pathfinder::Node> current = openSet.pop();
		if (current->hash == end->hash)
		{
			return reconstruct_path(start, end);
		}

		if (current->distanceTo(end) < closest->distanceTo(end))
		{
			closest = current;
		}

		current->closed = true;
		int_t neighborCount = getNeighbors(entity, current, size, end, maxDistance);

		for (int_t i = 0; i < neighborCount; i++)
		{
			std::shared_ptr<pathfinder::Node> neighbor = neighbors[i];
			float newG = current->g + current->distanceTo(neighbor);
			if (!neighbor->inOpenSet() || newG < neighbor->g)
			{
				neighbor->cameFrom = current;
				neighbor->g = newG;
				neighbor->h = neighbor->distanceTo(end);
				if (neighbor->inOpenSet())
				{
					openSet.changeCost(neighbor, neighbor->g + neighbor->h);
				}
				else
				{
					neighbor->f = neighbor->g + neighbor->h;
					openSet.insert(neighbor);
				}
			}
		}
	}

	return closest == start ? nullptr : reconstruct_path(start, closest);
}

// newb12: PathFinder.getNeighbors() - gets walkable neighbors
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:77-105
int_t PathFinder::getNeighbors(Entity& entity, std::shared_ptr<pathfinder::Node> node, std::shared_ptr<pathfinder::Node> size, std::shared_ptr<pathfinder::Node> target, float maxDistance)
{
	int_t count = 0;
	int_t stepHeight = 0;
	if (isFree(entity, node->x, node->y + 1, node->z, size) > 0)
	{
		stepHeight = 1;
	}

	std::shared_ptr<pathfinder::Node> neighborZPos = getNode(entity, node->x, node->y, node->z + 1, size, stepHeight);
	std::shared_ptr<pathfinder::Node> neighborXNeg = getNode(entity, node->x - 1, node->y, node->z, size, stepHeight);
	std::shared_ptr<pathfinder::Node> neighborXPos = getNode(entity, node->x + 1, node->y, node->z, size, stepHeight);
	std::shared_ptr<pathfinder::Node> neighborZNeg = getNode(entity, node->x, node->y, node->z - 1, size, stepHeight);
	
	if (neighborZPos != nullptr && !neighborZPos->closed && neighborZPos->distanceTo(target) < maxDistance)
	{
		neighbors[count++] = neighborZPos;
	}

	if (neighborXNeg != nullptr && !neighborXNeg->closed && neighborXNeg->distanceTo(target) < maxDistance)
	{
		neighbors[count++] = neighborXNeg;
	}

	if (neighborXPos != nullptr && !neighborXPos->closed && neighborXPos->distanceTo(target) < maxDistance)
	{
		neighbors[count++] = neighborXPos;
	}

	if (neighborZNeg != nullptr && !neighborZNeg->closed && neighborZNeg->distanceTo(target) < maxDistance)
	{
		neighbors[count++] = neighborZNeg;
	}

	return count;
}

// newb12: PathFinder.getNode(Entity, int, int, int, Node, int) - gets walkable node
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:107-138
std::shared_ptr<pathfinder::Node> PathFinder::getNode(Entity& entity, int_t x, int_t y, int_t z, std::shared_ptr<pathfinder::Node> size, int_t stepHeight)
{
	std::shared_ptr<pathfinder::Node> node = nullptr;
	if (isFree(entity, x, y, z, size) > 0)
	{
		node = getNode(x, y, z);
	}

	if (node == nullptr && isFree(entity, x, y + stepHeight, z, size) > 0)
	{
		node = getNode(x, y + stepHeight, z);
		y += stepHeight;
	}

	if (node != nullptr)
	{
		int_t dropCount = 0;

		int_t freeResult;
		for (bool var9 = false; y > 0 && (freeResult = isFree(entity, x, y - 1, z, size)) > 0; y--)
		{
			if (freeResult < 0)
			{
				return nullptr;
			}

			if (++dropCount >= 4)
			{
				return nullptr;
			}
		}

		if (y > 0)
		{
			node = getNode(x, y, z);
		}
	}

	return node;
}

// newb12: PathFinder.getNode(int, int, int) - gets or creates node
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:140-149
std::shared_ptr<pathfinder::Node> PathFinder::getNode(int_t x, int_t y, int_t z)
{
	int_t hash = x | (y << 10) | (z << 20);
	std::shared_ptr<pathfinder::Node> node = nodes.get(hash);
	if (node == nullptr)
	{
		node = std::make_shared<pathfinder::Node>(x, y, z);
		nodes.put(hash, node);
	}

	return node;
}

// newb12: PathFinder.isFree() - checks if position is walkable
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:151-168
// Note: Java code uses var2, var3, var4 (original params) instead of loop vars - matches Java exactly
int_t PathFinder::isFree(Entity& entity, int_t x, int_t y, int_t z, std::shared_ptr<pathfinder::Node> size)
{
	for (int_t dx = x; dx < x + size->x; dx++)
	{
		for (int_t dy = y; dy < y + size->y; dy++)
		{
			for (int_t dz = z; dz < z + size->z; dz++)
			{
				const Material& material = level->getMaterial(x, y, z);
				if (material.blocksMotion())
				{
					return 0;
				}

				if (material == Material::water || material == Material::lava)
				{
					return -1;
				}
			}
		}
	}

	return 1;
}

// newb12: PathFinder.reconstruct_path() - reconstructs path from start to end
// Reference: newb12/net/minecraft/world/level/pathfinder/PathFinder.java:170-185
std::unique_ptr<pathfinder::Path> PathFinder::reconstruct_path(std::shared_ptr<pathfinder::Node> start, std::shared_ptr<pathfinder::Node> end)
{
	int_t pathLength = 1;

	for (std::shared_ptr<pathfinder::Node> node = end; node->cameFrom != nullptr; node = node->cameFrom)
	{
		pathLength++;
	}

	std::vector<std::shared_ptr<pathfinder::Node>> pathNodes(pathLength);
	std::shared_ptr<pathfinder::Node> current = end;

	pathNodes[--pathLength] = end;
	for (; current->cameFrom != nullptr; pathNodes[--pathLength] = current)
	{
		current = current->cameFrom;
	}

	return std::make_unique<pathfinder::Path>(std::move(pathNodes));
}