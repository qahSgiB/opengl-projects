#version 430 core


// fragment input
in VertexData
{
	vec2 tex_coord;
	vec3 position_ws;
} in_data;


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

layout (std140, binding = 3) uniform PhongMaterialBuffer
{
    vec3 ambient;
    vec3 diffuse;
    float alpha;
    vec3 specular;
    float shininess;
} material;

layout (binding = 0) uniform sampler2D particle_texture;


// output
layout (location = 0) out vec4 final_color;



void main()
{
	float intensity = texture(particle_texture, in_data.tex_coord).r;
	final_color = vec4(1.0f, 1.0f, 1.0f, 0.2 * intensity);
}