// Sign-wall fixture: text metrics against real Java output, and a dense wall of
// signs driven through the production world, storage and renderer paths.
//
// The wall is deliberately large. Sign-heavy views are the case where the port
// diverged from the reference client's cost, so the fixture keeps the same shape
// (many signs, all rotations, colour codes, full-length lines) available to
// every later change.

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "SharedConstants.h"
#include "client/gui/Font.h"
#include "client/renderer/Chunk.h"
#include "java/BufferedImage.h"
#include "java/File.h"
#include "java/Resource.h"
#include "nbt/CompoundTag.h"
#include "nbt/NbtIo.h"
#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"
#include "tools/headless/oracle/FontOracleData.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SignTile.h"
#include "world/level/tile/entity/SignTileEntity.h"

namespace
{

std::array<int_t, 256> productionCharWidths()
{
	std::unique_ptr<std::istream> is(Resource::getResource(u"/font/default.png"));
	BufferedImage image = BufferedImage::ImageIO_read(*is);
	return Font::computeCharWidths(image);
}

// One wall column: four lines mixing plain text, colour codes and the longest
// line the sign editor accepts.
void fillWallText(SignTileEntity &sign, int_t index)
{
	const jstring colors = u"0123456789abcdef";
	char16_t color = static_cast<char16_t>(colors[static_cast<size_t>(index) % colors.size()]);

	sign.messages[0] = u"Sign " + String::toString(index);
	sign.messages[1] = jstring(u"\u00a7") + color + u"colored";
	sign.messages[2] = (index % 3 == 0) ? jstring() : jstring(u"WWWWWWWWWWWWWWW");
	sign.messages[3] = jstring(u"\u00a7") + color + u"tail";
}

struct WallSign
{
	int_t x, y, z, data, index;
};

// Places `count` sign posts inside one chunk column and returns their sites.
std::vector<WallSign> buildSignWall(Level &level, int_t count)
{
	std::vector<WallSign> placed;
	placed.reserve(static_cast<size_t>(count));

	int_t index = 0;
	for (int_t y = 40; y < 128 && index < count; y += 2)
	{
		for (int_t z = 0; z < 16 && index < count; z++)
		{
			for (int_t x = 0; x < 16 && index < count; x++)
			{
				int_t data = index % 16;
				if (!level.setTileAndData(x, y, z, Tile::sign.id, data))
					continue;

				std::shared_ptr<SignTileEntity> sign = std::make_shared<SignTileEntity>();
				fillWallText(*sign, index);
				level.setTileEntity(x, y, z, sign);
				placed.push_back({ x, y, z, data, index });
				index++;
			}
		}
	}

	return placed;
}

void seedLevelData(File &directory, const jstring &name)
{
	std::unique_ptr<File> worldDirectory(File::open(directory, name));
	worldDirectory->mkdirs();
	std::unique_ptr<File> levelFile(File::open(*worldDirectory, u"level.dat"));

	CompoundTag root;
	std::unique_ptr<CompoundTag> data = std::make_unique<CompoundTag>();
	data->putLong(u"RandomSeed", 4242424242LL);
	data->putInt(u"SpawnX", 8);
	data->putInt(u"SpawnY", 64);
	data->putInt(u"SpawnZ", 8);
	data->putLong(u"Time", 0);
	data->putLong(u"SizeOnDisk", 0);
	root.putCompound(u"Data", std::move(data));

	std::unique_ptr<std::ostream> output(levelFile->toStreamOut());
	NbtIo::writeCompressed(root, *output);
}

}

// The glyph advances and string widths must equal what the reference font code
// produces on real Java for the same font texture.
HEADLESS_TEST(signs, font_metrics_match_java_oracle)
{
	std::array<int_t, 256> widths = productionCharWidths();

	for (int_t code = 0; code < 256; code++)
	{
		if (!ctx.checkEqual(widths[static_cast<size_t>(code)], FontOracle::charWidths[static_cast<size_t>(code)],
			"glyph advance for code " + std::to_string(code)))
			return;
	}

	for (const FontOracle::MeasuredLine &line : FontOracle::lines)
	{
		jstring text(line.text);
		ctx.checkEqual(Font::widthOf(widths, text), line.width,
			"measured width of sign line \"" + String::toUTF8(text) + "\"");
	}
}

HEADLESS_TEST(signs, color_codes_and_edit_markers_keep_alpha_widths)
{
	std::array<int_t, 256> widths = productionCharWidths();

	// Colour codes must not widen a line: the coloured copy measures the same
	// as the plain text it wraps.
	ctx.checkEqual(Font::widthOf(widths, u"\u00a7ccolored"), Font::widthOf(widths, u"colored"),
		"colour code adds no width");
	ctx.checkEqual(Font::widthOf(widths, u"tail\u00a7"), Font::widthOf(widths, u"tail") - 1,
		"dangling section sign keeps Alpha's negative one width");
	ctx.checkEqual(Font::widthOf(widths, u"> tail <"),
		Font::widthOf(widths, u"tail") + Font::widthOf(widths, u"> ") + Font::widthOf(widths, u" <"),
		"edited-line markers add their own width");
}

// A dense wall of signs must survive the production save and reopen with every
// line intact, and every sign must be a tile entity of the reloaded world.
HEADLESS_TEST(signs, sign_wall_survives_world_round_trip)
{
	headless::initGameRegistries();
	headless::TempDir directory(ctx, "sign-wall-round-trip");
	seedLevelData(directory.file(), u"SignWall");

	std::vector<WallSign> placed;
	{
		Level created(directory.newHandle(), u"SignWall", 1LL);
		placed = buildSignWall(created, 1024);
		if (!ctx.checkEqual(static_cast<long long>(placed.size()), 1024, "signs placed in wall"))
			return;
		created.save(true, nullptr);
	}

	Level reopened(directory.newHandle(), u"SignWall", 2LL);
	for (const WallSign &site : placed)
	{
		std::shared_ptr<TileEntity> loaded = reopened.getTileEntity(site.x, site.y, site.z);
		std::shared_ptr<SignTileEntity> sign = std::dynamic_pointer_cast<SignTileEntity>(loaded);
		if (!ctx.check(sign != nullptr, "sign tile entity at index " + std::to_string(site.index)))
			return;

		SignTileEntity expected;
		fillWallText(expected, site.index);
		for (int_t line = 0; line < 4; line++)
		{
			if (!ctx.check(sign->messages[line] == expected.messages[line],
				"sign " + std::to_string(site.index) + " line " + std::to_string(line)))
				return;
		}

		if (!ctx.checkEqual(reopened.getTile(site.x, site.y, site.z), Tile::sign.id,
			"sign block at index " + std::to_string(site.index)))
			return;
		if (!ctx.checkEqual(reopened.getData(site.x, site.y, site.z), site.data,
			"sign rotation at index " + std::to_string(site.index)))
			return;
	}
}

// The renderer keeps exactly one registration per sign while a wall-sized set of
// chunk sections rebuilds repeatedly.
HEADLESS_TEST(signs, wall_scale_rebuild_keeps_single_registrations)
{
	const int_t sections = 64;
	const int_t signsPerSection = 64;

	std::vector<std::vector<std::shared_ptr<TileEntity>>> sectionLocal(static_cast<size_t>(sections));
	std::vector<std::vector<std::shared_ptr<TileEntity>>> sectionDiscovered(static_cast<size_t>(sections));
	std::vector<std::shared_ptr<TileEntity>> global;

	for (int_t section = 0; section < sections; section++)
	{
		for (int_t i = 0; i < signsPerSection; i++)
			sectionDiscovered[static_cast<size_t>(section)].push_back(std::make_shared<SignTileEntity>());
	}

	for (int_t pass = 0; pass < 3; pass++)
	{
		for (int_t section = 0; section < sections; section++)
			Chunk::reconcileRenderableTileEntities(sectionLocal[static_cast<size_t>(section)], global,
				sectionDiscovered[static_cast<size_t>(section)]);

		if (!ctx.checkEqual(static_cast<long long>(global.size()),
			static_cast<long long>(sections) * signsPerSection,
			"registrations after rebuild pass " + std::to_string(pass)))
			return;
	}

	// Unloading the wall must leave nothing behind.
	for (int_t section = 0; section < sections; section++)
		Chunk::reconcileRenderableTileEntities(sectionLocal[static_cast<size_t>(section)], global, {});
	ctx.checkEqual(static_cast<long long>(global.size()), 0, "registrations after wall unload");
}

// Text layout for a full wall runs once per sign per frame in the client. The
// glyph lookup used to be a linear scan of the 144-character table, which cost
// more than the rest of the sign path put together.
HEADLESS_TEST(signs, wall_text_layout_stays_within_frame_budget)
{
	std::array<int_t, 256> widths = productionCharWidths();

	std::vector<SignTileEntity> wall(4324);
	for (size_t i = 0; i < wall.size(); i++)
		fillWallText(wall[i], static_cast<int_t>(i));

	auto start = std::chrono::steady_clock::now();
	long long total = 0;
	for (const SignTileEntity &sign : wall)
		for (int_t line = 0; line < 4; line++)
			total += Font::widthOf(widths, sign.messages[line]);
	double elapsedMs = std::chrono::duration<double, std::milli>(
		std::chrono::steady_clock::now() - start).count();

	ctx.check(total > 0, "wall text measured");
	// Debug builds are the slow case; 8 ms still leaves the frame budget to the
	// renderer, and the linear scan needed far more than this.
	ctx.check(elapsedMs < 8.0,
		"measuring 4324 signs takes " + std::to_string(elapsedMs) + " ms");
}
