#pragma once

#include <memory>
#include <string>

#include "java/File.h"
#include "java/String.h"

// Deterministic, isolated environment helpers for the headless suite.
//
// Nothing in here touches SDL, OpenGL, OpenAL or the user's real save folder.
// Every test that needs storage gets its own empty directory below the data
// root and that directory is removed again when the test finishes.
namespace headless
{

class TestContext;

// Initialises the tile, item, recipe and packet registries. Safe to call from
// every test; the work happens once.
void initGameRegistries();

void setDataRoot(const std::string &path);
const std::string &dataRoot();

void removeRecursively(File &target);

class TempDir
{
public:
	// The context is consulted on destruction: data of a failing test is kept
	// when the runner was started with --keep-failed-world.
	TempDir(TestContext &owner, const std::string &name);
	~TempDir();

	TempDir(const TempDir &) = delete;
	TempDir &operator=(const TempDir &) = delete;

	// Directory itself; usable as the "working directory" argument of Level.
	File &file() const { return *handle; }

	// A fresh File handle for APIs that take ownership, such as Level.
	File *newHandle() const;

	jstring path() const;

private:
	TestContext &owner;
	std::unique_ptr<File> handle;
	std::string name;
};

}
