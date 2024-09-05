#version 450 core


// fragment input
in VertexData
{
    vec2 tex_coord;
} in_data;


// uniform input
layout(binding = 0) uniform sampler2D tex;


// output
layout (location = 0) out vec4 color;



void main()
{
    color = vec4(texture(tex, in_data.tex_coord).rgb, 1.0);
}