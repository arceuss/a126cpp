#include "client/renderer/tileentity/SignRenderer.h"

#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "client/gui/Font.h"
#include "OpenGL.h"

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
SignRenderer::SignRenderer()
{
	// newb12: SignModel is initialized in member initializer
	worldBatch.reserve(4096);
}

void SignRenderer::renderImpl(SignTileEntity &sign, double x, double y, double z, float a,
	bool immediateGeometry, bool omitPositionTranslate)
{
	// Alpha: Block block = sign.getBlockType() (TileEntitySignRenderer.java:19).
	// Alpha re-reads the level every frame (TileEntity.java:71-87) and a missing
	// block yields null, which falls through to the wall-sign branch instead of
	// skipping the sign. Only an unloaded chunk is refused here, because the
	// port would otherwise read tile data that does not exist.
	if (sign.level == nullptr || !sign.level->hasChunkAt(sign.x, sign.y, sign.z))
		return;

	int_t tileId = sign.level->getTile(sign.x, sign.y, sign.z);
	Tile *tile = (tileId > 0 && tileId < 256) ? Tile::tiles[tileId] : nullptr;

	glPushMatrix();  // Alpha: GL11.glPushMatrix() (TileEntitySignRenderer.java:20)
	float size = 0.6666667f;  // Alpha: float f3 = 2.0F / 3.0F (TileEntitySignRenderer.java:21)

	if (tile != nullptr && tile->id == Tile::sign.id)  // Alpha: if (block == Block.signPost) (TileEntitySignRenderer.java:22)
	{
		// Omitted while compiling a cached list, because flushWorldBatch owns
		// this translate and must compute it against the live camera.
		if (!omitPositionTranslate)
			glTranslatef((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);  // Alpha lines 23-25
		float rot = sign.getData() * 360 / 16.0f;
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);
		signModel.cube2.visible = true;
	}
	else
	{
		int_t face = sign.getData();
		float rot = 0.0f;
		if (face == 2) rot = 180.0f;
		if (face == 4) rot = 90.0f;
		if (face == 5) rot = -90.0f;

		if (!omitPositionTranslate)
			glTranslatef((float)x + 0.5f, (float)y + 0.75f * size, (float)z + 0.5f);
		glRotatef(-rot, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, -0.3125f, -0.4375f);
		signModel.cube2.visible = false;
	}
	
	bindTexture(u"/item/sign.png");  // Alpha: TileEntitySignRenderer.java:44
	glPushMatrix();
	glScalef(size, -size, -size);
	if (immediateGeometry)
		signModel.renderImmediate();
	else
		signModel.render();
	glPopMatrix();
	
	Font *font = getFont();
	float s = 0.016666668f * size;
	glTranslatef(0.0f, 0.5f * size, 0.07f * size);
	glScalef(s, -s, s);
	glNormal3f(0.0f, 0.0f, -1.0f * s);
	glDepthMask(false);
	int_t col = 0;
	
	int_t xs[4];
	int_t ys[4];

	if (sign.selectedLine >= 0 && sign.selectedLine < 4)
	{
		// Alpha's edit marker is dynamic, so this branch is never cached by the
		// world renderer.  It remains here for TextEditScreen's direct render.
		jstring lines[4];
		for (int_t i = 0; i < 4; i++)
		{
			lines[i] = (i == sign.selectedLine)
				? u"> " + sign.messages[i] + u" <"
				: sign.messages[i];
			xs[i] = -font->width(lines[i]) / 2;
			ys[i] = i * 10 - 4 * 5;
		}
		if (immediateGeometry)
			font->drawLinesImmediate(lines, xs, ys, 4, col);
		else
			font->drawLinesBatched(lines, xs, ys, 4, col);
	}
	else
	{
		for (int_t i = 0; i < 4; i++)
		{
			xs[i] = -font->width(sign.messages[i]) / 2;
			ys[i] = i * 10 - 4 * 5;
		}
		if (immediateGeometry)
			font->drawLinesImmediate(sign.messages, xs, ys, 4, col);
		else
			font->drawLinesBatched(sign.messages, xs, ys, 4, col);
	}
	
	glDepthMask(true);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPopMatrix();
}

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
void SignRenderer::render(SignTileEntity &sign, double x, double y, double z, float a)
{
	renderImpl(sign, x, y, z, a, false, false);
}

void SignRenderer::renderEntity(TileEntity *entity, double x, double y, double z, float a)
{
	SignTileEntity *sign = dynamic_cast<SignTileEntity *>(entity);
	if (sign != nullptr)
		render(*sign, x, y, z, a);
}

std::size_t SignRenderer::WorldSignKeyHash::operator()(const WorldSignKey &key) const
{
	std::size_t hash = std::hash<int_t>{}(key.x);
	hash ^= std::hash<int_t>{}(key.y) + 0x9e3779b9U + (hash << 6) + (hash >> 2);
	hash ^= std::hash<int_t>{}(key.z) + 0x9e3779b9U + (hash << 6) + (hash >> 2);
	return hash;
}

SignRenderer::WorldSignKey SignRenderer::makeWorldSignKey(int_t x, int_t y, int_t z)
{
	WorldSignKey key;
	key.x = x;
	key.y = y;
	key.z = z;
	return key;
}

void SignRenderer::deleteCachedList(CachedWorldSign &cached)
{
	if (cached.list != 0)
	{
		glDeleteLists(cached.list, 1);
		cached.list = 0;
	}
}

GLuint SignRenderer::compileWorldList(SignTileEntity &sign, float brightness, int_t tileId, int_t data)
{
	GLuint list = glGenLists(1);
	if (list == 0)
		return 0;

	glNewList(list, GL_COMPILE);
	// Alpha TileEntityRenderer.renderTileEntity sets this immediately before
	// invoking the sign renderer (TileEntityRenderer.java:76-80). Capturing the
	// value is pixel-identical until the discrete world light value changes; the
	// queue detects that change and recompiles only the affected sign.
	glColor3f(brightness, brightness, brightness);
	// The sign position is deliberately absent: flushWorldBatch supplies it
	// per frame, so this list stays valid at any world coordinate.
	renderImpl(sign, 0.0, 0.0, 0.0, 0.0f, true, true);
	glEndList();

	return list;
}

bool SignRenderer::queueWorld(SignTileEntity &sign, float brightness)
{
	// TextEditScreen's direct preview uses selectedLine. Normal world signs are
	// immutable between explicit packet/edit notifications or block replacement,
	// so no per-frame string comparisons are needed here.
	if (sign.selectedLine >= 0)
		return false;
	if (sign.level == nullptr || !sign.level->hasChunkAt(sign.x, sign.y, sign.z))
		return false;

	int_t tileId = sign.level->getTile(sign.x, sign.y, sign.z);
	int_t data = sign.getData();
	WorldSignKey key = makeWorldSignKey(sign.x, sign.y, sign.z);
	CachedWorldSign &cached = worldCache[key];
	if (cached.list == 0 || cached.owner != &sign || cached.brightness != brightness || cached.tileId != tileId || cached.data != data)
	{
		deleteCachedList(cached);
		cached.owner = &sign;
		cached.list = compileWorldList(sign, brightness, tileId, data);
		cached.brightness = brightness;
		cached.tileId = tileId;
		cached.data = data;
	}

	if (cached.list == 0)
		return false;

	worldBatch.push_back(QueuedWorldSign{ cached.list, sign.x, sign.y, sign.z });
	return true;
}

void SignRenderer::flushWorldBatch()
{
	if (worldBatch.empty())
		return;

	// Alpha's dispatcher subtracts the interpolated camera position from the
	// tile-entity position in double precision and only then narrows to float
	// (TileEntityRenderDispatcher.java:73, TileEntitySignRenderer.java:23-25).
	// Reproducing that expression here keeps the value handed to GL small, so
	// the modelview matrix never holds a large world coordinate whose float
	// rounding would quantise a distant sign's position.
	const float size = 0.6666667f;
	for (const QueuedWorldSign &queued : worldBatch)
	{
		glPushMatrix();
		glTranslatef(
			static_cast<float>(static_cast<double>(queued.x) - TileEntityRenderDispatcher::xOff) + 0.5f,
			static_cast<float>(static_cast<double>(queued.y) - TileEntityRenderDispatcher::yOff) + 0.75f * size,
			static_cast<float>(static_cast<double>(queued.z) - TileEntityRenderDispatcher::zOff) + 0.5f);
		glCallList(queued.list);
		glPopMatrix();
	}
	worldBatch.clear();
}

void SignRenderer::invalidateWorldSign(SignTileEntity *sign)
{
	if (sign == nullptr)
		return;

	WorldSignKey key = makeWorldSignKey(sign->x, sign->y, sign->z);
	auto it = worldCache.find(key);
	if (it == worldCache.end() || it->second.owner != sign)
		return;

	deleteCachedList(it->second);
	worldCache.erase(it);
}

void SignRenderer::invalidateWorldSignAt(int_t x, int_t y, int_t z)
{
	WorldSignKey key = makeWorldSignKey(x, y, z);
	auto it = worldCache.find(key);
	if (it == worldCache.end())
		return;

	deleteCachedList(it->second);
	worldCache.erase(it);
}

void SignRenderer::clearWorldCache()
{
	worldBatch.clear();
	for (auto &entry : worldCache)
		deleteCachedList(entry.second);
	worldCache.clear();
}
