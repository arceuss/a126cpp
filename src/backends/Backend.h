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

struct Backend
{
	const char *cliName;
	const Configuration &(*configuration)();
	void (*initialize)();
	void (*present)();
	void (*shutdown)();
	bool (*hasCapability)(const char *capability);
	legacygl::Sink *(*sink)();
};

const Backend &nativeGLBackend();
const Backend &openGL21Backend();
const Backend &openGL46Backend();
const Backend &vulkanBackend();
const Backend &d3d12Backend();

bool select(const char *cliName);
const Configuration &configuration();
void initialize();
void present();
void shutdown();
bool hasCapability(const char *capability);
legacygl::Sink *sink();

}
