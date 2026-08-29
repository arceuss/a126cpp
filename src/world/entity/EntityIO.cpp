#include "world/entity/EntityIO.h"

#include <typeindex>
#include <unordered_map>

#include "util/Memory.h"
#include "world/entity/Entity.h"
#include "world/entity/Mob.h"
#include "world/entity/monster/Monster.h"
#include "world/entity/monster/Creeper.h"
#include "world/entity/monster/Skeleton.h"
#include "world/entity/monster/Spider.h"
#include "world/entity/monster/Giant.h"
#include "world/entity/monster/Zombie.h"
#include "world/entity/monster/Slime.h"
#include "world/entity/monster/Ghast.h"
#include "world/entity/monster/PigZombie.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/animal/Sheep.h"
#include "world/entity/animal/Cow.h"
#include "world/entity/animal/Chicken.h"
#include "world/entity/projectile/Arrow.h"
#include "world/entity/projectile/Snowball.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/Painting.h"
#include "world/entity/PrimedTnt.h"
#include "world/entity/item/FallingTile.h"
#include "world/entity/item/Boat.h"
#include "world/entity/item/Minecart.h"

namespace EntityIO
{

#define ENTITYIO_ID(type, name, id) { name, [](Level &level) -> std::shared_ptr<Entity> { return Util::make_shared<type>(level); } },
static std::unordered_map<jstring, std::shared_ptr<Entity>(*)(Level &level)> idClassMap = {
#include "world/entity/EntityIDs.h"
};
#undef ENTITYIO_ID

#define ENTITYIO_ID(type, name, id) { id, [](Level &level) -> std::shared_ptr<Entity> { return Util::make_shared<type>(level); } },
static std::unordered_map<int_t, std::shared_ptr<Entity>(*)(Level &level)> numClassMap = {
#include "world/entity/EntityIDs.h"
};
#undef ENTITYIO_ID

// Alpha keys these by the entity's exact class, so a subclass that is not
// registered has no identity of its own and inherits none.
#define ENTITYIO_ID(type, name, id) { std::type_index(typeid(type)), name },
static const std::unordered_map<std::type_index, jstring> classToIdMap = {
#include "world/entity/EntityIDs.h"
};
#undef ENTITYIO_ID

#define ENTITYIO_ID(type, name, id) { std::type_index(typeid(type)), id },
static const std::unordered_map<std::type_index, int_t> classToNumMap = {
#include "world/entity/EntityIDs.h"
};
#undef ENTITYIO_ID

jstring getEncodeId(const Entity &entity)
{
	auto it = classToIdMap.find(std::type_index(typeid(entity)));
	if (it == classToIdMap.end())
		return u"";
	return it->second;
}

int_t getEncodeNumericId(const Entity &entity)
{
	auto it = classToNumMap.find(std::type_index(typeid(entity)));
	if (it == classToNumMap.end())
		return -1;
	return it->second;
}

std::shared_ptr<Entity> loadStatic(CompoundTag &tag, Level &level)
{
	try
	{
		std::shared_ptr<Entity> entity = idClassMap.at(tag.getString(u"id"))(level);
		entity->load(tag);
		return entity;
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}
	return nullptr;
}

// Beta: EntityIO.newEntity() - creates entity from string ID (EntityIO.java:42-55)
std::shared_ptr<Entity> newEntity(const jstring &id, Level &level)
{
	try
	{
		auto it = idClassMap.find(id);
		if (it != idClassMap.end())
		{
			return it->second(level);
		}
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}
	return nullptr;
}

// Alpha 1.2.6: EntityList.createEntity() - creates entity from int ID (EntityList.java:55-72)
// Used for Packet24MobSpawn
std::shared_ptr<Entity> createEntity(int_t id, Level &level)
{
	try
	{
		auto it = numClassMap.find(id);
		if (it != numClassMap.end())
		{
			return it->second(level);
		}
		else
		{
			std::cout << "Skipping Entity with id " << id << std::endl;
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "Error creating entity with id " << id << ": " << e.what() << '\n';
	}
	return nullptr;
}

}
