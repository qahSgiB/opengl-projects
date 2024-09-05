#version 450 core



// vertex input
layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coord;


// uniform input
layout (std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;
	mat4 projection_inv;
	mat4 view;
	mat4 view_inv;
	mat3 view_it;
	vec3 eye_position;
};

layout (std140, binding = 1) uniform ModelData
{
	mat4 model;
	mat4 model_inv;
	mat3 model_it;
};



void main()
{
	gl_Position = projection * view * model * position;
}
