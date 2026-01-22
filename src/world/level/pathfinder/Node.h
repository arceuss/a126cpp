#pragma once

#include "java/Type.h"
#include <memory>

// newb12: Node - pathfinding node
// Reference: newb12/net/minecraft/world/level/pathfinder/Node.java
namespace pathfinder
{
	class Node
	{
	public:
		const int_t x;
		const int_t y;
		const int_t z;
		const int_t hash;
		
		int_t heapIdx = -1;
		float g = 0.0F;
		float h = 0.0F;
		float f = 0.0F;
		std::shared_ptr<Node> cameFrom;
		bool closed = false;

	public:
		Node(int_t x, int_t y, int_t z);
		
		float distanceTo(std::shared_ptr<Node> other);
		bool equals(std::shared_ptr<Node> other) const;
		bool inOpenSet() const;
	};
}