#version 450

layout(std140, set=0, binding=0) uniform CBData
{
    mat4 mvpMatrix;
    mat4 modelViewMatrix;
    mat4 textureMatrix;

    vec4 lightDir0;
    vec4 lightDir1;
    vec4 lightDiffuse0;
    vec4 lightDiffuse1;
    vec4 globalAmbient;

    vec4 currentColor;
    vec4 currentNormal;

    uint lightingEnabled;
    uint textureEnabled;
    uint hasVertexColor;
    uint hasVertexNormal;
    uint hasVertexTexCoord;

    uint _pad1a;
    uint _pad1b;
    uint _pad1c;

    vec4 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint fogMode;

    float alphaRef;
    uint alphaTestEnabled;

    float _pad2a;
    float _pad2b;
};

layout(set=0, binding=1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float fragFogDist;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = fragColor;

    if (textureEnabled != 0u)
    {
        color *= texture(texSampler, fragUV);
    }

    // Alpha test
    if (alphaTestEnabled != 0u && color.a <= alphaRef)
    {
        discard;
    }

    // Fog
    if (fogMode == 1u) // LINEAR
    {
        float f = clamp((fogEnd - fragFogDist) / (fogEnd - fogStart), 0.0, 1.0);
        color.rgb = mix(fogColor.rgb, color.rgb, f);
    }
    else if (fogMode == 2u) // EXP
    {
        float f = clamp(exp(-fogDensity * fragFogDist), 0.0, 1.0);
        color.rgb = mix(fogColor.rgb, color.rgb, f);
    }

    outColor = clamp(color, vec4(0.0), vec4(1.0));
}
