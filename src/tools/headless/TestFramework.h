#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "java/String.h"

// Minimal deterministic regression-test framework for the headless target.
//
// Tests self-register through HEADLESS_TEST so that adding a test requires no
// edit to any shared list. Test bodies report problems through TestContext;
// throwing is also treated as a failure by the runner.
namespace headless
{

enum class Outcome
{
	Pass,
	Fail,
	NotRun,
	Blocked
};

const char *outcomeName(Outcome outcome);

class TestContext
{
public:
	explicit TestContext(std::string testName);

	const std::string &testName() const { return name; }

	// Records a failure when condition is false. Returns condition so callers
	// can skip dependent work after the first problem.
	bool check(bool condition, const std::string &message);
	void fail(const std::string &message);

	// Marks the test as unable to run for a reason that is not a defect
	// (missing oracle data, unimplemented subsystem the test depends on).
	void block(const std::string &reason);

	bool checkEqual(long long actual, long long expected, const std::string &message);
	bool checkEqual(const std::string &actual, const std::string &expected, const std::string &message);
	bool checkEqual(const jstring &actual, const jstring &expected, const std::string &message);

	// Exact comparison of Java-visible floating point values. Java float/double
	// arithmetic is bit-defined, so parity tests compare raw bits rather than
	// an epsilon.
	bool checkEqualBits(float actual, float expected, const std::string &message);
	bool checkEqualBits(double actual, double expected, const std::string &message);

	Outcome outcome() const;
	const std::vector<std::string> &failures() const { return failureMessages; }
	const std::string &blockReason() const { return blocker; }

	// Set when the runner was asked to preserve temporary world data of tests
	// that failed.
	static bool keepFailedData;

private:
	std::string name;
	std::vector<std::string> failureMessages;
	std::string blocker;
	bool isBlocked = false;
};

using TestFunction = void (*)(TestContext &);

struct TestCase
{
	const char *suite;
	const char *name;
	TestFunction function;
};

const std::vector<TestCase> &registeredTests();
void registerTest(const TestCase &test);

struct TestRegistrar
{
	TestRegistrar(const char *suite, const char *name, TestFunction function);
};

}

#define HEADLESS_TEST(suiteId, testId)                                                    \
	static void headlessTestBody_##suiteId##_##testId(headless::TestContext &ctx);         \
	static const headless::TestRegistrar headlessTestRegistrar_##suiteId##_##testId(       \
		#suiteId, #testId, &headlessTestBody_##suiteId##_##testId);                        \
	static void headlessTestBody_##suiteId##_##testId(headless::TestContext &ctx)
