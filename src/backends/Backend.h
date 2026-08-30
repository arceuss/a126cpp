#pragma once

namespace legacygl
{

class Sink;

}

namespace renderbackend
{

enum class OpenGLProfile
{
	None,
	Compatibility,
	Core
};

struct Configuration
{
	const char *recordName;
	int requestedGLMajorVersion;
	int requestedGLMinorVersion;
	OpenGLProfile requestedGLProfile;
	int requiredGLMajorVersion;
	int requiredGLMinorVersion;
	OpenGLProfile requiredGLProfile;
	bool virtualNVFogDistance;
	bool supportsQueryValidation;
};

// Exactly one backend implementation provides these symbols in a process.
const Configuration &configuration();
void initialize();
void present();
void shutdown();
bool hasCapability(const char *capability);
legacygl::Sink *sink();

}
