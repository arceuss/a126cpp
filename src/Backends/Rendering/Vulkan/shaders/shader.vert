#version 450

layout(std140, set=0, binding=0) uniform CBData
{
    mat4 mvpMatrix;
    mat4 modelViewMatrix;
    mat4 textureMatrix;

    vec4 lightDir0;      // xyz=direction, w=enabled
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

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec4 inNormal;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDist;

void main()
{
    gl_Position = mvpMatrix * vec4(inPos, 1.0);

    // Texcoord
    if (hasVertexTexCoord != 0u)
    {
        vec4 tc = textureMatrix * vec4(inUV, 0.0, 1.0);
        fragUV = tc.xy;
    }
    else
    {
        fragUV = vec2(0.0, 0.0);
    }

    // Color
    vec4 vertColor = (hasVertexColor != 0u) ? inColor : currentColor;

    if (lightingEnabled != 0u)
    {
        vec3 n;
        if (hasVertexNormal != 0u)
            n = inNormal.xyz;
        else
            n = currentNormal.xyz;

        // Transform normal by modelview matrix (OpenGL convention).
        n = (modelViewMatrix * vec4(n, 0.0)).xyz;

        // GL_RESCALE_NORMAL / GL_NORMALIZE
        n = normalize(n);

        vec3 lit = globalAmbient.rgb;

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

        // Fixed-function lighting clamps the primary color before texturing.
        // Without this, upward-facing surfaces can exceed 1.0 and wash out.
        vertColor.rgb *= lit;
    }

    fragColor = clamp(vertColor, vec4(0.0), vec4(1.0));

    // Fog distance (eye-space)
    vec4 eyePos = modelViewMatrix * vec4(inPos, 1.0);
    fragFogDist = length(eyePos.xyz);
}
