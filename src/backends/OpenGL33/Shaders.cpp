#include "backends/OpenGL33/Shaders.h"

namespace legacygl
{

// GLSL 3.30 is the floor this shader actually needs. Explicit vertex-input and
// fragment-output locations are core in 3.30; the block/sampler `binding`
// qualifier (4.20) and inter-stage varying locations (4.40) are not, so the
// binding is assigned from the API and the varyings match by name.
static const char *CORE_GL_VERTEX_SHADER = R"GLSL(#version 330 core

const uint NORMAL_NONE = 0u;
const uint NORMAL_RESCALE = 1u;
const uint NORMAL_NORMALIZE = 2u;

const uint CM_FRONT = 1u;
const uint CM_BACK = 2u;
const uint CM_FRONT_AND_BACK = 3u;

const uint CM_AMBIENT = 0u;
const uint CM_DIFFUSE = 1u;
const uint CM_AMBIENT_AND_DIFFUSE = 2u;
const uint CM_SPECULAR = 3u;
const uint CM_EMISSION = 4u;

struct MaterialState
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 emission;
	vec4 shininess;
};

struct LightState
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 positionEye;
	vec4 spotDirectionCutoff;
	vec4 attenuationExponent;
};

layout(std140) uniform LegacyFFPBlock
{
	mat4 uModelView;
	mat4 uProjection;
	mat4 uTexture;
	mat4 uNormalMatrix;
	vec4 uGlobalAmbient;
	MaterialState uFrontMaterial;
	MaterialState uBackMaterial;
	LightState uLights[8];
	vec4 uFogColor;
	vec4 uFogParams;
	vec4 uTextureSize;
	vec4 uNormalParams;
	uvec4 uFlags0;
	uvec4 uFlags1;
	uvec4 uFlags2;
	uvec4 uFlags3;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inFlatPosition;
layout(location = 5) in vec4 inFlatColor;
layout(location = 6) in vec3 inFlatNormal;

out vec3 vTexCoord;
out vec4 vSmoothPrimary;
flat out vec4 vFlatPrimary;
out float vFogCoord;

void applyColorMaterial(inout MaterialState material, vec4 color, uint mode)
{
	if (mode == CM_AMBIENT)
		material.ambient = color;
	else if (mode == CM_DIFFUSE)
		material.diffuse = color;
	else if (mode == CM_AMBIENT_AND_DIFFUSE)
	{
		material.ambient = color;
		material.diffuse = color;
	}
	else if (mode == CM_SPECULAR)
		material.specular = color;
	else if (mode == CM_EMISSION)
		material.emission = color;
}

vec4 computeLighting(MaterialState material, vec3 normalEye, vec3 eyePosition)
{
	vec3 rgb = material.emission.rgb + uGlobalAmbient.rgb * material.ambient.rgb;
	float alpha = material.diffuse.a;
	uint lightMask = uFlags1.y;

	for (uint i = 0u; i < 8u; ++i)
	{
		if ((lightMask & (1u << i)) == 0u)
			continue;

		LightState light = uLights[i];
		vec3 lightDirection;
		float attenuation = 1.0;

		if (light.positionEye.w == 0.0)
		{
			lightDirection = normalize(light.positionEye.xyz);
		}
		else
		{
			vec3 lightPosition = light.positionEye.xyz / light.positionEye.w;
			vec3 toLight = lightPosition - eyePosition;
			float lightDistance = length(toLight);
			lightDirection = lightDistance > 0.0 ? toLight / lightDistance : vec3(0.0);

			vec3 attenuationTerms = light.attenuationExponent.xyz;
			float denominator = attenuationTerms.x + attenuationTerms.y * lightDistance +
				attenuationTerms.z * lightDistance * lightDistance;
			attenuation = denominator > 0.0 ? 1.0 / denominator : 1.0;

			float cosineCutoff = light.spotDirectionCutoff.w;
			if (cosineCutoff > -1.0)
			{
				vec3 spotDirection = normalize(light.spotDirectionCutoff.xyz);
				float spotCosine = dot(normalize(-lightDirection), spotDirection);
				if (spotCosine < cosineCutoff)
					attenuation = 0.0;
				else
					attenuation *= pow(max(spotCosine, 0.0), light.attenuationExponent.w);
			}
		}

		vec3 contribution = light.ambient.rgb * material.ambient.rgb;
		float normalDotLight = max(dot(normalEye, lightDirection), 0.0);
		if (normalDotLight > 0.0)
		{
			contribution += light.diffuse.rgb * material.diffuse.rgb * normalDotLight;
			float shininess = clamp(material.shininess.x, 0.0, 128.0);
			vec3 halfVector = normalize(lightDirection + vec3(0.0, 0.0, 1.0));
			float normalDotHalf = max(dot(normalEye, halfVector), 0.0);
			float specularFactor = shininess == 0.0 ? 1.0 : pow(normalDotHalf, shininess);
			contribution += light.specular.rgb * material.specular.rgb * specularFactor;
		}

		rgb += attenuation * contribution;
	}

	return clamp(vec4(rgb, alpha), 0.0, 1.0);
}

vec3 transformNormal(vec3 objectNormal)
{
	vec3 normalEye = (uNormalMatrix * vec4(objectNormal, 0.0)).xyz;
	if (uFlags1.x == NORMAL_RESCALE)
		normalEye *= uNormalParams.x;
	else if (uFlags1.x == NORMAL_NORMALIZE)
		normalEye = normalize(normalEye);
	return normalEye;
}

vec4 computePrimary(vec3 objectPosition, vec4 color, vec3 objectNormal)
{
	if (uFlags0.x == 0u)
		return clamp(color, 0.0, 1.0);

	MaterialState material = uFrontMaterial;
	if (uFlags0.z != 0u &&
		(uFlags3.x == CM_FRONT || uFlags3.x == CM_FRONT_AND_BACK))
	{
		applyColorMaterial(material, color, uFlags3.y);
	}

	vec3 eyePosition = (uModelView * vec4(objectPosition, 1.0)).xyz;
	return computeLighting(material, transformNormal(objectNormal), eyePosition);
}

void main()
{
	vec4 eyePosition = uModelView * vec4(inPosition, 1.0);
	gl_Position = uProjection * eyePosition;

	vec4 transformedTexCoord = uTexture * vec4(inTexCoord, 0.0, 1.0);
	vTexCoord = transformedTexCoord.xyw;
	vSmoothPrimary = computePrimary(inPosition, inColor, inNormal);
	vFlatPrimary = computePrimary(inFlatPosition, inFlatColor, inFlatNormal);
	vFogCoord = uFlags1.w == 2u ? length(eyePosition.xyz) :
		(uFlags1.w == 1u ? eyePosition.z : abs(eyePosition.z));
}
)GLSL";

static const char *CORE_GL_FRAGMENT_SHADER = R"GLSL(#version 330 core

const uint FOG_OFF = 0u;
const uint FOG_LINEAR = 1u;
const uint FOG_EXP = 2u;
const uint FOG_EXP2 = 3u;

const uint ALPHA_NEVER = 0u;
const uint ALPHA_LESS = 1u;
const uint ALPHA_EQUAL = 2u;
const uint ALPHA_LEQUAL = 3u;
const uint ALPHA_GREATER = 4u;
const uint ALPHA_NOTEQUAL = 5u;
const uint ALPHA_GEQUAL = 6u;
const uint ALPHA_ALWAYS = 7u;

const uint WRAP_REPEAT = 0u;
const uint WRAP_CLAMP = 1u;
const uint WRAP_CLAMP_TO_EDGE = 2u;

struct MaterialState
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 emission;
	vec4 shininess;
};

struct LightState
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 positionEye;
	vec4 spotDirectionCutoff;
	vec4 attenuationExponent;
};

layout(std140) uniform LegacyFFPBlock
{
	mat4 uModelView;
	mat4 uProjection;
	mat4 uTexture;
	mat4 uNormalMatrix;
	vec4 uGlobalAmbient;
	MaterialState uFrontMaterial;
	MaterialState uBackMaterial;
	LightState uLights[8];
	vec4 uFogColor;
	vec4 uFogParams;
	vec4 uTextureSize;
	vec4 uNormalParams;
	uvec4 uFlags0;
	uvec4 uFlags1;
	uvec4 uFlags2;
	uvec4 uFlags3;
};

uniform sampler2D uTextureSampler;

in vec3 vTexCoord;
in vec4 vSmoothPrimary;
flat in vec4 vFlatPrimary;
in float vFogCoord;

layout(location = 0) out vec4 outColor;

float mapLegacyCoordinate(float coordinate, float sourceSize, float derivedSize, uint wrapMode)
{
	float reduced = wrapMode == WRAP_REPEAT
		? coordinate - floor(coordinate)
		: clamp(coordinate, 0.0, 1.0);
	float mapped = (reduced * sourceSize + 1.0) / derivedSize;
	if (wrapMode == WRAP_CLAMP && reduced == 1.0)
		mapped = uintBitsToFloat(floatBitsToUint(mapped) - 1u);
	return mapped;
}

vec4 sampleLegacyTexture(vec2 coordinate)
{
	if (uFlags3.w == 0u)
		return texture(uTextureSampler, coordinate);

	vec2 mapped = vec2(
		mapLegacyCoordinate(coordinate.x, uTextureSize.x, uTextureSize.z, uFlags2.z),
		mapLegacyCoordinate(coordinate.y, uTextureSize.y, uTextureSize.w, uFlags2.w));
	return texture(uTextureSampler, mapped);
}

bool alphaTestPass(float alpha, float reference, uint functionCode)
{
	if (functionCode == ALPHA_NEVER) return false;
	if (functionCode == ALPHA_LESS) return alpha < reference;
	if (functionCode == ALPHA_EQUAL) return alpha == reference;
	if (functionCode == ALPHA_LEQUAL) return alpha <= reference;
	if (functionCode == ALPHA_GREATER) return alpha > reference;
	if (functionCode == ALPHA_NOTEQUAL) return alpha != reference;
	if (functionCode == ALPHA_GEQUAL) return alpha >= reference;
	return true;
}

float fogFactor(float coordinate)
{
	if (uFlags1.z == FOG_LINEAR)
	{
		float denominator = uFogParams.y - uFogParams.x;
		return denominator != 0.0
			? clamp((uFogParams.y - coordinate) / denominator, 0.0, 1.0)
			: (coordinate < uFogParams.y ? 1.0 : 0.0);
	}
	if (uFlags1.z == FOG_EXP)
		return clamp(exp(-uFogParams.z * coordinate), 0.0, 1.0);
	if (uFlags1.z == FOG_EXP2)
	{
		float densityCoordinate = uFogParams.z * coordinate;
		return clamp(exp(-(densityCoordinate * densityCoordinate)), 0.0, 1.0);
	}
	return 1.0;
}

void main()
{
	vec4 color = uFlags0.w != 0u ? vFlatPrimary : vSmoothPrimary;
	if (uFlags0.y != 0u && uFlags3.z != 0u)
	{
		vec2 coordinate = vTexCoord.xy / vTexCoord.z;
		color *= sampleLegacyTexture(coordinate);
	}

	color = clamp(color, 0.0, 1.0);
	if (uFlags2.x != 0u && !alphaTestPass(color.a, uFogParams.w, uFlags2.y))
		discard;

	if (uFlags1.z != FOG_OFF)
	{
		float factor = fogFactor(vFogCoord);
		color.rgb = mix(uFogColor.rgb, color.rgb, factor);
	}

	outColor = clamp(color, 0.0, 1.0);
}
)GLSL";

const char *coreGLVertexShaderSource()
{
	return CORE_GL_VERTEX_SHADER;
}

const char *coreGLFragmentShaderSource()
{
	return CORE_GL_FRAGMENT_SHADER;
}

}
