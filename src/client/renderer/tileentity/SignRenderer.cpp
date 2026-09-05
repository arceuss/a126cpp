#include "client/renderer/tileentity/SignRenderer.h"

#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/Tile.h"
#include "world/level/Level.h"
#include "client/gui/Font.h"
#include "client/renderer/culling/Culler.h"
#include "world/phys/AABB.h"
#include "OpenGL.h"

#include <algorithm>

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
SignRenderer::SignRenderer()
{
	// newb12: SignModel is initialized in member initializer
}

void SignRenderer::renderImpl(SignTileEntity &sign, double x, double y, double z, float a,
	bool omitPositionTranslate)
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
		font->drawLinesBatched(lines, xs, ys, 4, col);
	}
	else
	{
		for (int_t i = 0; i < 4; i++)
		{
			xs[i] = -font->width(sign.messages[i]) / 2;
			ys[i] = i * 10 - 4 * 5;
		}
		font->drawLinesBatched(sign.messages, xs, ys, 4, col);
	}
	
	glDepthMask(true);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPopMatrix();
}

// newb12: SignRenderer.render() (SignRenderer.java:12-68)
void SignRenderer::render(SignTileEntity &sign, double x, double y, double z, float a)
{
	renderImpl(sign, x, y, z, a, false);
}

void SignRenderer::renderEntity(TileEntity *entity, double x, double y, double z, float a)
{
	SignTileEntity *sign = dynamic_cast<SignTileEntity *>(entity);
	if (sign != nullptr)
		render(*sign, x, y, z, a);
}

std::size_t SignRenderer::RegionKeyHash::operator()(const RegionKey &key) const
{
	std::size_t hash = std::hash<int_t>{}(key.x);
	hash ^= std::hash<int_t>{}(key.y) + 0x9e3779b9U + (hash << 6) + (hash >> 2);
	hash ^= std::hash<int_t>{}(key.z) + 0x9e3779b9U + (hash << 6) + (hash >> 2);
	return hash;
}

SignRenderer::RegionKey SignRenderer::regionKeyFor(int_t x, int_t y, int_t z)
{
	RegionKey key;
	key.x = x >> REGION_SHIFT;
	key.y = y >> REGION_SHIFT;
	key.z = z >> REGION_SHIFT;
	return key;
}

void SignRenderer::deleteRegionList(CachedRegion &cached)
{
	if (cached.list != 0)
	{
		glDeleteLists(cached.list, 1);
		cached.list = 0;
	}
	cached.signs.clear();
}

// Bakes every queued sign of a region into one list. The transform chain per
// sign is renderImpl's, composed on the CPU (TileEntitySignRenderer.java:20-53):
//
//   T(pos + (0.5, 0.75*size, 0.5)) . Ry(-rot) . [T(0, -5/16, -7/16) for a wall
//   sign] . S(size, -size, -size) . board            (board, cube scale 1/16)
//   T(pos ...) . Ry(-rot) . [wall] . T(0, 0.5*size, 0.07*size) . S(f, -f, f)
//                                                       (text, f = size / 60)
//
// The outer S(size, -size, -size) is not baked: Alpha applies it with lighting
// on and no normalisation, which leaves the board's eye-space normals 1.5x
// long and its diffuse term that much brighter. The list pushes that scale
// around the boards itself, so they are built in the pre-scale frame (S^-1
// applied last) where the baked normals stay unit length and the same quirk
// falls out of the fixed-function pipeline as before.
GLuint SignRenderer::compileRegionList(const RegionKey &key, const std::vector<QueuedSign> &signs)
{
	GLuint list = glGenLists(1);
	if (list == 0)
		return 0;

	const float size = 0.6666667f;
	const float textScale = 0.016666668f * size;
	const long_t originX = static_cast<long_t>(key.x) * REGION_SIZE;
	const long_t originY = static_cast<long_t>(key.y) * REGION_SIZE;
	const long_t originZ = static_cast<long_t>(key.z) * REGION_SIZE;
	Font *font = getFont();
	Tesselator &t = Tesselator::instance;

	// Everything below this frame is the per-sign chain. The boards are drawn
	// under Alpha's outer S(size, -size, -size), pushed inside the list, so
	// their frame starts with S^-1; the text is drawn under no outer scale,
	// which keeps its baked normal unit length and exact for wall signs.
	auto signFrame = [&](const QueuedSign &sign, bool underOuterScale, ModelMatrix &frame, bool &post)
	{
		Tile *tile = (sign.tileId > 0 && sign.tileId < 256) ? Tile::tiles[sign.tileId] : nullptr;
		post = tile != nullptr && tile->id == Tile::sign.id;
		frame.loadIdentity();
		if (underOuterScale)
			frame.scale(1.0f / size, -1.0f / size, -1.0f / size);
		frame.translate(static_cast<float>(static_cast<long_t>(sign.x) - originX) + 0.5f,
			static_cast<float>(static_cast<long_t>(sign.y) - originY) + 0.75f * size,
			static_cast<float>(static_cast<long_t>(sign.z) - originZ) + 0.5f);
		if (post)
		{
			float rot = sign.data * 360 / 16.0f;
			frame.rotate(-rot, 0.0f, 1.0f, 0.0f);
		}
		else
		{
			float rot = 0.0f;
			if (sign.data == 2) rot = 180.0f;
			if (sign.data == 4) rot = 90.0f;
			if (sign.data == 5) rot = -90.0f;
			frame.rotate(-rot, 0.0f, 1.0f, 0.0f);
			frame.translate(0.0f, -0.3125f, -0.4375f);
		}
	};

	glNewList(list, GL_COMPILE);

	// Boards, under the outer scale Alpha applies with lighting on and no
	// normalisation; see the note above compileRegionList. Brightness is
	// Alpha's per-sign glColor3f before the renderer runs
	// (TileEntityRenderer.java:76-80); with GL_COLOR_MATERIAL it is the same
	// material whether it arrives as current colour or per-vertex colour.
	glPushMatrix();
	glScalef(size, -size, -size);
	bindTexture(u"/item/sign.png");
	t.begin();
	for (const QueuedSign &sign : signs)
	{
		ModelMatrix frame;
		bool post = false;
		signFrame(sign, true, frame, post);
		signModel.cube2.visible = post;
		ModelMatrix board = frame;
		board.scale(size, -size, -size);
		t.color(sign.brightness, sign.brightness, sign.brightness);
		signModel.emitTransformed(t, board);
	}
	t.end();
	glPopMatrix();

	// Text, depth-tested but not written, as Alpha draws it after each board.
	// Drawing every board before any text gives the same image: a later board
	// still overwrites earlier text where it is nearer, because the text never
	// wrote depth. Plain sign text is black and lighting cannot change it, but
	// colour-coded text is lit, so each sign's glyphs carry Alpha's text normal
	// (0, 0, -f) taken through the sign's own frame (TileEntitySignRenderer.java:53),
	// which under no outer scale is a unit vector.
	font->bindFontTexture();
	glDepthMask(false);
	t.begin();
	for (const QueuedSign &sign : signs)
	{
		ModelMatrix frame;
		bool post = false;
		signFrame(sign, false, frame, post);
		frame.translate(0.0f, 0.5f * size, 0.07f * size);
		frame.scale(textScale, -textScale, textScale);

		float nx = 0.0f, ny = 0.0f, nz = 0.0f;
		frame.transformNormal(0.0f, 0.0f, -1.0f * textScale, nx, ny, nz);
		t.normal(nx, ny, nz);

		int_t xs[4];
		int_t ys[4];
		for (int_t i = 0; i < 4; i++)
		{
			xs[i] = -font->width(sign.owner->messages[i]) / 2;
			ys[i] = i * 10 - 4 * 5;
		}
		font->appendLines(t, sign.owner->messages, xs, ys, 4, 0, false, &frame);
	}
	t.end();
	glDepthMask(true);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEndList();
	return list;
}

bool SignRenderer::queueWorld(SignTileEntity &sign, float brightness)
{
	// TextEditScreen's direct preview uses selectedLine. Normal world signs are
	// immutable between explicit packet/edit notifications or block replacement,
	// so no per-frame string comparisons are needed here.
	if (!batchWorldSigns || sign.selectedLine >= 0)
		return false;
	if (sign.level == nullptr || !sign.level->hasChunkAt(sign.x, sign.y, sign.z))
		return false;

	QueuedSign queued;
	queued.owner = &sign;
	queued.x = sign.x;
	queued.y = sign.y;
	queued.z = sign.z;
	queued.brightness = brightness;
	queued.tileId = sign.level->getTile(sign.x, sign.y, sign.z);
	queued.data = sign.getData();
	pendingRegions[regionKeyFor(sign.x, sign.y, sign.z)].push_back(queued);
	return true;
}

void SignRenderer::flushWorldBatch(Culler &culler)
{
	if (pendingRegions.empty())
		return;

	for (auto &pending : pendingRegions)
	{
		const RegionKey &key = pending.first;
		// The frustum test is per region rather than per sign, so a region's
		// list depends only on which signs are in range, not on where the
		// camera points; a sign outside the frustum but inside a visible
		// region is clipped by the GPU.
		const double x0 = static_cast<double>(key.x * REGION_SIZE);
		const double y0 = static_cast<double>(key.y * REGION_SIZE);
		const double z0 = static_cast<double>(key.z * REGION_SIZE);
		AABB regionBox(x0, y0, z0, x0 + REGION_SIZE, y0 + REGION_SIZE, z0 + REGION_SIZE);
		if (!culler.isVisible(regionBox))
			continue;

		std::vector<QueuedSign> &signs = pending.second;
		// LevelRenderer visits tile entities in list order, which can change
		// between frames; the batch's draw order is by position so the same
		// set of signs always compiles to the same list.
		std::sort(signs.begin(), signs.end(), [](const QueuedSign &a, const QueuedSign &b)
		{
			if (a.y != b.y) return a.y < b.y;
			if (a.z != b.z) return a.z < b.z;
			return a.x < b.x;
		});

		CachedRegion &cached = regionCache[key];
		if (cached.list == 0 || cached.signs != signs)
		{
			deleteRegionList(cached);
			cached.list = compileRegionList(key, signs);
			cached.signs = signs;
		}
		if (cached.list == 0)
			continue;

		// Alpha's dispatcher subtracts the interpolated camera position from
		// the tile-entity position in double precision and only then narrows
		// to float (TileEntityRenderDispatcher.java:73); the region origin
		// takes that place so the modelview never holds a large coordinate.
		glPushMatrix();
		glTranslatef(
			static_cast<float>(static_cast<double>(key.x * REGION_SIZE) - TileEntityRenderDispatcher::xOff),
			static_cast<float>(static_cast<double>(key.y * REGION_SIZE) - TileEntityRenderDispatcher::yOff),
			static_cast<float>(static_cast<double>(key.z * REGION_SIZE) - TileEntityRenderDispatcher::zOff));
		glCallList(cached.list);
		glPopMatrix();
	}
	pendingRegions.clear();
}

void SignRenderer::invalidateWorldSign(SignTileEntity *sign)
{
	if (sign == nullptr)
		return;
	invalidateWorldSignAt(sign->x, sign->y, sign->z);
}

void SignRenderer::invalidateWorldSignAt(int_t x, int_t y, int_t z)
{
	auto it = regionCache.find(regionKeyFor(x, y, z));
	if (it == regionCache.end())
		return;
	deleteRegionList(it->second);
	regionCache.erase(it);
}

void SignRenderer::clearWorldCache()
{
	pendingRegions.clear();
	for (auto &entry : regionCache)
		deleteRegionList(entry.second);
	regionCache.clear();
}
