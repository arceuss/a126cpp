#include "client/sound/SoundRepository.h"

#include "util/Mth.h"
#include <algorithm>

// Beta: SoundRepository.add() - adds sound to repository (SoundRepository.java:18-42)
Sound SoundRepository::add(const jstring &name, const std::string &filePath)
{
	// Beta: Extract base name (remove extension)
	jstring baseName = name;
	size_t dotPos = baseName.find(u".");
	if (dotPos != jstring::npos)
	{
		baseName = baseName.substr(0, dotPos);
	}

	// Beta: Trim trailing digits if trimDigits is true (SoundRepository.java:22-26)
	if (trimDigits)
	{
		while (!baseName.empty() && baseName.back() >= u'0' && baseName.back() <= u'9')
		{
			baseName = baseName.substr(0, baseName.length() - 1);
		}
	}

	// Beta: Replace "/" with "." (SoundRepository.java:28)
	for (size_t i = 0; i < baseName.length(); ++i)
	{
		if (baseName[i] == u'/')
			baseName[i] = u'.';
	}

	// Beta: Add to map (SoundRepository.java:29-35)
	if (urls.find(baseName) == urls.end())
	{
		urls[baseName] = std::vector<Sound>();
	}

	Sound sound(name, filePath);
	urls[baseName].push_back(sound);
	all.push_back(sound);
	count++;

	return sound;
}

// Beta: SoundRepository.get() - retrieves random sound with matching name (SoundRepository.java:44-47)
// Beta 1.2 does NO processing in get() - it's a direct lookup!
// The caller must pass the processed key (e.g., "step.stone" not "step/stone1.ogg")
Sound *SoundRepository::get(const jstring &name)
{
	// Beta: Direct lookup without processing (SoundRepository.java:44-47)
	// When add("step/stone1.ogg") is called, it processes to "step.stone" and stores under that key
	// When get("step.stone") is called, it should do a direct lookup for "step.stone" - NO processing!
	auto it = urls.find(name);
	if (it == urls.end() || it->second.empty())
		return nullptr;

	// Beta: Return random sound from list
	std::vector<Sound> &sounds = it->second;
	return &sounds[random.nextInt((int_t)sounds.size())];
}

// Beta: SoundRepository.any() - returns random sound from all sounds (SoundRepository.java:49-51)
Sound *SoundRepository::any()
{
	if (all.empty())
		return nullptr;

	return &all[random.nextInt((int_t)all.size())];
}