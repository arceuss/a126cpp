// Vertex shader - mechanical copy from D3D11_Shaders.h for HLSL→SPIRV→GLSL comparison
// Added Vulkan bindings for DXC -spirv

struct CBData
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

    uint _pad1a;
    uint _pad1b;
    uint _pad1c;

    float4 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint fogMode;

    float alphaRef;
    uint alphaTestEnabled;

    float2 _pad0;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CBData> cb : register(b0);

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
    float  fogDist : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.pos = mul(float4(input.pos, 1.0), cb.mvpMatrix);

    // Texcoord
    if (cb.hasVertexTexCoord)
    {
        float4 tc = mul(float4(input.uv, 0.0, 1.0), cb.textureMatrix);
        output.uv = tc.xy;
    }
    else
    {
        output.uv = float2(0.0, 0.0);
    }

    // Color
    float4 vertColor = cb.hasVertexColor ? input.color : cb.currentColor;

    if (cb.lightingEnabled)
    {
        float3 n;
        if (cb.hasVertexNormal)
            n = input.normal.xyz;
        else
            n = cb.currentNormal.xyz;

        n = mul(float4(n, 0.0), cb.modelViewMatrix).xyz;
        n = normalize(n);

        float3 lit = cb.globalAmbient.rgb;

        if (cb.lightDir0.w > 0.5)
        {
            float ndl = max(0.0, dot(n, cb.lightDir0.xyz));
            lit += cb.lightDiffuse0.rgb * ndl;
        }
        if (cb.lightDir1.w > 0.5)
        {
            float ndl = max(0.0, dot(n, cb.lightDir1.xyz));
            lit += cb.lightDiffuse1.rgb * ndl;
        }

        vertColor.rgb *= lit;
    }

    output.color = vertColor;

    float4 eyePos = mul(float4(input.pos, 1.0), cb.modelViewMatrix);
    output.fogDist = length(eyePos.xyz);

    return output;
}
