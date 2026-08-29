#include "tools/headless/TestFramework.h"

#include <cstring>
#include <sstream>
#include <utility>

namespace headless
{

bool TestContext::keepFailedData = false;

const char *outcomeName(Outcome outcome)
{
	switch (outcome)
	{
		case Outcome::Pass: return "PASS";
		case Outcome::Fail: return "FAIL";
		case Outcome::NotRun: return "NOT_RUN";
		case Outcome::Blocked: return "BLOCKED";
	}
	return "NOT_RUN";
}

TestContext::TestContext(std::string testName) : name(std::move(testName))
{
}

bool TestContext::check(bool condition, const std::string &message)
{
	if (!condition)
		failureMessages.push_back(message);
	return condition;
}

void TestContext::fail(const std::string &message)
{
	failureMessages.push_back(message);
}

void TestContext::block(const std::string &reason)
{
	isBlocked = true;
	blocker = reason;
}

bool TestContext::checkEqual(long long actual, long long expected, const std::string &message)
{
	if (actual == expected)
		return true;

	std::ostringstream text;
	text << message << ": expected " << expected << ", got " << actual;
	failureMessages.push_back(text.str());
	return false;
}

bool TestContext::checkEqual(const std::string &actual, const std::string &expected, const std::string &message)
{
	if (actual == expected)
		return true;

	std::ostringstream text;
	text << message << ": expected \"" << expected << "\", got \"" << actual << '"';
	failureMessages.push_back(text.str());
	return false;
}

bool TestContext::checkEqual(const jstring &actual, const jstring &expected, const std::string &message)
{
	return checkEqual(String::toUTF8(actual), String::toUTF8(expected), message);
}

bool TestContext::checkEqualBits(float actual, float expected, const std::string &message)
{
	uint32_t actualBits = 0;
	uint32_t expectedBits = 0;
	std::memcpy(&actualBits, &actual, sizeof(actualBits));
	std::memcpy(&expectedBits, &expected, sizeof(expectedBits));
	if (actualBits == expectedBits)
		return true;

	std::ostringstream text;
	text << message << ": expected " << expected << " (bits 0x" << std::hex << expectedBits
		<< "), got " << std::dec << actual << " (bits 0x" << std::hex << actualBits << ')';
	failureMessages.push_back(text.str());
	return false;
}

bool TestContext::checkEqualBits(double actual, double expected, const std::string &message)
{
	uint64_t actualBits = 0;
	uint64_t expectedBits = 0;
	std::memcpy(&actualBits, &actual, sizeof(actualBits));
	std::memcpy(&expectedBits, &expected, sizeof(expectedBits));
	if (actualBits == expectedBits)
		return true;

	std::ostringstream text;
	text << message << ": expected " << expected << " (bits 0x" << std::hex << expectedBits
		<< "), got " << std::dec << actual << " (bits 0x" << std::hex << actualBits << ')';
	failureMessages.push_back(text.str());
	return false;
}

Outcome TestContext::outcome() const
{
	if (!failureMessages.empty())
		return Outcome::Fail;
	if (isBlocked)
		return Outcome::Blocked;
	return Outcome::Pass;
}

// Function-local storage keeps registration order independent of static
// initialisation order across translation units.
static std::vector<TestCase> &mutableTests()
{
	static std::vector<TestCase> tests;
	return tests;
}

const std::vector<TestCase> &registeredTests()
{
	return mutableTests();
}

void registerTest(const TestCase &test)
{
	mutableTests().push_back(test);
}

TestRegistrar::TestRegistrar(const char *suite, const char *name, TestFunction function)
{
	registerTest(TestCase{suite, name, function});
}

}
