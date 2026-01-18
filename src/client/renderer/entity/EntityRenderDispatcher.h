#pragma once

#include "world/level/Level.h"

#include "client/Options.h"
#include "client/renderer/Textures.h"
#include "client/gui/Font.h"

#include <unordered_map>
#include <typeindex>
#include <memory>

class EntityRenderer;
class PlayerRenderer;
class ItemRenderer;

// newb12: EntityRenderDispatcher - routes entities to their renderers
// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java
class EntityRenderDispatcher
{
private:
	// newb12: Map<Class, EntityRenderer> renderers - registry of entity renderers
	// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:45
	std::unordered_map<std::type_index, EntityRenderer*> renderers;

	Font *font = nullptr;

public:
	static EntityRenderDispatcher instance;

	static PlayerRenderer playerRenderer;
	static ItemRenderer itemRenderer;

public:
	static double xOff;
	static double yOff;
	static double zOff;

	Textures *textures = nullptr;

	std::shared_ptr<Level> level = nullptr;

	std::shared_ptr<Player> player = nullptr;
	float playerRotY = 0.0f, playerRotX = 0.0f;

	Options *options = nullptr;

	double xPlayer = 0.0, yPlayer = 0.0, zPlayer = 0.0;

private:
	// newb12: EntityRenderDispatcher() - constructor that registers all renderers
	// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:68-100
	EntityRenderDispatcher();

public:
	void prepare(std::shared_ptr<Level> level, Textures &textures, Font &font, std::shared_ptr<Player> player, Options &options, float a);
	void render(Entity &entity, float a);
	void render(Entity &entity, double x, double y, double z, float rot, float a);

	// newb12: getRenderer(Class) - gets renderer for entity class, recursively checks superclass
	// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:102-110
	EntityRenderer* getRenderer(const std::type_index &type);

	// newb12: getRenderer(Entity) - gets renderer for entity instance
	// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:112-114
	EntityRenderer* getRenderer(Entity &entity);

	void setLevel(std::shared_ptr<Level> level);

	double distanceToSqr(double x, double y, double z);

	Font &getFont();

	// Register a renderer for an entity type
	void registerRenderer(const std::type_index &type, EntityRenderer *renderer);
};
