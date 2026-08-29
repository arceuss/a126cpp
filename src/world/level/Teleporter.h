#pragma once

#include "java/Random.h"

class Entity;
class Level;

// Alpha 1.2.6: Teleporter - places an entity at a nether portal, building one
// when the destination has none (Teleporter.java:12-229).
class Teleporter
{
private:
	Random random;

public:
	// Alpha: Teleporter.func_4107_a (Teleporter.java:15-21)
	void teleport(Level &level, Entity &entity);

	// Alpha: Teleporter.func_4106_b (Teleporter.java:23-79)
	bool findPortal(Level &level, Entity &entity);

	// Alpha: Teleporter.func_4108_c (Teleporter.java:81-229)
	bool createPortal(Level &level, Entity &entity);
};
