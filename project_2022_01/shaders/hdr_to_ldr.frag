#version 450 core



in VertexData
{
	vec2 tex_coord;
} in_data;



layout(location = 0) uniform float exposure;
layout(location = 1) uniform float gamma;

layout(binding = 0) uniform sampler2D input_tex;



layout(location = 0) out vec4 final_color;



void main()
{
	vec3 c = texture(input_tex, in_data.tex_coord).rgb;
	vec3 c_exposure = 1.0f - exp2(-c * exposure);
	vec3 c_gamma = pow(c_exposure, vec3(1.0f / gamma));
	final_color = vec4(c_gamma, 1.0f);
}