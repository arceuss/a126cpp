#include "SharedConstants.h"

#include <vector>

#include "java/Resource.h"

namespace SharedConstants
{

static jstring readAcceptableChars()
{
	std::string result;

	std::unique_ptr<std::istream> is(Resource::getResource(u"/font.txt"));
	std::string line;
	while (std::getline(*is, line))
	{
		if (line.empty() || line[0] == '#')
			continue;
		if (line.back() == '\r')
			line.pop_back();
		result.append(line);
	}
	return String::fromUTF8(result);
}

// Alpha 1.2.6 uses protocol version 2000 in Packet1Login (specialized for alphaplace server)
// This value is sent as protocolVersion in login packet and used as entity ID
const int NETWORK_PROTOCOL_VERSION = 2000;
const int maxChatLength = 100;
const jstring acceptableLetters = readAcceptableChars();

static std::vector<int_t> buildLetterIndex()
{
	// One entry per UTF-16 code unit. First occurrence wins, matching
	// `acceptableLetters.find(c)`; the quote character appears twice.
	std::vector<int_t> table(0x10000, -1);
	for (size_t i = 0; i < acceptableLetters.size(); i++)
	{
		int_t &slot = table[static_cast<size_t>(acceptableLetters[i])];
		if (slot < 0)
			slot = static_cast<int_t>(i);
	}
	return table;
}

static const std::vector<int_t> letterIndexTable = buildLetterIndex();

int_t letterIndex(char_t c)
{
	return letterIndexTable[static_cast<size_t>(c)];
}

}
