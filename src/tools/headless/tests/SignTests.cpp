// Layer 8: tile-entity renderer membership without drawing.
//
// The production reconciliation function is a direct transliteration of
// Alpha WorldRenderer.func_1198_a (WorldRenderer.java:114-116,169-174).
// Tests drive that function without creating an OpenGL context.

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "SharedConstants.h"
#include "client/renderer/Chunk.h"
#include "tools/headless/TestFramework.h"
#include "world/level/tile/entity/SignTileEntity.h"

static int registrations(const std::vector<std::shared_ptr<TileEntity>> &global,
	const std::shared_ptr<TileEntity> &target)
{
	return static_cast<int>(std::count(global.begin(), global.end(), target));
}

HEADLESS_TEST(signs, repeated_rebuild_keeps_one_registration)
{
	std::vector<std::shared_ptr<TileEntity>> current;
	std::vector<std::shared_ptr<TileEntity>> global;
	std::shared_ptr<SignTileEntity> sign = std::make_shared<SignTileEntity>();
	const std::vector<std::shared_ptr<TileEntity>> discovered{ sign };

	for (int_t rebuild = 0; rebuild < 100; ++rebuild)
	{
		Chunk::reconcileRenderableTileEntities(current, global, discovered);
		if (!ctx.checkEqual(registrations(global, sign), 1,
			"sign registrations after rebuild " + std::to_string(rebuild)))
			return;
	}
	ctx.checkEqual(static_cast<long long>(global.size()), 1,
		"total registrations after 100 rebuilds");
}

HEADLESS_TEST(signs, renderer_move_removes_old_membership)
{
	std::vector<std::shared_ptr<TileEntity>> current;
	std::vector<std::shared_ptr<TileEntity>> global;
	std::shared_ptr<SignTileEntity> sign = std::make_shared<SignTileEntity>();
	Chunk::reconcileRenderableTileEntities(current, global, { sign });

	// setPos marks the renderer dirty; the following rebuild at the new
	// coordinates discovers no sign and performs this reconciliation.
	Chunk::reconcileRenderableTileEntities(current, global, {});
	ctx.checkEqual(registrations(global, sign), 0, "old sign after renderer move");
	ctx.check(current.empty(), "renderer-local membership after move");
}

HEADLESS_TEST(signs, unload_and_reload_are_symmetric)
{
	std::vector<std::shared_ptr<TileEntity>> current;
	std::vector<std::shared_ptr<TileEntity>> global;
	std::shared_ptr<SignTileEntity> sign = std::make_shared<SignTileEntity>();

	Chunk::reconcileRenderableTileEntities(current, global, { sign });
	Chunk::reconcileRenderableTileEntities(current, global, {});
	ctx.checkEqual(registrations(global, sign), 0, "sign registration after unload");

	Chunk::reconcileRenderableTileEntities(current, global, { sign });
	ctx.checkEqual(registrations(global, sign), 1, "sign registration after reload");
	ctx.checkEqual(static_cast<long long>(global.size()), 1, "total registrations after reload");
}

HEADLESS_TEST(signs, two_signs_produce_two_registrations)
{
	std::vector<std::shared_ptr<TileEntity>> current;
	std::vector<std::shared_ptr<TileEntity>> global;
	std::shared_ptr<SignTileEntity> first = std::make_shared<SignTileEntity>();
	std::shared_ptr<SignTileEntity> second = std::make_shared<SignTileEntity>();

	Chunk::reconcileRenderableTileEntities(current, global, { first, second });
	ctx.checkEqual(registrations(global, first), 1, "first sign registration");
	ctx.checkEqual(registrations(global, second), 1, "second sign registration");
	ctx.checkEqual(static_cast<long long>(global.size()), 2, "total two-sign registrations");
}

HEADLESS_TEST(signs, world_switch_leaves_no_old_registration)
{
	std::vector<std::shared_ptr<TileEntity>> global;
	std::vector<std::shared_ptr<TileEntity>> firstRenderer;
	std::shared_ptr<SignTileEntity> firstWorldSign = std::make_shared<SignTileEntity>();
	Chunk::reconcileRenderableTileEntities(firstRenderer, global, { firstWorldSign });

	// LevelRenderer::setLevel removes its chunks before binding the new level.
	Chunk::reconcileRenderableTileEntities(firstRenderer, global, {});

	std::vector<std::shared_ptr<TileEntity>> secondRenderer;
	std::shared_ptr<SignTileEntity> secondWorldSign = std::make_shared<SignTileEntity>();
	Chunk::reconcileRenderableTileEntities(secondRenderer, global, { secondWorldSign });

	ctx.checkEqual(registrations(global, firstWorldSign), 0, "old world sign registration");
	ctx.checkEqual(registrations(global, secondWorldSign), 1, "new world sign registration");
	ctx.checkEqual(static_cast<long long>(global.size()), 1, "total registrations after world switch");
}

// Sign and chat text rendering resolves one glyph index per character. The
// table must answer exactly what Alpha's linear
// `ChatAllowedCharacters.allowedCharacters.indexOf(c)` answers.
HEADLESS_TEST(signs, glyph_index_matches_linear_search)
{
	const jstring &letters = SharedConstants::acceptableLetters;
	ctx.check(!letters.empty(), "font character table is loaded");

	for (int_t code = 0; code < 0x10000; code++)
	{
		char_t c = static_cast<char_t>(code);
		size_t expected = letters.find(c);
		int_t actual = SharedConstants::letterIndex(c);
		int_t wanted = (expected == jstring::npos) ? -1 : static_cast<int_t>(expected);
		if (!ctx.checkEqual(actual, wanted, "glyph index for code " + std::to_string(code)))
			return;
	}

	// The quote character occurs twice in the table; first occurrence wins.
	ctx.checkEqual(SharedConstants::letterIndex(u'\''),
		static_cast<int_t>(letters.find(u'\'')), "duplicate glyph resolves to first slot");
}
