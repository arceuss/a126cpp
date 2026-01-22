#include "client/sound/SoundEngine.h"

#include "client/Options.h"
#include "world/entity/Mob.h"
#include "util/Mth.h"
#include "java/String.h"
#include "util/musDecode.h"

// OpenAL headers are already included in SoundEngine.h

#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cfloat>  // For FLT_MAX

// Include stb_vorbis for OGG decoding
// stb_vorbis.cpp is already compiled separately, so we forward declare the function we need
// This avoids including the implementation multiple times
extern "C" {
	typedef unsigned char uint8;
	int stb_vorbis_decode_memory(const uint8 *mem, int len, int *channels, int *sample_rate, short **output);
}

// Beta: SoundEngine - OpenAL-Soft wrapper matching Beta 1.2's SoundEngine.java
// Uses OpenAL-Soft directly (Paulscode is based on OpenAL) for accurate Beta 1.2 sound behavior

bool SoundEngine::loaded = false;

SoundEngine::SoundEngine()
{
	noMusicDelay = random.nextInt(12000);
}

SoundEngine::~SoundEngine()
{
	destroy();
}

// Beta: init(Options options) - initializes sound system (SoundEngine.java:26-32)
void SoundEngine::init(Options *options)
{
	this->options = options;
	this->streamingSounds.setTrimDigits(false);
	
	if (!loaded && (options == nullptr || options->sound != 0.0f || options->music != 0.0f))
	{
		if (initOpenAL())
		{
			loaded = true;
		}
	}
}

bool SoundEngine::initOpenAL()
{
	// Beta: Initialize OpenAL device and context (matching Paulscode LibraryLWJGLOpenAL behavior)
	device = alcOpenDevice(nullptr);
	if (!device)
	{
		std::cerr << "Failed to open OpenAL device" << std::endl;
		return false;
	}

	// Beta: Create context with default attributes (OpenAL 2013 doesn't support HRTF)
	context = alcCreateContext(device, nullptr);
	if (!context)
	{
		std::cerr << "Failed to create OpenAL context" << std::endl;
		alcCloseDevice(device);
		device = nullptr;
		return false;
	}

	if (!alcMakeContextCurrent(context))
	{
		std::cerr << "Failed to make OpenAL context current" << std::endl;
		alcDestroyContext(context);
		alcCloseDevice(device);
		context = nullptr;
		device = nullptr;
		return false;
	}

	// Generate source pool (Beta: Limited sources similar to Paulscode)
	freeSources.resize(MAX_SOURCES);
	alGenSources((ALsizei)freeSources.size(), freeSources.data());
	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		std::cerr << "Failed to generate OpenAL sources: " << alGetString(err) << std::endl;
		cleanupOpenAL();
		return false;
	}

	// Generate special sources for music and streaming
	alGenSources(1, &musicSource);
	alGenSources(1, &streamingSource);

	// Beta: Set distance model for 3D audio (matching Paulscode LibraryLWJGLOpenAL behavior)
	// Paulscode uses AL_INVERSE_DISTANCE (not CLAMPED) for Beta 1.2
	alDistanceModel(AL_INVERSE_DISTANCE);
	alDopplerFactor(1.0f);
	alDopplerVelocity(343.0f);  // Speed of sound in m/s
	
	// Beta: Set listener properties (Paulscode sets these on init)
	alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
	alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	float orientation[6] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	alListenerfv(AL_ORIENTATION, orientation);
	
	// Beta: Clear any errors from initialization
	alGetError();

	return true;
}

void SoundEngine::cleanupOpenAL()
{
	// Stop all active sources
	for (auto &pair : activeSources)
	{
		ALuint source = pair.second;
		alSourceStop(source);
		alSourcei(source, AL_BUFFER, 0);
	}
	activeSources.clear();
	sourceInfoMap.clear();  // Beta: Clear source metadata

	// Clean up source pool
	if (!freeSources.empty())
	{
		alDeleteSources((ALsizei)freeSources.size(), freeSources.data());
		freeSources.clear();
	}

	if (musicSource != 0)
	{
		alSourceStop(musicSource);
		alSourcei(musicSource, AL_BUFFER, 0);
		alDeleteSources(1, &musicSource);
		musicSource = 0;
	}

	if (streamingSource != 0)
	{
		alSourceStop(streamingSource);
		alSourcei(streamingSource, AL_BUFFER, 0);
		alDeleteSources(1, &streamingSource);
		streamingSource = 0;
	}

	// Clean up buffers
	for (auto &pair : soundBuffers)
	{
		ALuint buffer = pair.second;
		alDeleteBuffers(1, &buffer);
	}
	soundBuffers.clear();

	if (context)
	{
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		context = nullptr;
	}

	if (device)
	{
		alcCloseDevice(device);
		device = nullptr;
	}
}

void SoundEngine::updateOptions()
{
	if (!loaded && options && (options->sound != 0.0f || options->music != 0.0f))
	{
		if (initOpenAL())
			loaded = true;
	}

	if (loaded)
	{
		// Beta: Update music volume (SoundEngine.java:63-68)
		if (options->music == 0.0f)
		{
			if (musicSource != 0)
			{
				alSourceStop(musicSource);
				alSourcei(musicSource, AL_BUFFER, 0);
			}
		}
		else if (musicSource != 0)
		{
			ALint state;
			alGetSourcei(musicSource, AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING)
			{
				alSourcef(musicSource, AL_GAIN, options->music);
			}
		}
		
		// Beta: Update streaming source volume when options change
		// Update sourceVolume in sourceInfoMap and recalculate gain
		std::string streamingId = "streaming";
		auto streamingInfoIt = sourceInfoMap.find(streamingId);
		if (streamingInfoIt != sourceInfoMap.end() && streamingSource != 0)
		{
			ALint state;
			alGetSourcei(streamingSource, AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING || state == AL_PAUSED)
			{
				SourceInfo &info = streamingInfoIt->second;
				// Update sourceVolume with new options.sound value
				info.sourceVolume = 0.5f * options->sound;
				
				// Recalculate gain based on distance (same as updateSourceGains())
				float currentListenerX, currentListenerY, currentListenerZ;
				alGetListener3f(AL_POSITION, &currentListenerX, &currentListenerY, &currentListenerZ);
				float dX = info.x - currentListenerX;
				float dY = info.y - currentListenerY;
				float dZ = info.z - currentListenerZ;
				float distanceFromListener = std::sqrt(dX * dX + dY * dY + dZ * dZ);
				
				float gain = 1.0f;
				if (info.attModel == 2)
				{
					if (distanceFromListener <= 0.0f)
					{
						gain = 1.0f;
					}
					else if (distanceFromListener >= info.distOrRoll)
					{
						gain = 0.0f;
					}
					else
					{
						gain = 1.0f - distanceFromListener / info.distOrRoll;
					}
					if (gain > 1.0f) gain = 1.0f;
					if (gain < 0.0f) gain = 0.0f;
				}
				
				// Update AL_GAIN with new volume
				alSourcef(streamingSource, AL_GAIN, gain * info.sourceVolume);
			}
		}
		
		// Beta: Stop streaming if sound is turned off
		if (options->sound == 0.0f && streamingSource != 0)
		{
			ALint state;
			alGetSourcei(streamingSource, AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING || state == AL_PAUSED)
			{
				alSourceStop(streamingSource);
				alSourcei(streamingSource, AL_BUFFER, 0);
				sourceInfoMap.erase(streamingId);
			}
		}
	}
}

void SoundEngine::destroy()
{
	if (loaded)
	{
		cleanupOpenAL();
		loaded = false;
	}
}

void SoundEngine::add(const jstring &name, const std::string &filePath)
{
	// Beta: Add sound to repository (SoundEngine.java:77-79)
	sounds.add(name, filePath);
	
	// Beta: Preload buffer immediately to prevent unloading
	// Unlike Beta 1.2 which uses Paulscode's lazy loading, we preload for OpenAL-Soft
	// If OpenAL isn't initialized yet, the buffer will be loaded on first play
	if (loaded)
	{
		ALuint buffer = loadOGGFile(filePath);
		if (buffer == 0)
		{
			std::cerr << "Warning: Failed to preload sound buffer for " << filePath << std::endl;
		}
	}
	// If not loaded yet, the buffer will be loaded lazily on first play (loadOGGFile caches it)
}

void SoundEngine::addStreaming(const jstring &name, const std::string &filePath)
{
	streamingSounds.add(name, filePath);
}

void SoundEngine::addMusic(const jstring &name, const std::string &filePath)
{
	songs.add(name, filePath);
}

// Beta: playMusicTick() - plays background music (SoundEngine.java:89-106)
void SoundEngine::playMusicTick()
{
	if (!loaded || !options || options->music == 0.0f || musicSource == 0)
		return;

	ALint musicState;
	alGetSourcei(musicSource, AL_SOURCE_STATE, &musicState);

	ALint streamState = AL_STOPPED;
	if (streamingSource != 0)
		alGetSourcei(streamingSource, AL_SOURCE_STATE, &streamState);

	if (musicState == AL_PLAYING || streamState == AL_PLAYING)
	{
		if (noMusicDelay > 0)
			noMusicDelay--;
		return;
	}

	if (noMusicDelay > 0)
	{
		noMusicDelay--;
		return;
	}

	Sound *song = songs.any();
	if (song != nullptr)
	{
		noMusicDelay = random.nextInt(12000) + 12000;

		// Load and play background music
		ALuint buffer = loadOGGFile(song->filePath);
		if (buffer != 0)
		{
			alSourceStop(musicSource);
			alSourcei(musicSource, AL_BUFFER, buffer);
			alSourcei(musicSource, AL_LOOPING, AL_TRUE);
			alSourcef(musicSource, AL_GAIN, options->music);
			alSource3f(musicSource, AL_POSITION, 0.0f, 0.0f, 0.0f);
			alSourcef(musicSource, AL_REFERENCE_DISTANCE, 1.0f);
			alSourcef(musicSource, AL_MAX_DISTANCE, 10000.0f);
			alSourcePlay(musicSource);
		}
	}
}

void SoundEngine::update(Mob *player, float a)
{
	if (!loaded || !options || options->sound == 0.0f || !player)
		return;

	// Beta: Update listener position based on player (SoundEngine.java:108-127)
	float yRot = player->yRotO + (player->yRot - player->yRotO) * a;
	double x = player->xo + (player->x - player->xo) * a;
	double y = player->yo + (player->y - player->yo) * a;
	double z = player->zo + (player->z - player->zo) * a;

	float yCos = Mth::cos(-yRot * Mth::DEGRAD - Mth::PI);
	float ySin = Mth::sin(-yRot * Mth::DEGRAD - Mth::PI);
	float forwardX = -ySin;
	float forwardY = 0.0f;
	float forwardZ = -yCos;
	float upX = 0.0f;
	float upY = 1.0f;
	float upZ = 0.0f;

	updateListener((float)x, (float)y, (float)z, forwardX, forwardY, forwardZ, upX, upY, upZ);
	checkAndReleaseFinishedSources();
}

void SoundEngine::updateListener(float x, float y, float z, float forwardX, float forwardY, float forwardZ, float upX, float upY, float upZ)
{
	// Beta: Update listener position (LibraryLWJGLOpenAL.java:65)
	alListener3f(AL_POSITION, x, y, z);
	float orientation[6] = { forwardX, forwardY, forwardZ, upX, upY, upZ };
	alListenerfv(AL_ORIENTATION, orientation);
	
	// Beta: Store listener position for manual gain calculation (SourceLWJGLOpenAL.java:listenerPosition)
	listenerX = x;
	listenerY = y;
	listenerZ = z;
	
	// Beta: Update all active source gains based on new listener position (SourceLWJGLOpenAL.java:positionChanged)
	updateSourceGains();
}

void SoundEngine::checkAndReleaseFinishedSources()
{
	// Beta: Check all active sources and release finished ones back to pool
	// Beta: Sources transition: AL_INITIAL -> AL_PLAYING -> AL_STOPPED
	// We should release sources that are STOPPED (finished) or INITIAL (never started/stopped)
	std::vector<std::string> toRelease;
	for (auto it = activeSources.begin(); it != activeSources.end();)
	{
		ALuint source = it->second;
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		
		// Beta: Release sources that are stopped (finished playing)
		// Don't release AL_PLAYING or AL_PAUSED sources
		if (state == AL_STOPPED || state == AL_INITIAL)
		{
			// Beta: Properly reset source before returning to pool (matching Paulscode)
			// Detach buffer first, then reset all properties
			alSourceStop(source);
			alSourcei(source, AL_BUFFER, 0);  // Detach buffer (OpenAL allows buffer to be reused)
			alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
			alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
			alSourcef(source, AL_GAIN, 1.0f);
			alSourcef(source, AL_PITCH, 1.0f);
			alSourcef(source, AL_REFERENCE_DISTANCE, 1.0f);
			alSourcef(source, AL_MAX_DISTANCE, FLT_MAX);
			alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);
			alSourcei(source, AL_LOOPING, AL_FALSE);
			
			// Clear any errors from reset
			alGetError();
			
			freeSources.push_back(source);
			toRelease.push_back(it->first);
			++it;
		}
		else
		{
			++it;
		}
	}

	// Beta: Erase released sources after iteration (safe erase)
	for (const auto &id : toRelease)
	{
		activeSources.erase(id);
		sourceInfoMap.erase(id);  // Beta: Also remove source metadata
	}
}

// Beta: Update all active source gains based on distance (matching Paulscode SourceLWJGLOpenAL.java:positionChanged, 199-208)
void SoundEngine::updateSourceGains()
{
	// Beta: For each active source, calculate distance and update gain (SourceLWJGLOpenAL.java:calculateDistance, calculateGain)
	for (auto &pair : activeSources)
	{
		const std::string &id = pair.first;
		ALuint source = pair.second;
		
		// Beta: Check if source is still playing
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING && state != AL_PAUSED)
			continue;  // Skip stopped sources (they'll be cleaned up by checkAndReleaseFinishedSources)
		
		auto infoIt = sourceInfoMap.find(id);
		if (infoIt == sourceInfoMap.end())
			continue;  // No metadata for this source
		
		const SourceInfo &info = infoIt->second;
		
		// Beta: Calculate distance from listener (SourceLWJGLOpenAL.java:calculateDistance, 417-424)
		// Beta: Read listener position from OpenAL (matching Paulscode's listenerPosition FloatBuffer)
		float currentListenerX, currentListenerY, currentListenerZ;
		alGetListener3f(AL_POSITION, &currentListenerX, &currentListenerY, &currentListenerZ);
		float dX = info.x - currentListenerX;
		float dY = info.y - currentListenerY;
		float dZ = info.z - currentListenerZ;
		float distanceFromListener = std::sqrt(dX * dX + dY * dY + dZ * dZ);
		
		// Beta: Calculate gain based on attenuation model (SourceLWJGLOpenAL.java:calculateGain, 426-446)
		float gain = 1.0f;  // Beta: Default gain (SourceLWJGLOpenAL.java:444)
		if (info.attModel == 2)
		{
			if (distanceFromListener <= 0.0f)
			{
				gain = 1.0f;  // Beta: SourceLWJGLOpenAL.java:429
			}
			else if (distanceFromListener >= info.distOrRoll)
			{
				gain = 0.0f;  // Beta: SourceLWJGLOpenAL.java:431
			}
			else
			{
				gain = 1.0f - distanceFromListener / info.distOrRoll;  // Beta: SourceLWJGLOpenAL.java:433
			}
			if (gain > 1.0f)
				gain = 1.0f;  // Beta: SourceLWJGLOpenAL.java:437
			if (gain < 0.0f)
				gain = 0.0f;  // Beta: SourceLWJGLOpenAL.java:441
		}
		
		// Beta: Update AL_GAIN = gain * sourceVolume (SourceLWJGLOpenAL.java:203, no fadeOut/fadeIn for simple sounds)
		alSourcef(source, AL_GAIN, gain * info.sourceVolume);
	}
	
	// Beta: Also update streaming source if it's playing (streaming source is not in activeSources)
	std::string streamingId = "streaming";
	auto streamingInfoIt = sourceInfoMap.find(streamingId);
	if (streamingInfoIt != sourceInfoMap.end() && streamingSource != 0)
	{
		ALint state;
		alGetSourcei(streamingSource, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING || state == AL_PAUSED)
		{
			const SourceInfo &info = streamingInfoIt->second;
			
			// Beta: Calculate distance from listener
			float currentListenerX, currentListenerY, currentListenerZ;
			alGetListener3f(AL_POSITION, &currentListenerX, &currentListenerY, &currentListenerZ);
			float dX = info.x - currentListenerX;
			float dY = info.y - currentListenerY;
			float dZ = info.z - currentListenerZ;
			float distanceFromListener = std::sqrt(dX * dX + dY * dY + dZ * dZ);
			
			// Beta: Calculate gain based on attenuation model
			float gain = 1.0f;
			if (info.attModel == 2)
			{
				if (distanceFromListener <= 0.0f)
				{
					gain = 1.0f;
				}
				else if (distanceFromListener >= info.distOrRoll)
				{
					gain = 0.0f;
				}
				else
				{
					gain = 1.0f - distanceFromListener / info.distOrRoll;
				}
				if (gain > 1.0f)
					gain = 1.0f;
				if (gain < 0.0f)
					gain = 0.0f;
			}
			
			// Beta: Update AL_GAIN = gain * sourceVolume
			alSourcef(streamingSource, AL_GAIN, gain * info.sourceVolume);
		}
	}
}

// Beta: playStreaming() - plays streaming sound (SoundEngine.java:129-150)
void SoundEngine::playStreaming(const jstring &name, float x, float y, float z, float volume, float pitch)
{
	// Java: if (loaded && this.options.sound != 0.0F) { (line 130)
	if (!loaded || !options || options->sound == 0.0f || streamingSource == 0)
		return;

	// Java: String id = "streaming"; (line 131)
	std::string id = "streaming";
	
	// Java: if (soundSystem.playing("streaming")) { soundSystem.stop("streaming"); } (lines 132-134)
	ALint state;
	alGetSourcei(streamingSource, AL_SOURCE_STATE, &state);
	if (state == AL_PLAYING || state == AL_PAUSED)
	{
		alSourceStop(streamingSource);
		alSourcei(streamingSource, AL_BUFFER, 0);
	}

	// Java: if (name != null) { (line 136)
	// Beta: If name is empty/null, just stop and clear source info (used when record is removed)
	if (name.empty())
	{
		// Ensure source is stopped and cleared (even if it wasn't playing)
		alSourceStop(streamingSource);
		alSourcei(streamingSource, AL_BUFFER, 0);
		sourceInfoMap.erase(id);
		return;
	}

	Sound *sound = streamingSounds.get(name);
	if (sound == nullptr || volume <= 0.0f)
		return;

	// Beta: Stop background music if streaming starts (SoundEngine.java:139-141)
	if (musicSource != 0)
	{
		alGetSourcei(musicSource, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING)
		{
			alSourceStop(musicSource);
			alSourcei(musicSource, AL_BUFFER, 0);
		}
	}

	// Java: float dist = 16.0F; (line 143)
	// Java: soundSystem.newStreamingSource(true, id, sound.url, sound.name, false, x, y, z, 2, dist * 4.0F); (line 144)
	// Beta: dist * 4.0F = 64.0F for streaming (matching newb12)
	float dist = 16.0f * 4.0f;  // dist * 4.0F = 64.0F

	// Load streaming sound (MUS files are XOR-encrypted OGG)
	ALuint buffer = loadOGGFile(sound->filePath, true);  // isMUS = true for streaming sounds
	if (buffer == 0)
	{
		// If buffer load fails, clear source info
		sourceInfoMap.erase(id);
		return;
	}

	// Beta: Set streaming source properties (matching Paulscode newStreamingSource with attModel = 2)
	// Java: soundSystem.newStreamingSource(true, id, sound.url, sound.name, false, x, y, z, 2, dist * 4.0F); (line 144)
	// Ensure source is clean before setting buffer (source was already stopped above if playing)
	alSourcei(streamingSource, AL_BUFFER, 0);  // Clear buffer first to ensure clean state
	alSource3f(streamingSource, AL_POSITION, x, y, z);  // Set position first
	alSource3f(streamingSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);  // Set velocity
	alSourcei(streamingSource, AL_SOURCE_RELATIVE, AL_FALSE);  // World-relative
	alSourcei(streamingSource, AL_BUFFER, buffer);  // Set buffer after position/velocity
	
	// Beta: Attenuation model 2 (SourceLWJGLOpenAL.java:299-303, 236-246)
	// Beta: For attModel != 1, Paulscode sets AL_ROLLOFF_FACTOR = 0.0F (disables OpenAL distance attenuation)
	alSourcef(streamingSource, AL_ROLLOFF_FACTOR, 0.0f);  // Beta: Disable OpenAL distance attenuation (SourceLWJGLOpenAL.java:242, 302)
	
	// Java: soundSystem.setPitch(id, pitch) - MUST be called after newSource, before setVolume
	alSourcef(streamingSource, AL_PITCH, pitch);
	
	// Java: soundSystem.setVolume(id, 0.5F * this.options.sound); (line 145)
	float sourceVolume = 0.5f * options->sound;
	sourceInfoMap[id] = SourceInfo(x, y, z, dist, sourceVolume, 2);  // attModel = 2
	
	// Beta: Calculate initial gain based on current listener position (SourceLWJGLOpenAL.java:calculateGain, 426-446)
	// Beta: gain = 1.0F - distanceFromListener / distOrRoll (for attModel == 2)
	float currentListenerX, currentListenerY, currentListenerZ;
	alGetListener3f(AL_POSITION, &currentListenerX, &currentListenerY, &currentListenerZ);
	float dX = x - currentListenerX;
	float dY = y - currentListenerY;
	float dZ = z - currentListenerZ;
	float distanceFromListener = std::sqrt(dX * dX + dY * dY + dZ * dZ);
	float gain = 1.0f;  // Beta: Default gain (SourceLWJGLOpenAL.java:444)
	if (distanceFromListener <= 0.0f)
	{
		gain = 1.0f;  // Beta: SourceLWJGLOpenAL.java:429
	}
	else if (distanceFromListener >= dist)
	{
		gain = 0.0f;  // Beta: SourceLWJGLOpenAL.java:431
	}
	else
	{
		gain = 1.0f - distanceFromListener / dist;  // Beta: SourceLWJGLOpenAL.java:433
	}
	if (gain > 1.0f)
		gain = 1.0f;  // Beta: SourceLWJGLOpenAL.java:437
	if (gain < 0.0f)
		gain = 0.0f;  // Beta: SourceLWJGLOpenAL.java:441
	
	// Beta: AL_GAIN = gain * sourceVolume (SourceLWJGLOpenAL.java:203)
	alSourcef(streamingSource, AL_GAIN, gain * sourceVolume);
	alSourcei(streamingSource, AL_LOOPING, AL_FALSE);  // toLoop = false
	
	// Java: soundSystem.play(id); (line 146)
	alGetError();  // Clear errors
	alSourcePlay(streamingSource);
	
	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		std::cerr << "OpenAL error playing streaming sound: " << alGetString(err) << std::endl;
	}
}

// Beta: play() - plays 3D sound (SoundEngine.java:152-173)
// Beta: soundSystem.newSource(volume > 1.0F, id, sound.url, sound.name, false, x, y, z, 2, dist);
// Beta: attenuation model 2 = AL_INVERSE_DISTANCE (matching Paulscode)
void SoundEngine::play(const jstring &name, float x, float y, float z, float volume, float pitch)
{
	if (!loaded || !options || options->sound == 0.0f)
		return;

	Sound *sound = sounds.get(name);
	if (sound == nullptr || volume <= 0.0f)  // Beta: volume > 0.0F check (line 155)
		return;

	idCounter = (idCounter + 1) % 256;  // Beta: this.idCounter = (this.idCounter + 1) % 256 (line 156)
	std::string id = "sound_" + std::to_string(idCounter);  // Beta: String id = "sound_" + this.idCounter (line 157)

	float dist = 16.0f;  // Beta: float dist = 16.0F (line 158)
	if (volume > 1.0f)  // Beta: if (volume > 1.0F) (line 159)
		dist *= volume;  // Beta: dist *= volume (line 160)

	ALuint source = getOrCreateSource(id, false);
	if (source == 0)
		return;

	ALuint buffer;
	if (!loadSound(*sound, buffer))
		return;

	// Beta: soundSystem.newSource(volume > 1.0F, id, sound.url, sound.name, false, x, y, z, 2, dist);
	// Beta: Reset source before use (Paulscode does this to ensure clean state)
	alSourceStop(source);
	alSourcei(source, AL_BUFFER, 0);
	alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	
	// Beta: Set source properties matching Paulscode newSource() behavior (SourceLWJGLOpenAL.java:295-303)
	// Beta: soundSystem.newSource(volume > 1.0F, id, sound.url, sound.name, false, x, y, z, 2, dist);
	// Beta: Attenuation model 2: Paulscode disables OpenAL distance attenuation and calculates gain manually!
	alSourcei(source, AL_BUFFER, buffer);
	
	// Beta: Set 3D position (SourceLWJGLOpenAL.java:295)
	alSource3f(source, AL_POSITION, x, y, z);
	alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);  // Beta: SourceLWJGLOpenAL.java:297
	alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);  // Beta: World-relative (not listener-relative)
	
	// Beta: Attenuation model 2 (SourceLWJGLOpenAL.java:299-303, 236-246)
	// Beta: For attModel != 1, Paulscode sets AL_ROLLOFF_FACTOR = 0.0F (disables OpenAL distance attenuation)
	int attModel = 2;  // Beta: Attenuation model 2 (linear distance attenuation)
	alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);  // Beta: Disable OpenAL distance attenuation (SourceLWJGLOpenAL.java:242, 256)
	
	// Beta: soundSystem.setPitch(id, pitch) - MUST be called after newSource, before setVolume (SoundEngine.java:164)
	// Beta: Paulscode clamps pitch between 0.5F and 2.0F (Library.java:401-416)
	float clampedPitch = pitch;
	if (clampedPitch < 0.5f)
		clampedPitch = 0.5f;
	else if (clampedPitch > 2.0f)
		clampedPitch = 2.0f;
	alSourcef(source, AL_PITCH, clampedPitch);  // Beta: soundSystem.setPitch(id, pitch) (SoundEngine.java:164)
	
	// Beta: Clamp volume if > 1.0F (SoundEngine.java:165-167) - MUST happen after setPitch
	float finalVolume = volume > 1.0f ? 1.0f : volume;
	float sourceVolume = finalVolume * options->sound;  // Beta: sourceVolume (SourceLWJGLOpenAL.java:sourceVolume)
	sourceInfoMap[id] = SourceInfo(x, y, z, dist, sourceVolume, attModel);
	
	// Beta: Calculate initial gain based on current listener position (SourceLWJGLOpenAL.java:calculateGain, 426-446)
	// Beta: gain = 1.0F - distanceFromListener / distOrRoll (for attModel == 2)
	// Beta: Read listener position from OpenAL (matching Paulscode's listenerPosition FloatBuffer)
	float currentListenerX, currentListenerY, currentListenerZ;
	alGetListener3f(AL_POSITION, &currentListenerX, &currentListenerY, &currentListenerZ);
	float dX = x - currentListenerX;
	float dY = y - currentListenerY;
	float dZ = z - currentListenerZ;
	float distanceFromListener = std::sqrt(dX * dX + dY * dY + dZ * dZ);
	float gain = 1.0f;  // Beta: Default gain (SourceLWJGLOpenAL.java:444)
	if (attModel == 2)
	{
		if (distanceFromListener <= 0.0f)
		{
			gain = 1.0f;  // Beta: SourceLWJGLOpenAL.java:429
		}
		else if (distanceFromListener >= dist)
		{
			gain = 0.0f;  // Beta: SourceLWJGLOpenAL.java:431
		}
		else
		{
			gain = 1.0f - distanceFromListener / dist;  // Beta: SourceLWJGLOpenAL.java:433
		}
		if (gain > 1.0f)
			gain = 1.0f;  // Beta: SourceLWJGLOpenAL.java:437
		if (gain < 0.0f)
			gain = 0.0f;  // Beta: SourceLWJGLOpenAL.java:441
	}
	
	// Beta: AL_GAIN = gain * sourceVolume (SourceLWJGLOpenAL.java:203, no fadeOut/fadeIn for simple sounds)
	alSourcef(source, AL_GAIN, gain * sourceVolume);
	alSourcei(source, AL_LOOPING, AL_FALSE);
	
	// Beta: Clear errors before playing
	alGetError();
	alSourcePlay(source);  // Beta: soundSystem.play(id) (line 170)
	
	// Beta: Check for errors after playing
	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		std::cerr << "OpenAL error playing sound: " << alGetString(err) << std::endl;
	}
}

// Beta: playUI() - plays UI sound (SoundEngine.java:175-192)
void SoundEngine::playUI(const jstring &name, float volume, float pitch)
{
	if (!loaded || !options || options->sound == 0.0f)
		return;

	Sound *sound = sounds.get(name);
	if (sound == nullptr)
		return;

	idCounter = (idCounter + 1) % 256;
	std::string id = "sound_" + std::to_string(idCounter);

	if (volume > 1.0f)
		volume = 1.0f;
	volume *= 0.25f;  // Beta: UI sounds are quieter (SoundEngine.java:186)

	ALuint source = getOrCreateSource(id, false);
	if (source == 0)
		return;

	ALuint buffer;
	if (!loadSound(*sound, buffer))
		return;

	// Beta: UI sounds are non-positional (SoundEngine.java:181, matching Paulscode)
	// Beta: Reset source before use
	alSourceStop(source);
	alSourcei(source, AL_BUFFER, 0);
	alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	
	// Beta: UI sounds (attModel = 0) - matching Beta 1.2 SoundEngine.java:181 (newSource with attModel=0, distOrRoll=0.0F)
	// Beta: Paulscode sets AL_ROLLOFF_FACTOR = 0.0F for attModel != 1 (SourceLWJGLOpenAL.java:242, 302)
	alSourcei(source, AL_BUFFER, buffer);
	alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);  // Beta: Position at origin (SoundEngine.java:181)
	alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);  // Beta: No velocity (matching Paulscode)
	alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);  // Beta: No rolloff for attModel = 0 (SourceLWJGLOpenAL.java:242, 302)
	alSourcef(source, AL_PITCH, pitch);  // Beta: soundSystem.setPitch(id, pitch) (SoundEngine.java:187)
	alSourcef(source, AL_GAIN, volume * options->sound);  // Beta: soundSystem.setVolume(id, volume * options.sound) (SoundEngine.java:188)
	alSourcei(source, AL_LOOPING, AL_FALSE);  // Beta: Not looping (SoundEngine.java:181 - toLoop=false)
	
	// Beta: Clear errors before playing
	alGetError();
	alSourcePlay(source);
	
	// Beta: Check for errors after playing
	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		std::cerr << "OpenAL error playing UI sound: " << alGetString(err) << std::endl;
	}
}

ALuint SoundEngine::getOrCreateSource(const std::string &id, bool streaming)
{
	if (streaming)
		return streamingSource;

	// Beta: First check for finished sources and release them
	checkAndReleaseFinishedSources();

	// Try to get from pool
	if (!freeSources.empty())
	{
		ALuint source = freeSources.back();
		freeSources.pop_back();
		
		// Beta: Reset source before reuse (Paulscode does this)
		alSourceStop(source);
		alSourcei(source, AL_BUFFER, 0);
		alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSourcef(source, AL_GAIN, 1.0f);
		alSourcef(source, AL_PITCH, 1.0f);
		alSourcef(source, AL_REFERENCE_DISTANCE, 1.0f);
		alSourcei(source, AL_LOOPING, AL_FALSE);
		
		activeSources[id] = source;
		return source;
	}

	// Beta: No free sources - steal the oldest one (Paulscode behavior)
	// This prevents sounds from completely stopping when pool is exhausted
	if (!activeSources.empty())
	{
		auto oldest = activeSources.begin();
		ALuint source = oldest->second;
		
		// Stop and reset the stolen source
		alSourceStop(source);
		alSourcei(source, AL_BUFFER, 0);
		alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSourcef(source, AL_GAIN, 1.0f);
		alSourcef(source, AL_PITCH, 1.0f);
		alSourcef(source, AL_REFERENCE_DISTANCE, 1.0f);
		alSourcei(source, AL_LOOPING, AL_FALSE);
		
		// Remove old mapping and create new one
		activeSources.erase(oldest);
		activeSources[id] = source;
		return source;
	}

	// Should never happen, but return 0 if somehow no sources exist
	return 0;
}

void SoundEngine::releaseSource(const std::string &id)
{
	auto it = activeSources.find(id);
	if (it != activeSources.end())
	{
		ALuint source = it->second;
		alSourceStop(source);
		alSourcei(source, AL_BUFFER, 0);
		freeSources.push_back(source);
		activeSources.erase(it);
	}
}

// Load OGG file using stb_vorbis and create OpenAL buffer
// For MUS files (streaming sounds), decrypt them first using XOR decryption
ALuint SoundEngine::loadOGGFile(const std::string &filePath, bool isMUS)
{
	// Check cache first (use filePath + isMUS flag as key for MUS files)
	std::string cacheKey = filePath + (isMUS ? "_mus" : "");
	auto it = soundBuffers.find(cacheKey);
	if (it != soundBuffers.end())
		return it->second;

	// Read file into memory
	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		std::cerr << "Failed to open sound file: " << filePath << std::endl;
		return 0;
	}

	size_t fileSize = (size_t)file.tellg();
	if (fileSize == 0 || fileSize > 100 * 1024 * 1024)  // Max 100MB
	{
		std::cerr << "Invalid file size: " << filePath << std::endl;
		file.close();
		return 0;
	}

	file.seekg(0, std::ios::beg);
	std::vector<unsigned char> fileData(fileSize);
	if (!file.read((char*)fileData.data(), fileSize))
	{
		std::cerr << "Failed to read sound file: " << filePath << std::endl;
		file.close();
		return 0;
	}
	file.close();
	
	// Beta: Decrypt MUS files (XOR-encrypted OGG) before decoding
	// Use simplemusdec.h logic for in-memory decryption
	if (isMUS)
	{
		// Extract filename from path for hashCode calculation
		size_t lastSlash = filePath.find_last_of("/\\");
		std::string filename = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
		
		// Calculate seed from filename hashCode (simplemusdec.h: mus_java_string_hashcode)
		int32_t seed = 0;
		for (size_t i = 0; i < filename.length(); i++)
		{
			seed = static_cast<int32_t>(static_cast<uint32_t>(seed) * 31u + static_cast<uint32_t>(static_cast<unsigned char>(filename[i])));
		}
		
		// Decrypt using XOR (simplemusdec.h: mus_decode_file logic)
		for (size_t i = 0; i < fileData.size(); i++)
		{
			uint8_t enc = fileData[i];
			
			// Reverse: enc = val ^ (seed >> 8)
			uint8_t key = static_cast<uint8_t>(static_cast<uint32_t>(seed) >> 8);
			uint8_t val = static_cast<uint8_t>(enc ^ key);
			
			fileData[i] = val;
			
			// Update seed: seed = seed * 498729871 + 85731 * (int8_t)val;
			seed = static_cast<int32_t>(
				static_cast<uint32_t>(seed) * 498729871u +
				static_cast<uint32_t>(85731 * static_cast<int32_t>(static_cast<int8_t>(val)))
			);
		}
	}

	// Beta: Decode OGG using stb_vorbis_decode_memory (stb_vorbis.c:5386)
	// This function decodes the entire file into memory
	int channels, sampleRate;
	short *output = nullptr;
	int samples = stb_vorbis_decode_memory((const uint8*)fileData.data(), (int)fileSize, &channels, &sampleRate, &output);
	
	if (samples <= 0 || output == nullptr || channels <= 0 || channels > 2)
	{
		std::cerr << "Failed to decode OGG file: " << filePath << " (samples: " << samples << ", channels: " << (output ? channels : -1) << ")" << std::endl;
		if (output)
			free(output);
		return 0;
	}

	// Beta: Determine OpenAL format (LibraryLWJGLOpenAL.java:179-206)
	// Beta: Paulscode supports both mono and stereo - NO conversion to mono!
	// Beta: 4352 = AL_FORMAT_MONO8, 4353 = AL_FORMAT_MONO16
	// Beta: 4354 = AL_FORMAT_STEREO8, 4355 = AL_FORMAT_STEREO16
	ALenum format;
	if (channels == 1)
	{
		format = AL_FORMAT_MONO16;  // Beta: 4353
	}
	else if (channels == 2)
	{
		format = AL_FORMAT_STEREO16;  // Beta: 4355 - keep stereo!
	}
	else
	{
		std::cerr << "Unsupported channel count: " << channels << " in " << filePath << std::endl;
		free(output);
		return 0;
	}

	// Beta: Create OpenAL buffer (LibraryLWJGLOpenAL.java:208-217)
	ALuint buffer = 0;
	alGenBuffers(1, &buffer);
	if (buffer == 0)
	{
		ALenum err = alGetError();
		std::cerr << "OpenAL error generating buffer: " << alGetString(err) << std::endl;
		free(output);
		return 0;
	}

	// Beta: Buffer audio data (LibraryLWJGLOpenAL.java:217)
	// Beta: Keep original format (mono or stereo) - no conversion
	int numBytes = samples * channels * sizeof(short);
	alBufferData(buffer, format, output, numBytes, sampleRate);
	
	free(output);

	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		std::cerr << "OpenAL error creating buffer: " << alGetString(err) << std::endl;
		alDeleteBuffers(1, &buffer);
		return 0;
	}

	// Cache buffer using cacheKey
	soundBuffers[cacheKey] = buffer;
	return buffer;
}

bool SoundEngine::loadSound(const Sound &sound, ALuint &buffer, bool isMUS)
{
	buffer = loadOGGFile(sound.filePath, isMUS);
	return buffer != 0;
}