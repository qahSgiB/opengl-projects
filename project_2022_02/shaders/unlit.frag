#version 450 core


// fragment input
in VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} in_data;


// uniform input
layout (std140, binding = 3) uniform PhongMaterialBuffer
{
    vec3 ambient;
    vec3 diffuse;
    float alpha;
    vec3 specular;
    float shininess;
} material;

layout (location = 0) uniform bool has_tex;
layout (location = 1) uniform float uv_mult;

layout (location = 2) uniform float show_snow_accum;

layout (binding = 0) uniform sampler2D material_diffuse_tex;


// output
layout (location = 0) out vec4 final_color;



void main()
{
	vec3 color = has_tex ? texture(material_diffuse_tex, in_data.tex_coord * uv_mult).rgb : material.diffuse;
	final_color = vec4(color, 1.0);
}