#pragma once

// HLSL shader source strings for the D3D11 rendering backend.
// Compiled at runtime via D3DCompile().

static const char* g_vsSource = R"HLSL(
cbuffer PerDraw : register(b0)
{
    float4x4 mvpMatrix;
    float4x4 modelViewMatrix;
    float4x4 textureMatrix;

    // Lighting
    float4 lightDir0;      // xyz=direction, w=enabled
    float4 lightDir1;
    float4 lightDiffuse0;
    float4 lightDiffuse1;
    float4 globalAmbient;

    // Current color (for non-vertex-colored geometry)
    float4 currentColor;

    // Current normal
    float4 currentNormal;

    // Feature flags
    uint lightingEnabled;
    uint textureEnabled;
    uint hasVertexColor;
    uint hasVertexNormal;
    uint hasVertexTexCoord;

    // Fog (shared with PS, but VS needs to compute distance)
    float4 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint fogMode; // 0=off, 1=linear, 2=exp

    // Alpha test
    float alphaRef;
    uint alphaTestEnabled;

    float2 _pad0;
};

struct VSInput
{
    float3 pos    : POSITION;
    float2 uv     : TEXCOORD0;
    float4 color  : COLOR0;
    float4 normal : NORMAL;
};

struct VSOutput
{
    float4 pos     : SV_POSITION;
    float2 uv      : TEXCOORD0;
    float4 color   : COLOR0;
    float fogDist  : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.pos = mul(float4(input.pos, 1.0), mvpMatrix);

    // Texcoord
    if (hasVertexTexCoord)
    {
        float4 tc = mul(float4(input.uv, 0.0, 1.0), textureMatrix);
        output.uv = tc.xy;
    }
    else
    {
        output.uv = float2(0.0, 0.0);
    }

    // Color
    float4 vertColor = hasVertexColor ? input.color : currentColor;

    if (lightingEnabled)
    {
        float3 n;
        if (hasVertexNormal)
            n = input.normal.xyz;
        else
            n = currentNormal.xyz;

        // Transform normal by modelview matrix (OpenGL convention).
        // Light direction was already transformed to eye space at glLightfv time.
        // With w=0, translation is ignored — only rotation/scale applied.
        n = mul(float4(n, 0.0), modelViewMatrix).xyz;

        // GL_RESCALE_NORMAL / GL_NORMALIZE: renormalize after MV transform
        n = normalize(n);

        float3 lit = globalAmbient.rgb;

        if (lightDir0.w > 0.5)
        {
            float ndl = max(0.0, dot(n, lightDir0.xyz));
            lit += lightDiffuse0.rgb * ndl;
        }
        if (lightDir1.w > 0.5)
        {
            float ndl = max(0.0, dot(n, lightDir1.xyz));
            lit += lightDiffuse1.rgb * ndl;
        }

        // GL_COLOR_MATERIAL: vertex color is material ambient+diffuse
        vertColor.rgb *= lit;
    }

    output.color = vertColor;

    // Fog distance (eye-space)
    float4 eyePos = mul(float4(input.pos, 1.0), modelViewMatrix);
    output.fogDist = length(eyePos.xyz);

    return output;
}
)HLSL";

static const char* g_psSource = R"HLSL(
cbuffer PerDraw : register(b0)
{
    float4x4 mvpMatrix;
    float4x4 modelViewMatrix;
    float4x4 textureMatrix;

    float4 lightDir0;
    float4 lightDir1;
    float4 lightDiffuse0;
    float4 lightDiffuse1;
    float4 globalAmbient;

    float4 currentColor;
    float4 currentNormal;

    uint lightingEnabled;
    uint textureEnabled;
    uint hasVertexColor;
    uint hasVertexNormal;
    uint hasVertexTexCoord;

    float4 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint fogMode;

    float alphaRef;
    uint alphaTestEnabled;

    float2 _pad0;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct PSInput
{
    float4 pos     : SV_POSITION;
    float2 uv      : TEXCOORD0;
    float4 color   : COLOR0;
    float fogDist  : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = input.color;

    if (textureEnabled)
    {
        color *= tex.Sample(samp, input.uv);
    }

    // Alpha test
    if (alphaTestEnabled && color.a <= alphaRef)
    {
        discard;
    }

    // Fog
    if (fogMode == 1) // LINEAR
    {
        float f = saturate((fogEnd - input.fogDist) / (fogEnd - fogStart));
        color.rgb = lerp(fogColor.rgb, color.rgb, f);
    }
    else if (fogMode == 2) // EXP
    {
        float f = saturate(exp(-fogDensity * input.fogDist));
        color.rgb = lerp(fogColor.rgb, color.rgb, f);
    }

    return color;
}
)HLSL";
