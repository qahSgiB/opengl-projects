#version 450 core


// fragment input
in VertexData
{
    vec2 tex_coord;
} in_data;


// uniform input
layout(location = 0) uniform float snow_vel;
layout(location = 1) uniform bool clear;

layout(location = 2) uniform float delta;

layout(binding = 0) uniform sampler2D snow_in;
layout(binding = 1) uniform sampler2D broom_pos;


// output
layout (location = 0) out float snow_out;



void main()
{
    snow_out = (clear || texture(broom_pos, in_data.tex_coord).r > 0.0) ? 0.0 : min(texture(snow_in, in_data.tex_coord).r + snow_vel * delta, 1.0);
}