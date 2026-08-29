// Headless deterministic regression runner for a126cpp.
//
// This target links the same production code as the client but never
// initialises SDL, OpenGL, OpenAL or any input device, so it can run in CI or
// over a terminal session. Failures make the process exit non-zero; a suite
// that could not run reports NOT_RUN or BLOCKED instead of silently passing.

#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "tools/headless/TestFramework.h"
#include "tools/headless/TestWorld.h"

static std::string fullName(const headless::TestCase &test)
{
	return std::string(test.suite) + '.' + test.name;
}

static void printUsage()
{
	std::cout <<
		"a126cpp headless regression suite\n"
		"\n"
		"Usage: a126cpp-headless-tests [options]\n"
		"\n"
		"  --all                 Run every registered test.\n"
		"  --<suite>             Run one suite, for example --rng or --nbt.\n"
		"  --test <suite.name>   Run a single test.\n"
		"  --list                List all registered tests and exit.\n"
		"  --suites              List all suite names and exit.\n"
		"  --data-dir <path>     Root for temporary test data.\n"
		"                        Default: build/headless-tests\n"
		"  --keep-failed-world   Keep temporary data of failing tests.\n"
		"  --help                Show this message.\n";
}

int main(int argc, char *argv[])
{
	std::vector<headless::TestCase> tests = headless::registeredTests();

	// Registration order depends on translation unit initialisation order, so
	// sort to make the run order reproducible.
	std::sort(tests.begin(), tests.end(),
		[](const headless::TestCase &a, const headless::TestCase &b)
		{
			const int suiteOrder = std::strcmp(a.suite, b.suite);
			if (suiteOrder != 0)
				return suiteOrder < 0;
			return std::strcmp(a.name, b.name) < 0;
		});

	std::set<std::string> suites;
	for (const headless::TestCase &test : tests)
		suites.insert(test.suite);

	bool runAll = false;
	std::set<std::string> selectedSuites;
	std::set<std::string> selectedTests;

	for (int i = 1; i < argc; ++i)
	{
		const std::string argument = argv[i];

		if (argument == "--help" || argument == "-h")
		{
			printUsage();
			return 0;
		}
		if (argument == "--list")
		{
			for (const headless::TestCase &test : tests)
				std::cout << fullName(test) << '\n';
			return 0;
		}
		if (argument == "--suites")
		{
			for (const std::string &suite : suites)
				std::cout << suite << '\n';
			return 0;
		}
		if (argument == "--all")
		{
			runAll = true;
			continue;
		}
		if (argument == "--keep-failed-world")
		{
			headless::TestContext::keepFailedData = true;
			continue;
		}
		if (argument == "--test")
		{
			if (i + 1 >= argc)
			{
				std::cerr << "--test requires a test name\n";
				return 2;
			}
			selectedTests.insert(argv[++i]);
			continue;
		}
		if (argument == "--data-dir")
		{
			if (i + 1 >= argc)
			{
				std::cerr << "--data-dir requires a path\n";
				return 2;
			}
			headless::setDataRoot(argv[++i]);
			continue;
		}
		if (argument.rfind("--", 0) == 0)
		{
			const std::string suite = argument.substr(2);
			if (suites.find(suite) == suites.end())
			{
				std::cerr << "Unknown option or suite: " << argument << '\n';
				std::cerr << "Known suites:";
				for (const std::string &known : suites)
					std::cerr << ' ' << known;
				std::cerr << '\n';
				return 2;
			}
			selectedSuites.insert(suite);
			continue;
		}

		std::cerr << "Unexpected argument: " << argument << '\n';
		printUsage();
		return 2;
	}

	if (!runAll && selectedSuites.empty() && selectedTests.empty())
	{
		printUsage();
		return 2;
	}

	headless::initGameRegistries();

	int passed = 0;
	int failed = 0;
	int blocked = 0;
	int skipped = 0;

	for (const headless::TestCase &test : tests)
	{
		const std::string name = fullName(test);
		const bool selected = runAll
			|| selectedSuites.find(test.suite) != selectedSuites.end()
			|| selectedTests.find(name) != selectedTests.end();
		if (!selected)
		{
			++skipped;
			continue;
		}

		headless::TestContext context(name);
		try
		{
			test.function(context);
		}
		catch (const std::exception &problem)
		{
			context.fail(std::string("uncaught exception: ") + problem.what());
		}
		catch (...)
		{
			context.fail("uncaught non-standard exception");
		}

		const headless::Outcome outcome = context.outcome();
		std::cout << '[' << headless::outcomeName(outcome) << "] " << name << '\n';
		if (outcome == headless::Outcome::Blocked)
		{
			++blocked;
			std::cout << "       " << context.blockReason() << '\n';
		}
		else if (outcome == headless::Outcome::Fail)
		{
			++failed;
			for (const std::string &message : context.failures())
				std::cout << "       " << message << '\n';
		}
		else
		{
			++passed;
		}
		std::cout.flush();
	}

	// Requesting a specific test that does not exist must not look like success.
	std::vector<std::string> unknownTests;
	for (const std::string &requested : selectedTests)
	{
		const bool exists = std::any_of(tests.begin(), tests.end(),
			[&requested](const headless::TestCase &test) { return fullName(test) == requested; });
		if (!exists)
			unknownTests.push_back(requested);
	}
	for (const std::string &unknown : unknownTests)
		std::cout << "[NOT_RUN] " << unknown << "\n       no such test\n";

	std::cout << "\n"
		<< passed << " passed, "
		<< failed << " failed, "
		<< blocked << " blocked, "
		<< unknownTests.size() << " not run";
	if (skipped > 0)
		std::cout << ", " << skipped << " deselected";
	std::cout << '\n';

	if (failed > 0 || !unknownTests.empty())
		return 1;
	return 0;
}
