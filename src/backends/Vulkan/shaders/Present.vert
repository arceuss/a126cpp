#version 450

const vec2 POSITIONS[3] = vec2[](
	vec2(-1.0, -1.0),
	vec2(3.0, -1.0),
	vec2(-1.0, 3.0));

const vec2 TEX_COORDS[3] = vec2[](
	vec2(0.0, 0.0),
	vec2(2.0, 0.0),
	vec2(0.0, 2.0));

layout(location = 0) out vec2 vTexCoord;

void main()
{
	gl_Position = vec4(POSITIONS[gl_VertexIndex], 0.0, 1.0);
	// Legacy draws use a negative-height viewport, so image row zero is
	// already the visual top. The positive-height present pass must not flip V.
	vTexCoord = TEX_COORDS[gl_VertexIndex];
}
