#pragma once

#include "world/level/pathfinder/Node.h"
#include "world/entity/Entity.h"
#include "world/phys/Vec3.h"
#include <memory>
#include <vector>

// newb12: Path - represents a pathfinding path
// Reference: newb12/net/minecraft/world/level/pathfinder/Path.java
namespace pathfinder
{
	class Path
	{
	private:
		std::vector<std::shared_ptr<Node>> nodes;
		int_t pos;

	public:
		const int_t length;

		// newb12: Path constructor takes Node array (Path.java:11-14)
		Path(std::vector<std::shared_ptr<Node>> nodes);

		std::shared_ptr<Node> current();
		void next();
		bool isDone();
		std::shared_ptr<Node> get(int_t index);
		Vec3* current(Entity &entity);
	};
}
