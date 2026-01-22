#pragma once

#include "client/sound/Sound.h"
#include "java/String.h"
#include "java/Random.h"
#include <unordered_map>
#include <vector>
#include <string>

// Beta: SoundRepository class - stores and retrieves sounds (SoundRepository.java:11-52)
class SoundRepository
{
private:
	Random random;
	std::unordered_map<jstring, std::vector<Sound>> urls;
	std::vector<Sound> all;
	bool trimDigits = true;

public:
	int_t count = 0;

	// Beta: add(String name, File file) - adds sound to repository (SoundRepository.java:18-42)
	Sound add(const jstring &name, const std::string &filePath);

	// Beta: get(String name) - retrieves random sound with matching name (SoundRepository.java:44-47)
	Sound *get(const jstring &name);

	// Beta: any() - returns random sound from all sounds (SoundRepository.java:49-51)
	Sound *any();

	void setTrimDigits(bool trim) { trimDigits = trim; }
};