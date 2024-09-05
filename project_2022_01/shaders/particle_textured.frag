#version 450 core



in VertexData
{
    vec2 tex_coord;
    vec4 color;

    vec3 position_vs;

    flat int id;
} in_data;


layout(binding = 0) uniform sampler2D particle_texture;

layout (location = 2) uniform float mirror_clip_distance;


layout(location = 0) out vec4 final_color;



void main()
{
    if (-in_data.position_vs.z < mirror_clip_distance) {
        discard;
    }

    float texture_intensity = texture(particle_texture, in_data.tex_coord).r;
    final_color = vec4(in_data.color.rgb * texture_intensity, texture_intensity);
}