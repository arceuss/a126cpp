#version 450

layout(push_constant) uniform ClearPushConstants
{
	vec4 color;
	float depth;
} uClear;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = uClear.color;
	gl_FragDepth = uClear.depth;
}
