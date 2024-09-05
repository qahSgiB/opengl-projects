#version 450 core


// fragment input
in VertexData
{
    vec2 tex_coord;
} in_data;


// uniform inputs
layout (location = 0) uniform float radius;

layout (binding = 0) uniform sampler2D color_in;


// output
layout (location = 0) out float color_out;



const float gauss_k[8] = float[8](5.98, 5.61, 4.64, 3.37, 2.16, 1.21, 0.6, 0.26);
// const float gauss_k[3] = float[3](6.0, 4.0, 1.0);


void main()
{
    int gauss_size = gauss_k.length();

    vec2 texel_size = 1.0 / textureSize(color_in, 0);
    if (radius > 0.0) {
        texel_size *= radius / (gauss_size - 1);
    }

    float color_sum = 0.0;
    float weight_sum = 0.0;
    for (int y = -gauss_size + 1; y <= gauss_size - 1; y++) {
        for (int x = -gauss_size + 1; x <= gauss_size - 1; x++) {
            float gauss_mult = gauss_k[abs(y)] * gauss_k[abs(x)];
            color_sum += gauss_mult * texture(color_in, in_data.tex_coord + vec2(x, y) * texel_size).r;
            weight_sum += gauss_mult;
        }
    }

    color_out = color_sum / weight_sum;
}