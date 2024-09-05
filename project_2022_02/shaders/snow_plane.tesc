#version 450 core


// tesselation
layout (vertices = 3) out;
// layout (vertices = 4) out;


// patch input
in VertexData
{
	vec3 position_ws;
	vec2 tex_coord;
} in_data[];


// uniform input
layout (location = 3) uniform float tess_factor;


// output
out VertexData
{
	vec3 position_ws;
	vec2 tex_coord;
} out_data[];



void main()
{
    out_data[gl_InvocationID].position_ws = in_data[gl_InvocationID].position_ws;
    out_data[gl_InvocationID].tex_coord = in_data[gl_InvocationID].tex_coord;

    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = tess_factor;
        gl_TessLevelInner[1] = tess_factor;
        gl_TessLevelOuter[0] = tess_factor;
        gl_TessLevelOuter[1] = tess_factor;
        gl_TessLevelOuter[2] = tess_factor;
        gl_TessLevelOuter[3] = tess_factor;
    }
}