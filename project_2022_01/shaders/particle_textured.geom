#version 450 core



layout (points) in;
layout (triangle_strip, max_vertices = 4) out;



in VertexData
{
    vec4 position_ws;
    vec4 color;

    float fade;
    float blink;

    flat int id;
} in_data[1];


layout(std140, binding = 0) uniform CameraBuffer
{
    mat4 projection;
    mat4 projection_inv;
    mat4 view;
    mat4 view_inv;
    mat3 view_it;
    vec3 eye_position;
};

layout (std140, binding = 1) buffer FireworkParams
{
    uint particle_count;

    float explosion_force;
    float explosion_force_variance;

    float particle_size_base;
    float rocket_size_mult;

    float hue_variance;
    float saturation_variance;

    float end_time;

    float fade_start;
    float fade_start_variance;
    float fade_end_variance;
    float fade_size_mult;

    float blink_start;
    float blink_freq;
    float blink_start_variance;
    float blink_freq_variance;
    float blink_size_mult;
};

layout(location = 0) uniform uint stage;


out VertexData
{
    vec2 tex_coord;
    vec4 color;

    vec3 position_vs;

    flat int id;
} out_data;



const vec2 quad_tex_coords[4] = vec2[4](
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

const vec2 quad_offsets[4] = vec2[4](
    vec2(-0.5, +0.5),
    vec2(-0.5, -0.5),
    vec2(+0.5, +0.5),
    vec2(+0.5, -0.5)
);



void main()
{
    // flying1 stage (rocket) multiplier
    float size_mult_rocket = stage == 0 ? rocket_size_mult : 1.0f;

    // fading multiplier
    float fade = in_data[0].fade;
    float size_mult_fade = (fade < 0.0f ? 0.0f : pow(fade, 0.5f) * (1.0f - fade_size_mult) + fade_size_mult);

    // blinking multiplier
    float blink = in_data[0].blink;
    float size_mult_blink = blink * (1.0f - blink_size_mult) + blink_size_mult;

    // total multiplier
    float size_mult = size_mult_blink * size_mult_fade * size_mult_rocket * particle_size_base;

    for (int i = 0; i < 4; i++)
    {
        out_data.tex_coord = quad_tex_coords[i];
        out_data.color = in_data[0].color;
        out_data.id = in_data[0].id;

        vec4 position_ws = in_data[0].position_ws;

        vec3 up = eye_position - vec3(position_ws / position_ws.w);
        vec3 tangent = (up.x == 0.0f) ? vec3(1.0f, 0.0f, 0.0f) : normalize(vec3(up.y, -up.x, 0.0f));
        vec3 bitangent = normalize(cross(tangent, up));

        vec2 offset = quad_offsets[i];

        vec4 position_vs = view * (position_ws + vec4(size_mult * (offset.x * tangent + offset.y * bitangent), 0.0f));
        out_data.position_vs = position_vs.xyz;
        gl_Position = projection * position_vs;

        EmitVertex();
    }
}