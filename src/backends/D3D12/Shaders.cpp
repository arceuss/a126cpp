#include "backends/D3D12/Shaders.h"

namespace d3d12backend
{

static const char *LEGACY_VERTEX_SHADER = R"HLSL(
static const uint NORMAL_NONE = 0u;
static const uint NORMAL_RESCALE = 1u;
static const uint NORMAL_NORMALIZE = 2u;

static const uint CM_FRONT = 1u;
static const uint CM_BACK = 2u;
static const uint CM_FRONT_AND_BACK = 3u;

static const uint CM_AMBIENT = 0u;
static const uint CM_DIFFUSE = 1u;
static const uint CM_AMBIENT_AND_DIFFUSE = 2u;
static const uint CM_SPECULAR = 3u;
static const uint CM_EMISSION = 4u;

struct MaterialState
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 emission;
	float4 shininess;
};

struct LightState
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 positionEye;
	float4 spotDirectionCutoff;
	float4 attenuationExponent;
};

cbuffer LegacyFFPBlock : register(b0)
{
	column_major float4x4 uModelView;
	column_major float4x4 uProjection;
	column_major float4x4 uTexture;
	column_major float4x4 uNormalMatrix;
	float4 uGlobalAmbient;
	MaterialState uFrontMaterial;
	MaterialState uBackMaterial;
	LightState uLights[8];
	float4 uFogColor;
	float4 uFogParams;
	float4 uTextureSize;
	float4 uNormalParams;
	uint4 uFlags0;
	uint4 uFlags1;
	uint4 uFlags2;
	uint4 uFlags3;
	float4 uCurrentColor;
	float4 uCurrentNormal;
	float4 uCurrentTexCoord;
	uint4 uFlags4;
};

struct VertexInput
{
	float3 position : POSITION0;
	float4 color : COLOR0;
	float3 normal : NORMAL0;
	float2 texCoord : TEXCOORD0;
	float3 flatPosition : POSITION1;
	float4 flatColor : COLOR1;
	float3 flatNormal : NORMAL1;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float3 texCoord : TEXCOORD0;
	float4 smoothPrimary : COLOR0;
	nointerpolation float4 flatPrimary : COLOR1;
	float fogCoord : TEXCOORD1;
};

void applyColorMaterial(inout MaterialState material, float4 color, uint mode)
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

float4 computeLighting(MaterialState material, float3 normalEye, float3 eyePosition)
{
	float3 rgb = material.emission.rgb + uGlobalAmbient.rgb * material.ambient.rgb;
	float alpha = material.diffuse.a;
	uint lightMask = uFlags1.y;

	[unroll]
	for (uint i = 0u; i < 8u; ++i)
	{
		if ((lightMask & (1u << i)) == 0u)
			continue;

		LightState light = uLights[i];
		float3 lightDirection;
		float attenuation = 1.0;

		if (light.positionEye.w == 0.0)
		{
			lightDirection = normalize(light.positionEye.xyz);
		}
		else
		{
			float3 lightPosition = light.positionEye.xyz / light.positionEye.w;
			float3 toLight = lightPosition - eyePosition;
			float lightDistance = length(toLight);
			lightDirection = lightDistance > 0.0 ? toLight / lightDistance : float3(0.0, 0.0, 0.0);

			float3 attenuationTerms = light.attenuationExponent.xyz;
			float denominator = attenuationTerms.x + attenuationTerms.y * lightDistance +
				attenuationTerms.z * lightDistance * lightDistance;
			attenuation = denominator > 0.0 ? 1.0 / denominator : 1.0;

			float cosineCutoff = light.spotDirectionCutoff.w;
			if (cosineCutoff > -1.0)
			{
				float3 spotDirection = normalize(light.spotDirectionCutoff.xyz);
				float spotCosine = dot(normalize(-lightDirection), spotDirection);
				if (spotCosine < cosineCutoff)
					attenuation = 0.0;
				else
					attenuation *= pow(max(spotCosine, 0.0), light.attenuationExponent.w);
			}
		}

		float3 contribution = light.ambient.rgb * material.ambient.rgb;
		float normalDotLight = max(dot(normalEye, lightDirection), 0.0);
		if (normalDotLight > 0.0)
		{
			contribution += light.diffuse.rgb * material.diffuse.rgb * normalDotLight;
			float shininess = clamp(material.shininess.x, 0.0, 128.0);
			float3 halfVector = normalize(lightDirection + float3(0.0, 0.0, 1.0));
			float normalDotHalf = max(dot(normalEye, halfVector), 0.0);
			float specularFactor = shininess == 0.0 ? 1.0 : pow(normalDotHalf, shininess);
			contribution += light.specular.rgb * material.specular.rgb * specularFactor;
		}

		rgb += attenuation * contribution;
	}

	return saturate(float4(rgb, alpha));
}

float3 transformNormal(float3 objectNormal)
{
	float3 normalEye = mul(uNormalMatrix, float4(objectNormal, 0.0)).xyz;
	if (uFlags1.x == NORMAL_RESCALE)
		normalEye *= uNormalParams.x;
	else if (uFlags1.x == NORMAL_NORMALIZE)
		normalEye = normalize(normalEye);
	return normalEye;
}

float4 computePrimary(float3 objectPosition, float4 color, float3 objectNormal)
{
	float4 primary = saturate(color);
	if (uFlags0.x != 0u)
	{
		MaterialState material = uFrontMaterial;
		if (uFlags0.z != 0u &&
			(uFlags3.x == CM_FRONT || uFlags3.x == CM_FRONT_AND_BACK))
		{
			applyColorMaterial(material, color, uFlags3.y);
		}

		float3 eyePosition = mul(uModelView, float4(objectPosition, 1.0)).xyz;
		primary = computeLighting(material, transformNormal(objectNormal), eyePosition);
	}
	return primary;
}

VertexOutput main(VertexInput input)
{
	VertexOutput output = (VertexOutput)0;
	float4 color = uFlags4.x != 0u ? input.color : uCurrentColor;
	float3 normal = uFlags4.y != 0u ? input.normal : uCurrentNormal.xyz;
	float2 texCoord = uFlags4.z != 0u ? input.texCoord : uCurrentTexCoord.xy;
	float4 flatColor = uFlags4.x != 0u ? input.flatColor : uCurrentColor;
	float3 flatNormal = uFlags4.y != 0u ? input.flatNormal : uCurrentNormal.xyz;
	float4 eyePosition = mul(uModelView, float4(input.position, 1.0));
	float4 glClip = mul(uProjection, eyePosition);
	output.position = float4(glClip.xy, 0.5 * (glClip.z + glClip.w), glClip.w);

	float4 transformedTexCoord = mul(uTexture, float4(texCoord, 0.0, 1.0));
	output.texCoord = transformedTexCoord.xyw;
	output.smoothPrimary = computePrimary(input.position, color, normal);
	output.flatPrimary = computePrimary(input.flatPosition, flatColor, flatNormal);
	output.fogCoord = uFlags1.w == 2u ? length(eyePosition.xyz) :
		(uFlags1.w == 1u ? eyePosition.z : abs(eyePosition.z));
	return output;
}
)HLSL";

static const char *LEGACY_PIXEL_SHADER = R"HLSL(
static const uint FOG_OFF = 0u;
static const uint FOG_LINEAR = 1u;
static const uint FOG_EXP = 2u;
static const uint FOG_EXP2 = 3u;

static const uint ALPHA_NEVER = 0u;
static const uint ALPHA_LESS = 1u;
static const uint ALPHA_EQUAL = 2u;
static const uint ALPHA_LEQUAL = 3u;
static const uint ALPHA_GREATER = 4u;
static const uint ALPHA_NOTEQUAL = 5u;
static const uint ALPHA_GEQUAL = 6u;
static const uint ALPHA_ALWAYS = 7u;

static const uint WRAP_REPEAT = 0u;
static const uint WRAP_CLAMP = 1u;
static const uint WRAP_CLAMP_TO_EDGE = 2u;

struct MaterialState
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 emission;
	float4 shininess;
};

struct LightState
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 positionEye;
	float4 spotDirectionCutoff;
	float4 attenuationExponent;
};

cbuffer LegacyFFPBlock : register(b0)
{
	column_major float4x4 uModelView;
	column_major float4x4 uProjection;
	column_major float4x4 uTexture;
	column_major float4x4 uNormalMatrix;
	float4 uGlobalAmbient;
	MaterialState uFrontMaterial;
	MaterialState uBackMaterial;
	LightState uLights[8];
	float4 uFogColor;
	float4 uFogParams;
	float4 uTextureSize;
	float4 uNormalParams;
	uint4 uFlags0;
	uint4 uFlags1;
	uint4 uFlags2;
	uint4 uFlags3;
	float4 uCurrentColor;
	float4 uCurrentNormal;
	float4 uCurrentTexCoord;
	uint4 uFlags4;
};

Texture2D<float4> uTextureSampler : register(t0);
SamplerState uSampler : register(s0);

struct PixelInput
{
	float4 position : SV_Position;
	float3 texCoord : TEXCOORD0;
	float4 smoothPrimary : COLOR0;
	nointerpolation float4 flatPrimary : COLOR1;
	float fogCoord : TEXCOORD1;
};

float mapLegacyCoordinate(float coordinate, float sourceSize, float derivedSize, uint wrapMode)
{
	float reduced = wrapMode == WRAP_REPEAT ? coordinate - floor(coordinate) : saturate(coordinate);
	float mapped = (reduced * sourceSize + 1.0) / derivedSize;
	if (wrapMode == WRAP_CLAMP && reduced == 1.0)
		mapped = asfloat(asuint(mapped) - 1u);
	return mapped;
}

float4 sampleLegacyTexture(float2 coordinate)
{
	float4 sampled = uTextureSampler.Sample(uSampler, coordinate);
	if (uFlags3.w != 0u)
	{
		float2 mapped = float2(
			mapLegacyCoordinate(coordinate.x, uTextureSize.x, uTextureSize.z, uFlags2.z),
			mapLegacyCoordinate(coordinate.y, uTextureSize.y, uTextureSize.w, uFlags2.w));
		sampled = uTextureSampler.Sample(uSampler, mapped);
	}
	return sampled;
}

bool alphaTestPass(float alpha, float reference, uint functionCode)
{
	bool passed = true;
	if (functionCode == ALPHA_NEVER) passed = false;
	else if (functionCode == ALPHA_LESS) passed = alpha < reference;
	else if (functionCode == ALPHA_EQUAL) passed = alpha == reference;
	else if (functionCode == ALPHA_LEQUAL) passed = alpha <= reference;
	else if (functionCode == ALPHA_GREATER) passed = alpha > reference;
	else if (functionCode == ALPHA_NOTEQUAL) passed = alpha != reference;
	else if (functionCode == ALPHA_GEQUAL) passed = alpha >= reference;
	return passed;
}

float fogFactor(float coordinate)
{
	float factor = 1.0;
	if (uFlags1.z == FOG_LINEAR)
	{
		float denominator = uFogParams.y - uFogParams.x;
		factor = denominator != 0.0 ? saturate((uFogParams.y - coordinate) / denominator) :
			(coordinate < uFogParams.y ? 1.0 : 0.0);
	}
	else if (uFlags1.z == FOG_EXP)
		factor = saturate(exp(-uFogParams.z * coordinate));
	else if (uFlags1.z == FOG_EXP2)
	{
		float densityCoordinate = uFogParams.z * coordinate;
		factor = saturate(exp(-(densityCoordinate * densityCoordinate)));
	}
	return factor;
}

float4 shade(PixelInput input)
{
	float4 color = uFlags0.w != 0u ? input.flatPrimary : input.smoothPrimary;
	if (uFlags0.y != 0u && uFlags3.z != 0u)
	{
		float2 coordinate = input.texCoord.xy / input.texCoord.z;
		color *= sampleLegacyTexture(coordinate);
	}

	color = saturate(color);
	if (uFlags2.x != 0u && !alphaTestPass(color.a, uFogParams.w, uFlags2.y))
		discard;

	if (uFlags1.z != FOG_OFF)
	{
		float factor = fogFactor(input.fogCoord);
		color.rgb = lerp(uFogColor.rgb, color.rgb, factor);
	}
	return saturate(color);
}

float4 main(PixelInput input) : SV_Target0
{
	return shade(input);
}

uint4 mainLogic(PixelInput input) : SV_Target0
{
	return (uint4)floor(shade(input) * 255.0 + 0.5);
}
)HLSL";

static const char *CLEAR_VERTEX_SHADER = R"HLSL(
float4 main(uint vertexId : SV_VertexID) : SV_Position
{
	float2 position = float2((vertexId << 1u) & 2u, vertexId & 2u);
	return float4(position * 2.0 - 1.0, 0.0, 1.0);
}
)HLSL";

static const char *CLEAR_PIXEL_SHADER = R"HLSL(
cbuffer ClearBlock : register(b0)
{
	float4 uClearColor;
};

float4 main() : SV_Target0
{
	return uClearColor;
}
)HLSL";

static const char *PRESENT_VERTEX_SHADER = R"HLSL(
float4 main(uint vertexId : SV_VertexID) : SV_Position
{
	float2 position = float2((vertexId << 1u) & 2u, vertexId & 2u);
	return float4(position * 2.0 - 1.0, 0.0, 1.0);
}
)HLSL";

static const char *PRESENT_PIXEL_SHADER = R"HLSL(
Texture2D<float4> uSource : register(t0);

float4 main(float4 position : SV_Position) : SV_Target0
{
	return uSource.Load(int3(int2(position.xy), 0));
}
)HLSL";

const char *legacyVertexShaderSource()
{
	return LEGACY_VERTEX_SHADER;
}

const char *legacyPixelShaderSource()
{
	return LEGACY_PIXEL_SHADER;
}

const char *clearVertexShaderSource()
{
	return CLEAR_VERTEX_SHADER;
}

const char *clearPixelShaderSource()
{
	return CLEAR_PIXEL_SHADER;
}

const char *presentVertexShaderSource()
{
	return PRESENT_VERTEX_SHADER;
}

const char *presentPixelShaderSource()
{
	return PRESENT_PIXEL_SHADER;
}

}
