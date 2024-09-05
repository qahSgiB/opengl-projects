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


// output
out VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} out_data;



void main()
{
	out_data.tex_coord = tex_coord;
	out_data.position_ws = vec3(model * position);
	out_data.normal_ws = normalize(model_it * normal);

	gl_Position = projection * view * model * position;
}
