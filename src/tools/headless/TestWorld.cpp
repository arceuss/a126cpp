#include "tools/headless/TestWorld.h"

#include <exception>
#include <stdexcept>

#include "network/Packet.h"
#include "tools/headless/TestFramework.h"
#include "world/item/Items.h"
#include "world/item/crafting/FurnaceRecipes.h"
#include "world/item/crafting/Recipes.h"
#include "world/level/tile/Tile.h"

namespace headless
{

static std::string dataRootPath = "build/headless-tests";

void initGameRegistries()
{
	static bool initialized = false;
	if (initialized)
		return;
	initialized = true;

	// Same order the client uses in Minecraft::init(): tiles, then items, then
	// the recipe tables that reference them.
	Tile::initTiles();
	Items::initItems();
	FurnaceRecipes::getInstance().init();
	Recipes::getInstance();
	Packet::ensurePacketRegistryInitialized();
}

void setDataRoot(const std::string &path)
{
	dataRootPath = path;
}

const std::string &dataRoot()
{
	return dataRootPath;
}

void removeRecursively(File &target)
{
	if (!target.exists())
		return;

	if (target.isDirectory())
	{
		std::vector<std::unique_ptr<File>> children = target.listFiles();
		for (std::unique_ptr<File> &child : children)
		{
			if (child != nullptr)
				removeRecursively(*child);
		}
	}

	target.remove();
}

TempDir::TempDir(TestContext &owner, const std::string &directoryName)
	: owner(owner), name(directoryName)
{
	std::unique_ptr<File> root(File::open(String::fromUTF8(dataRootPath)));
	if (root == nullptr)
		throw std::runtime_error("headless: cannot open test data root");
	root->mkdirs();

	handle.reset(File::open(*root, String::fromUTF8(directoryName)));
	if (handle == nullptr)
		throw std::runtime_error("headless: cannot open test directory " + directoryName);

	// Start from a clean slate so a previous run cannot influence this one.
	removeRecursively(*handle);
	handle->mkdirs();
}

TempDir::~TempDir()
{
	if (handle == nullptr)
		return;

	const bool failing = owner.outcome() == Outcome::Fail || std::uncaught_exceptions() > 0;
	if (TestContext::keepFailedData && failing)
		return;

	removeRecursively(*handle);
}

File *TempDir::newHandle() const
{
	std::unique_ptr<File> root(File::open(String::fromUTF8(dataRootPath)));
	return File::open(*root, String::fromUTF8(name));
}

jstring TempDir::path() const
{
	return handle->toString();
}

}
