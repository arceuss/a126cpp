#include "world/level/pathfinder/Path.h"
#include "world/entity/Entity.h"
#include "world/phys/Vec3.h"

namespace pathfinder
{
	// newb12: Path constructor takes Node array (Path.java:11-14)
	Path::Path(std::vector<std::shared_ptr<Node>> nodes)
		: nodes(std::move(nodes)), length(this->nodes.size()), pos(0)
	{
	}

	// newb12: Path.current() - gets current node (Path.java:16-18)
	std::shared_ptr<Node> Path::current()
	{
		return nodes[pos];
	}

	// newb12: Path.next() - advances to next node (Path.java:20-22)
	void Path::next()
	{
		pos++;
	}

	// newb12: Path.isDone() - checks if path is complete (Path.java:24-26)
	bool Path::isDone()
	{
		return pos >= nodes.size();
	}

	// newb12: Path.get() - gets node at index (Path.java:28-30)
	std::shared_ptr<Node> Path::get(int_t index)
	{
		return nodes[index];
	}

	// newb12: Path.current() - gets current path position as Vec3 (Path.java:32-37)
	Vec3* Path::current(Entity &entity)
	{
		double var2 = nodes[pos]->x + (int_t)(entity.bbWidth + 1.0F) * 0.5;
		double var4 = nodes[pos]->y;
		double var6 = nodes[pos]->z + (int_t)(entity.bbWidth + 1.0F) * 0.5;
		return Vec3::newTemp(var2, var4, var6);
	}
}
