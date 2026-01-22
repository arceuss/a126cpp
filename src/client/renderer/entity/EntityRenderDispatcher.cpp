#include "client/renderer/entity/EntityRenderDispatcher.h"

#include "client/renderer/entity/EntityRenderer.h"
#include "client/renderer/entity/PlayerRenderer.h"
#include "client/renderer/entity/ItemRenderer.h"
#include "client/renderer/entity/PigRenderer.h"
#include "client/renderer/entity/CowRenderer.h"
#include "client/renderer/entity/SheepRenderer.h"
#include "client/renderer/entity/ChickenRenderer.h"
#include "client/renderer/entity/FallingTileRenderer.h"
#include "client/renderer/entity/BoatRenderer.h"
#include "client/renderer/entity/MinecartRenderer.h"
#include "client/renderer/entity/PaintingRenderer.h"
#include "client/renderer/entity/TntRenderer.h"
#include "client/renderer/entity/ArrowRenderer.h"
#include "client/renderer/entity/CreeperRenderer.h"
#include "client/renderer/entity/HumanoidMobRenderer.h"
#include "client/renderer/entity/GiantMobRenderer.h"
#include "client/renderer/entity/SlimeRenderer.h"
#include "client/renderer/entity/GhastRenderer.h"
#include "client/renderer/entity/SpiderRenderer.h"
#include "client/model/PigModel.h"
#include "client/model/CowModel.h"
#include "client/model/SheepModel.h"
#include "client/model/SheepFurModel.h"
#include "client/model/ChickenModel.h"
#include "client/model/HumanoidModel.h"
#include "client/model/ZombieModel.h"
#include "client/model/SkeletonModel.h"
#include "client/model/SlimeModel.h"
#include "world/entity/Entity.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/item/FallingTile.h"
#include "world/entity/item/Boat.h"
#include "world/entity/item/Minecart.h"
#include "world/entity/Painting.h"
#include "world/entity/PrimedTnt.h"
#include "world/entity/projectile/Arrow.h"
#include "world/entity/projectile/FishingHook.h"
#include "client/renderer/entity/FishingHookRenderer.h"
#include "world/entity/player/Player.h"
#include "world/entity/animal/Pig.h"
#include "world/entity/animal/Cow.h"
#include "world/entity/animal/Sheep.h"
#include "world/entity/animal/Chicken.h"
#include "world/entity/monster/Creeper.h"
#include "world/entity/monster/Zombie.h"
#include "world/entity/monster/Skeleton.h"
#include "world/entity/monster/Giant.h"
#include "world/entity/monster/Slime.h"
#include "world/entity/monster/Ghast.h"
#include "world/entity/monster/Spider.h"
#include "world/entity/monster/PigZombie.h"
#include "world/entity/monster/Monster.h"
#include "client/renderer/entity/MobRenderer.h"
#include "util/Memory.h"

#include "OpenGL.h"
#include <iostream>
#include <typeinfo>
#include <typeindex>

EntityRenderDispatcher EntityRenderDispatcher::instance;

double EntityRenderDispatcher::xOff = 0.0;
double EntityRenderDispatcher::yOff = 0.0;
double EntityRenderDispatcher::zOff = 0.0;

PlayerRenderer EntityRenderDispatcher::playerRenderer = PlayerRenderer(EntityRenderDispatcher::instance);
ItemRenderer EntityRenderDispatcher::itemRenderer = ItemRenderer(EntityRenderDispatcher::instance);

// newb12: EntityRenderDispatcher() - constructor that registers all renderers
// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:68-100
EntityRenderDispatcher::EntityRenderDispatcher()
{
	// Register ItemRenderer for EntityItem
	registerRenderer(typeid(EntityItem), &itemRenderer);
	
	// Register PlayerRenderer for Player
	registerRenderer(typeid(Player), &playerRenderer);
	
	// Register animal renderers
	// newb12: PigRenderer(new PigModel(), new PigModel(0.5F), 0.7F)
	static PigRenderer pigRenderer(*this, Util::make_shared<PigModel>(), Util::make_shared<PigModel>(0.5f), 0.7f);
	registerRenderer(typeid(Pig), &pigRenderer);
	
	// newb12: SheepRenderer(new SheepModel(), new SheepFurModel(), 0.7F)
	static SheepRenderer sheepRenderer(*this, Util::make_shared<SheepModel>(), Util::make_shared<SheepFurModel>(), 0.7f);
	registerRenderer(typeid(Sheep), &sheepRenderer);
	
	// newb12: CowRenderer(new CowModel(), 0.7F)
	static CowRenderer cowRenderer(*this, Util::make_shared<CowModel>(), 0.7f);
	registerRenderer(typeid(Cow), &cowRenderer);
	
	// newb12: ChickenRenderer(new ChickenModel(), 0.3F)
	static ChickenRenderer chickenRenderer(*this, Util::make_shared<ChickenModel>(), 0.3f);
	registerRenderer(typeid(Chicken), &chickenRenderer);
	
	// Register item/other entity renderers
	static FallingTileRenderer fallingTileRenderer(*this);
	registerRenderer(typeid(FallingTile), &fallingTileRenderer);
	
	static BoatRenderer boatRenderer(*this);
	registerRenderer(typeid(Boat), &boatRenderer);
	
	static MinecartRenderer minecartRenderer(*this);
	registerRenderer(typeid(Minecart), &minecartRenderer);
	
	static PaintingRenderer paintingRenderer(*this);
	registerRenderer(typeid(Painting), &paintingRenderer);
	
	static TntRenderer tntRenderer(*this);
	registerRenderer(typeid(PrimedTnt), &tntRenderer);
	
	static ArrowRenderer arrowRenderer(*this);
	registerRenderer(typeid(Arrow), &arrowRenderer);
	
	static FishingHookRenderer fishingHookRenderer(*this);
	registerRenderer(typeid(FishingHook), &fishingHookRenderer);
	
	// Register monster renderers
	static CreeperRenderer creeperRenderer(*this);
	registerRenderer(typeid(Creeper), &creeperRenderer);
	
	static HumanoidMobRenderer zombieRenderer(*this, Util::make_shared<ZombieModel>(), 0.5f);
	registerRenderer(typeid(Zombie), &zombieRenderer);
	
	static HumanoidMobRenderer skeletonRenderer(*this, Util::make_shared<SkeletonModel>(), 0.5f);
	registerRenderer(typeid(Skeleton), &skeletonRenderer);
	
	static GiantMobRenderer giantRenderer(*this, Util::make_shared<HumanoidModel>(), 0.5f, 6.0f);
	registerRenderer(typeid(Giant), &giantRenderer);
	
	static SlimeRenderer slimeRenderer(*this, Util::make_shared<SlimeModel>(0), Util::make_shared<SlimeModel>(16), 0.25f);
	registerRenderer(typeid(Slime), &slimeRenderer);
	
	static GhastRenderer ghastRenderer(*this);
	registerRenderer(typeid(Ghast), &ghastRenderer);
	
	static SpiderRenderer spiderRenderer(*this);
	registerRenderer(typeid(Spider), &spiderRenderer);
	
	// PigZombie extends Zombie, so it uses the same HumanoidMobRenderer
	static HumanoidMobRenderer pigZombieRenderer(*this, Util::make_shared<ZombieModel>(), 0.5f);
	registerRenderer(typeid(PigZombie), &pigZombieRenderer);
	
	// Register MobRenderer for Monster (base class for hostile mobs without specific renderers)
	// Monster uses HumanoidModel like zombies/skeletons
	static MobRenderer monsterRenderer(*this, Util::make_shared<HumanoidModel>(), 0.5f);
	registerRenderer(typeid(Monster), &monsterRenderer);
	
	// TODO: Register MobRenderer for Mob (fallback for unregistered mobs)
	// TODO: Register DefaultRenderer for Entity (fallback for all entities)
}

void EntityRenderDispatcher::prepare(std::shared_ptr<Level> level, Textures &textures, Font &font, std::shared_ptr<Player> player, Options &options, float a)
{
	this->level = level;
	this->textures = &textures;
	this->options = &options;
	this->player = player;
	this->font = &font;

	playerRotY = player->yRotO + (player->yRot - player->yRotO) * a;
	playerRotX = player->xRotO + (player->xRot - player->xRotO) * a;
	xPlayer = player->xOld + (player->x - player->xOld) * a;
	yPlayer = player->yOld + (player->y - player->yOld) * a;
	zPlayer = player->zOld + (player->z - player->zOld) * a;
}

void EntityRenderDispatcher::render(Entity &entity, float a)
{
	double x = entity.xOld + (entity.x - entity.xOld) * a;
	double y = entity.yOld + (entity.y - entity.yOld) * a;
	double z = entity.zOld + (entity.z - entity.zOld) * a;
	float rot = entity.yRotO + (entity.yRot - entity.yRotO) * a;
	
	float brightness = entity.getBrightness(a);
	glColor3f(brightness, brightness, brightness);
	render(entity, x - xOff, y - yOff, z - zOff, rot, a);
}

// newb12: EntityRenderDispatcher.render(Entity, double, double, double, float, float)
// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:139-145
void EntityRenderDispatcher::render(Entity &entity, double x, double y, double z, float rot, float a)
{
	EntityRenderer *renderer = getRenderer(entity);
	if (renderer == nullptr)
	{
		// Fallback: use playerRenderer for unregistered entities (temporary until all renderers are registered)
		// This maintains backward compatibility
		renderer = &playerRenderer;
	}
	renderer->render(entity, x, y, z, rot, a);  // Beta: var10.render(...) (EntityRenderDispatcher.java:142)
	renderer->postRender(entity, x, y, z, rot, a);  // Beta: var10.postRender(...) (EntityRenderDispatcher.java:143)
}

void EntityRenderDispatcher::setLevel(std::shared_ptr<Level> level)
{
	this->level = level;
}

double EntityRenderDispatcher::distanceToSqr(double x, double y, double z)
{
	double dx = x - xPlayer;
	double dy = y - yPlayer;
	double dz = z - zPlayer;
	return dx * dx + dy * dy + dz * dz;
}

// newb12: EntityRenderDispatcher.getRenderer(Class) - recursively looks up renderer by class hierarchy
// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:102-110
// Note: C++ doesn't have easy type hierarchy traversal like Java, so we check common base types
EntityRenderer* EntityRenderDispatcher::getRenderer(const std::type_index &type)
{
	auto it = renderers.find(type);
	if (it != renderers.end())
	{
		return it->second;
	}
	
	// If not found, check if it's Entity base class (stop recursion)
	if (type == typeid(Entity))
	{
		return nullptr;  // No default renderer registered yet
	}
	
	// In C++, we can't easily traverse the type hierarchy like Java's getSuperclass()
	// Instead, we'll rely on explicit registration of renderers for each type
	// Fallback renderers for base classes (Mob, Animal, etc.) will be registered separately
	return nullptr;
}

// newb12: EntityRenderDispatcher.getRenderer(Entity) - gets renderer for entity instance
// Reference: newb12/net/minecraft/client/renderer/entity/EntityRenderDispatcher.java:112-114
EntityRenderer* EntityRenderDispatcher::getRenderer(Entity &entity)
{
	return getRenderer(std::type_index(typeid(entity)));
}

// Register a renderer for an entity type
void EntityRenderDispatcher::registerRenderer(const std::type_index &type, EntityRenderer *renderer)
{
	renderers[type] = renderer;
}

Font &EntityRenderDispatcher::getFont()
{
	return *font;
}
