#pragma once

#include "client/sound/SoundRepository.h"
#include "client/Options.h"
#include "world/entity/Mob.h"
#include "java/String.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations for OpenAL
// Include OpenAL headers to get proper type definitions
#include <AL/al.h>
#include <AL/alc.h>

// Beta: SoundEngine class - wraps OpenAL-Soft (SoundEngine.java:14-193)
class SoundEngine
{
private:
	static constexpr int_t SOUND_DISTANCE = 16;
	
	static bool loaded;
	ALCdevice *device = nullptr;
	ALCcontext *context = nullptr;
	
	SoundRepository sounds;
	SoundRepository streamingSounds;
	SoundRepository songs;
	int_t idCounter = 0;
	Options *options = nullptr;
	Random random;
	int_t noMusicDelay = 0;

	// Beta: Per-source metadata for manual gain calculation (matching Paulscode SourceLWJGLOpenAL)
	struct SourceInfo
	{
		float x, y, z;  // Source position (SourceLWJGLOpenAL.java:position)
		float distOrRoll;  // Distance/rolloff for attModel == 2 (SourceLWJGLOpenAL.java:distOrRoll)
		float sourceVolume;  // Source volume multiplier (SourceLWJGLOpenAL.java:sourceVolume)
		int attModel;  // Attenuation model: 0=none, 1=rolloff, 2=linear (SourceLWJGLOpenAL.java:attModel)
		
		SourceInfo() : x(0.0f), y(0.0f), z(0.0f), distOrRoll(0.0f), sourceVolume(1.0f), attModel(0) {}
		SourceInfo(float x, float y, float z, float distOrRoll, float sourceVolume, int attModel)
			: x(x), y(y), z(z), distOrRoll(distOrRoll), sourceVolume(sourceVolume), attModel(attModel) {}
	};

	// OpenAL source management
	std::unordered_map<std::string, ALuint> activeSources;  // Map of source IDs to OpenAL sources (currently playing)
	std::unordered_map<std::string, SourceInfo> sourceInfoMap;  // Map of source IDs to source metadata (for manual gain calculation)
	std::vector<ALuint> freeSources;  // Pool of available sources
	std::unordered_map<std::string, ALuint> soundBuffers;  // Cache of loaded sound buffers
	ALuint musicSource = 0;  // Background music source
	ALuint streamingSource = 0;  // Streaming source
	static constexpr int_t MAX_SOURCES = 32;  // Beta: Limited source pool (Paulscode uses similar limits)
	
	// Listener position tracking (for manual gain calculation)
	float listenerX = 0.0f, listenerY = 0.0f, listenerZ = 0.0f;

	// Helper methods
	bool initOpenAL();
	void cleanupOpenAL();
	ALuint getOrCreateSource(const std::string &id, bool streaming);
	void releaseSource(const std::string &id);
	ALuint loadOGGFile(const std::string &filePath, bool isMUS = false);
	bool loadSound(const Sound &sound, ALuint &buffer, bool isMUS = false);
	void updateListener(float x, float y, float z, float forwardX, float forwardY, float forwardZ, float upX, float upY, float upZ);
	void checkAndReleaseFinishedSources();
	void updateSourceGains();  // Beta: Update all active source gains based on distance (matching Paulscode positionChanged)

public:
	SoundEngine();
	~SoundEngine();

	// Beta: init(Options options) - initializes sound system (SoundEngine.java:26-32)
	void init(Options *options);

	// Beta: updateOptions() - updates sound options (SoundEngine.java:57-69)
	void updateOptions();

	// Beta: destroy() - cleans up sound system (SoundEngine.java:71-75)
	void destroy();

	// Beta: add(String name, File file) - adds sound (SoundEngine.java:77-79)
	void add(const jstring &name, const std::string &filePath);

	// Beta: addStreaming(String name, File file) - adds streaming sound (SoundEngine.java:81-83)
	void addStreaming(const jstring &name, const std::string &filePath);

	// Beta: addMusic(String name, File file) - adds music (SoundEngine.java:85-87)
	void addMusic(const jstring &name, const std::string &filePath);

	// Beta: playMusicTick() - plays background music (SoundEngine.java:89-106)
	void playMusicTick();

	// Beta: update(Mob player, float a) - updates listener position (SoundEngine.java:108-127)
	void update(Mob *player, float a);

	// Beta: playStreaming(String name, float x, float y, float z, float volume, float pitch) - plays streaming sound (SoundEngine.java:129-150)
	void playStreaming(const jstring &name, float x, float y, float z, float volume, float pitch);

	// Beta: play(String name, float x, float y, float z, float volume, float pitch) - plays 3D sound (SoundEngine.java:152-173)
	void play(const jstring &name, float x, float y, float z, float volume, float pitch);

	// Beta: playUI(String name, float volume, float pitch) - plays UI sound (SoundEngine.java:175-192)
	void playUI(const jstring &name, float volume, float pitch);
};