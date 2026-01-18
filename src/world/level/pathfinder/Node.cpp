#include "world/level/pathfinder/Node.h"
#include "util/Mth.h"
#include "java/String.h"

namespace pathfinder
{
	// newb12: Node constructor
	// Reference: newb12/net/minecraft/world/level/pathfinder/Node.java:17-22
	Node::Node(int_t x, int_t y, int_t z)
		: x(x), y(y), z(z), hash(x | (y << 10) | (z << 20))
	{
	}

	// newb12: Node.distanceTo()
	// Reference: newb12/net/minecraft/world/level/pathfinder/Node.java:24-29
	float Node::distanceTo(std::shared_ptr<Node> other)
	{
		float var2 = other->x - this->x;
		float var3 = other->y - this->y;
		float var4 = other->z - this->z;
		return Mth::sqrt(var2 * var2 + var3 * var3 + var4 * var4);
	}

	// newb12: Node.equals()
	// Reference: newb12/net/minecraft/world/level/pathfinder/Node.java:32-34
	bool Node::equals(std::shared_ptr<Node> other) const
	{
		return other->hash == this->hash;
	}

	// newb12: Node.inOpenSet()
	// Reference: newb12/net/minecraft/world/level/pathfinder/Node.java:41-43
	bool Node::inOpenSet() const
	{
		return this->heapIdx >= 0;
	}
}
