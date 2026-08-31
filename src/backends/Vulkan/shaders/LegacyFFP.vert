#version 450

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

layout(std140, set = 0, binding = 0) uniform LegacyFFPBlock
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
	vec4 uCurrentColor;
	vec4 uCurrentNormal;
	vec4 uCurrentTexCoord;
	uvec4 uFlags4;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inFlatPosition;
layout(location = 5) in vec4 inFlatColor;
layout(location = 6) in vec3 inFlatNormal;

layout(location = 0) out vec3 vTexCoord;
layout(location = 1) out vec4 vSmoothPrimary;
layout(location = 2) flat out vec4 vFlatPrimary;
layout(location = 3) out float vFogCoord;

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
	// A display list retains one resident copy of its geometry. Attributes the
	// captured draw never supplied are read here from the current state, which
	// is what lets one chunk or glyph list be drawn under any current colour,
	// normal or texture coordinate without duplicating its vertices.
	vec4 color = uFlags4.x != 0u ? inColor : uCurrentColor;
	vec3 normal = uFlags4.y != 0u ? inNormal : uCurrentNormal.xyz;
	vec2 texCoord = uFlags4.z != 0u ? inTexCoord : uCurrentTexCoord.xy;
	vec4 flatColor = uFlags4.x != 0u ? inFlatColor : uCurrentColor;
	vec3 flatNormal = uFlags4.y != 0u ? inFlatNormal : uCurrentNormal.xyz;

	vec4 eyePosition = uModelView * vec4(inPosition, 1.0);
	vec4 glClip = uProjection * eyePosition;
	gl_Position = vec4(glClip.xy, 0.5 * (glClip.z + glClip.w), glClip.w);
	gl_PointSize = 1.0;

	vec4 transformedTexCoord = uTexture * vec4(texCoord, 0.0, 1.0);
	vTexCoord = transformedTexCoord.xyw;
	vSmoothPrimary = computePrimary(inPosition, color, normal);
	vFlatPrimary = computePrimary(inFlatPosition, flatColor, flatNormal);
	vFogCoord = uFlags1.w == 2u ? length(eyePosition.xyz) :
		(uFlags1.w == 1u ? eyePosition.z : abs(eyePosition.z));
}
