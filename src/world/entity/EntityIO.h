#pragma once

#include <memory>

#include "nbt/CompoundTag.h"

#include "java/Type.h"
#include "java/String.h"

#include "util/Memory.h"

class Entity;
class Level;

class EntityIO_detail;

namespace EntityIO
{

std::shared_ptr<Entity> loadStatic(CompoundTag &tag, Level &level);

// Beta: EntityIO.newEntity() - creates entity from string ID (EntityIO.java:42-55)
std::shared_ptr<Entity> newEntity(const jstring &id, Level &level);

// Alpha 1.2.6: EntityList.createEntity() - creates entity from int ID (EntityList.java:55-72)
// Used for Packet24MobSpawn
std::shared_ptr<Entity> createEntity(int_t id, Level &level);

}
