#version 450 core


// fragment input
in VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} in_data;


// output
layout (location = 0) out float color;



void main()
{
    color = 1.0;
}