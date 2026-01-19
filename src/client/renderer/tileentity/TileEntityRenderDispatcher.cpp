#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"

#include "client/renderer/tileentity/TileEntityRenderer.h"
#include "client/renderer/tileentity/SignRenderer.h"
#include "client/renderer/Textures.h"
#include "client/gui/Font.h"
#include "world/level/Level.h"
#include "world/entity/player/Player.h"
#include "world/level/tile/entity/TileEntity.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/level/tile/entity/MobSpawnerTileEntity.h"
#include "OpenGL.h"

TileEntityRenderDispatcher TileEntityRenderDispatcher::instance;

double TileEntityRenderDispatcher::xOff = 0.0;
double TileEntityRenderDispatcher::yOff = 0.0;
double TileEntityRenderDispatcher::zOff = 0.0;

// Beta: TileEntityRenderDispatcher() (TileEntityRenderDispatcher.java:30-37)
TileEntityRenderDispatcher::TileEntityRenderDispatcher()
{
	// Beta: Register SignRenderer (TileEntityRenderDispatcher.java:31)
	static SignRenderer signRenderer;
	signRenderer.init(this);
	registerRenderer<SignTileEntity>(&signRenderer);
	
	// Beta: Register MobSpawnerRenderer (TileEntityRenderDispatcher.java:32)
	// TODO: Add MobSpawnerRenderer when implemented
}

// Beta: TileEntityRenderDispatcher.hasRenderer() (TileEntityRenderDispatcher.java:49-51)
bool TileEntityRenderDispatcher::hasRenderer(TileEntity *e)
{
	return getRenderer(e) != nullptr;
}

// Beta: TileEntityRenderDispatcher.getRenderer() - gets renderer by entity (TileEntityRenderDispatcher.java:53-55)
TileEntityRenderer *TileEntityRenderDispatcher::getRenderer(TileEntity *e)
{
	if (e == nullptr)
		return nullptr;
	
	// Beta: Get renderer by type (TileEntityRenderDispatcher.java:54)
	// Use typeid to get the type
	std::type_index type = std::type_index(typeid(*e));
	auto it = renderers.find(type);
	if (it != renderers.end())
		return it->second;
	
	// Beta: Try base class (TileEntityRenderDispatcher.java:41-44)
	// In C++, we can't easily traverse inheritance, so just return nullptr
	return nullptr;
}

// Beta: TileEntityRenderDispatcher.prepare() (TileEntityRenderDispatcher.java:57-67)
void TileEntityRenderDispatcher::prepare(Level *level, Textures *textures, Font *font, Player *player, float a)
{
	this->level = level;  // Beta: this.level = level (TileEntityRenderDispatcher.java:58)
	this->textures = textures;  // Beta: this.textures = textures (TileEntityRenderDispatcher.java:59)
	this->player = player;  // Beta: this.player = player (TileEntityRenderDispatcher.java:60)
	this->font = font;  // Beta: this.font = font (TileEntityRenderDispatcher.java:61)
	
	// Beta: Interpolate player rotation and position (TileEntityRenderDispatcher.java:62-66)
	playerRotY = player->yRotO + (player->yRot - player->yRotO) * a;  // Beta: this.playerRotY = player.yRotO + (player.yRot - player.yRotO) * a (TileEntityRenderDispatcher.java:62)
	playerRotX = player->xRotO + (player->xRot - player->xRotO) * a;  // Beta: this.playerRotX = player.xRotO + (player.xRot - player.xRotO) * a (TileEntityRenderDispatcher.java:63)
	xPlayer = player->xOld + (player->x - player->xOld) * a;  // Beta: this.xPlayer = player.xOld + (player.x - player.xOld) * a (TileEntityRenderDispatcher.java:64)
	yPlayer = player->yOld + (player->y - player->yOld) * a;  // Beta: this.yPlayer = player.yOld + (player.y - player.yOld) * a (TileEntityRenderDispatcher.java:65)
	zPlayer = player->zOld + (player->z - player->zOld) * a;  // Beta: this.zPlayer = player.zOld + (player.z - player.zOld) * a (TileEntityRenderDispatcher.java:66)
}

// Beta: TileEntityRenderDispatcher.render() - renders with distance check (TileEntityRenderDispatcher.java:69-75)
// Performance optimization: More aggressive distance culling for better performance
void TileEntityRenderDispatcher::render(TileEntity *e, float a)
{
	// Beta: Check distance (TileEntityRenderDispatcher.java:70)
	// Original: 4096.0 (64 blocks) - reduced to 2304.0 (48 blocks) for better performance
	// This culls distant signs while still rendering nearby ones
	if (e->distanceToSqr(xPlayer, yPlayer, zPlayer) < 2304.0)
	{
		// Beta: Get brightness and set color (TileEntityRenderDispatcher.java:71-72)
		float br = level->getBrightness(e->x, e->y, e->z);
		glColor3f(br, br, br);
		
		// Beta: Render at interpolated position (TileEntityRenderDispatcher.java:73)
		render(e, e->x - xOff, e->y - yOff, e->z - zOff, a);
	}
}

// Beta: TileEntityRenderDispatcher.render() - renders at position (TileEntityRenderDispatcher.java:77-82)
// Performance optimization: Direct type check instead of expensive typeid/dynamic_cast
void TileEntityRenderDispatcher::render(TileEntity *entity, double x, double y, double z, float a)
{
	// Performance: Direct type check for signs (most common tile entity)
	// Avoids expensive typeid/dynamic_cast lookup every frame
	SignTileEntity *sign = dynamic_cast<SignTileEntity *>(entity);
	if (sign != nullptr)
	{
		// Performance: Cached renderer lookup - avoids expensive typeid/map lookup every frame
		// SignRenderer is registered in constructor, so we can cache the lookup result
		static SignRenderer *cachedSignRenderer = nullptr;
		static bool rendererLookupAttempted = false;
		
		if (!rendererLookupAttempted)
		{
			// First time: look up renderer using template method (cached forever)
			// We know it exists because constructor registers it
			TileEntityRenderer *renderer = getRenderer<SignTileEntity>();
			if (renderer != nullptr)
			{
				cachedSignRenderer = static_cast<SignRenderer *>(renderer);  // Safe cast - we register SignRenderer
			}
			rendererLookupAttempted = true;
		}
		
		if (cachedSignRenderer != nullptr)
		{
			cachedSignRenderer->render(*sign, x, y, z, a);
		}
		return;
	}
	
	// Fallback to generic renderer lookup for other tile entity types
	TileEntityRenderer *renderer = getRenderer(entity);
	if (renderer != nullptr)
	{
		// For other types, use dynamic_cast as fallback
		SignRenderer *signRenderer = dynamic_cast<SignRenderer *>(renderer);
		if (signRenderer != nullptr)
		{
			signRenderer->renderEntity(entity, x, y, z, a);
		}
		// TODO: Add other renderer types (MobSpawnerRenderer, etc.)
	}
}

// Beta: TileEntityRenderDispatcher.setLevel() (TileEntityRenderDispatcher.java:84-86)
void TileEntityRenderDispatcher::setLevel(Level *level)
{
	this->level = level;  // Beta: this.level = level (TileEntityRenderDispatcher.java:85)
}

// Beta: TileEntityRenderDispatcher.distanceToSqr() (TileEntityRenderDispatcher.java:88-93)
double TileEntityRenderDispatcher::distanceToSqr(double x, double y, double z)
{
	double xd = x - xPlayer;  // Beta: double xd = x - this.xPlayer (TileEntityRenderDispatcher.java:89)
	double yd = y - yPlayer;  // Beta: double yd = y - this.yPlayer (TileEntityRenderDispatcher.java:90)
	double zd = z - zPlayer;  // Beta: double zd = z - this.zPlayer (TileEntityRenderDispatcher.java:91)
	return xd * xd + yd * yd + zd * zd;  // Beta: return xd * xd + yd * yd + zd * zd (TileEntityRenderDispatcher.java:92)
}

// Beta: TileEntityRenderDispatcher.getFont() (TileEntityRenderDispatcher.java:95-97)
Font *TileEntityRenderDispatcher::getFont()
{
	return font;  // Beta: return this.font (TileEntityRenderDispatcher.java:96)
}
