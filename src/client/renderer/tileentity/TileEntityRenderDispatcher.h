#pragma once

#include <unordered_map>
#include <memory>
#include <typeindex>

#include "java/Type.h"

class TileEntity;
class TileEntityRenderer;
class Textures;
class Font;
class Level;
class Player;

// Beta 1.2: TileEntityRenderDispatcher.java
class TileEntityRenderDispatcher
{
private:
	std::unordered_map<std::type_index, TileEntityRenderer *> renderers;

public:
	static TileEntityRenderDispatcher instance;
	
	static double xOff;
	static double yOff;
	static double zOff;
	
	Textures *textures = nullptr;
	Level *level = nullptr;
	Player *player = nullptr;
	
	float playerRotY = 0.0f;
	float playerRotX = 0.0f;
	
	double xPlayer = 0.0;
	double yPlayer = 0.0;
	double zPlayer = 0.0;

private:
	Font *font = nullptr;

public:
	TileEntityRenderDispatcher();
	
	// Beta: TileEntityRenderDispatcher.getRenderer() - gets renderer by class (TileEntityRenderDispatcher.java:39-47)
	template<typename T>
	TileEntityRenderer *getRenderer()
	{
		std::type_index type = std::type_index(typeid(T));
		auto it = renderers.find(type);
		if (it != renderers.end())
			return it->second;
		
		// Beta: Try superclass (TileEntityRenderDispatcher.java:41-44)
		// In C++, we can't easily get superclass, so just return nullptr
		return nullptr;
	}
	
	// Beta: TileEntityRenderDispatcher.hasRenderer() (TileEntityRenderDispatcher.java:49-51)
	bool hasRenderer(TileEntity *e);
	
	// Beta: TileEntityRenderDispatcher.getRenderer() - gets renderer by entity (TileEntityRenderDispatcher.java:53-55)
	TileEntityRenderer *getRenderer(TileEntity *e);
	
	// Beta: TileEntityRenderDispatcher.prepare() (TileEntityRenderDispatcher.java:57-67)
	void prepare(Level *level, Textures *textures, Font *font, Player *player, float a);
	
	// Beta: TileEntityRenderDispatcher.render() - renders with distance check (TileEntityRenderDispatcher.java:69-75)
	void render(TileEntity *e, float a);
	
	// Beta: TileEntityRenderDispatcher.render() - renders at position (TileEntityRenderDispatcher.java:77-82)
	void render(TileEntity *entity, double x, double y, double z, float a);
	
	// Beta: TileEntityRenderDispatcher.setLevel() (TileEntityRenderDispatcher.java:84-86)
	void setLevel(Level *level);
	
	// Beta: TileEntityRenderDispatcher.distanceToSqr() (TileEntityRenderDispatcher.java:88-93)
	double distanceToSqr(double x, double y, double z);
	
	// Beta: TileEntityRenderDispatcher.getFont() (TileEntityRenderDispatcher.java:95-97)
	Font *getFont();
	
	// Register renderer for a specific tile entity type
	template<typename T>
	void registerRenderer(TileEntityRenderer *renderer)
	{
		std::type_index type = std::type_index(typeid(T));
		renderers[type] = renderer;
	}
};
