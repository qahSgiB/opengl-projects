#version 450 core


// vertex input
layout (location = 0) in vec4 position;


// uniform input
layout (location = 0) uniform float elapsed_time_s;

layout (std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;
	mat4 projection_inv;
	mat4 view;
	mat4 view_inv;
	mat3 view_it;
	vec3 eye_position;
};


// output
out VertexData
{
	vec3 position_ws;
	vec4 position_vs;
} out_data;



void main()
{
	vec4 p = position;
	p.y = mod(p.y - elapsed_time_s * 3.0, 50.0);
	out_data.position_vs = view * p;
}
