#pragma once

#include "java/String.h"
#include <string>

// Beta: Sound class - holds sound name and URL/path (Sound.java:5-13)
class Sound
{
public:
	jstring name;
	std::string filePath;  // File path instead of URL for C++

	Sound(const jstring &name, const std::string &filePath)
		: name(name), filePath(filePath) {}
};