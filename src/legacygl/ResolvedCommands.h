#pragma once

#include "legacygl/Geometry.h"
#include "legacygl/Matrix.h"
#include "legacygl/Primitive.h"

// Loader-neutral commands emitted after the semantic core has resolved legacy
// state. Pointer payloads are valid only for the duration of the Sink callback;
// a backend that defers work must copy them before returning.

namespace legacygl
{

struct LightState
{
	bool enabled = false;
	float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float diffuse[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	// Eye space: glLight* transformed it by the model-view matrix current at
	// the time of the call.
	float positionEye[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
	float spotDirectionEye[3] = { 0.0f, 0.0f, -1.0f };
	float spotExponent = 0.0f;
	float spotCutoff = 180.0f;
	float constantAttenuation = 1.0f;
	float linearAttenuation = 0.0f;
	float quadraticAttenuation = 0.0f;
};

struct MaterialState
{
	float ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	float diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float emission[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 0.0f;
};

struct ResolvedEnableState
{
	bool texture2D = false;
	bool depthTest = false;
	bool alphaTest = false;
	bool blend = false;
	bool cullFace = false;
	bool fog = false;
	bool lighting = false;
	bool colorMaterial = false;
	bool rescaleNormal = false;
	bool normalize = false;
	bool colorLogicOp = false;
	bool polygonOffsetFill = false;
	bool scissorTest = false;
	bool stencilTest = false;
	bool lineSmooth = false;
	bool dither = true;
};

struct ResolvedPipelineState
{
	unsigned int blendSource = 0;
	unsigned int blendDestination = 0;
	unsigned int alphaFunction = 0;
	float alphaReference = 0.0f;
	unsigned int depthFunction = 0;
	bool depthWrite = true;
	bool colorWrite[4] = { true, true, true, true };
	unsigned int cullFaceMode = 0;
	unsigned int frontFaceMode = 0;
	unsigned int shadeModel = 0;
	unsigned int logicOpcode = 0;
	float lineWidth = 1.0f;
	float polygonOffsetFactor = 0.0f;
	float polygonOffsetUnits = 0.0f;
	int viewport[4] = { 0, 0, 0, 0 };
};

struct ResolvedFogState
{
	unsigned int mode = 0;
	float density = 1.0f;
	float start = 0.0f;
	float end = 1.0f;
	float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	unsigned int distanceMode = 0;
};

struct ResolvedLightingState
{
	static const int MAX_LIGHTS = 8;

	LightState lights[MAX_LIGHTS];
	float modelAmbient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	unsigned int colorMaterialFace = 0;
	unsigned int colorMaterialMode = 0;
	MaterialState frontMaterial;
	MaterialState backMaterial;
};

struct ResolvedTextureState
{
	unsigned int name = 0;
	unsigned int minFilter = 0;
	unsigned int magFilter = 0;
	unsigned int wrapS = 0;
	unsigned int wrapT = 0;
	float borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	int level0Width = 0;
	int level0Height = 0;
	int level0InternalFormat = 0;
	bool level0Defined = false;
	bool complete = false;
};

struct ResolvedDraw
{
	long long sequence = 0;
	const Geometry *geometry = nullptr;
	const PrimitiveBatch *primitives = nullptr;
	Mat4 modelView;
	Mat4 projection;
	Mat4 textureMatrix;
	Mat4 normal;
	float normalRescaleFactor = 1.0f;
	ResolvedEnableState enables;
	ResolvedPipelineState pipeline;
	ResolvedFogState fog;
	ResolvedLightingState lighting;
	ResolvedTextureState texture;
};

struct ResolvedClear
{
	long long sequence = 0;
	unsigned int mask = 0;
	float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	double depth = 1.0;
	bool colorWrite[4] = { true, true, true, true };
	bool depthWrite = true;
	bool scissorTest = false;
	bool dither = true;
};

struct ResolvedTextureUpload
{
	long long sequence = 0;
	unsigned int texture = 0;
	bool subImage = false;
	int level = 0;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	int internalFormat = 0;
	unsigned int sourceFormat = 0;
	unsigned int sourceType = 0;
	int unpackAlignment = 4;
	const void *pixels = nullptr;
};

struct ResolvedReadback
{
	long long sequence = 0;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	unsigned int format = 0;
	unsigned int type = 0;
	int packAlignment = 4;
	void *pixels = nullptr;
};

}
