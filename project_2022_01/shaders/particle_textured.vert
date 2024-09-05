#version 450 core



layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 fade_timing;
layout (location = 3) in vec2 blink_timing;


layout (std140, binding = 0) uniform CameraBuffer
{
    mat4 projection;
    mat4 projection_inv;
    mat4 view;
    mat4 view_inv;
    mat3 view_it;
    vec3 eye_position;
};

layout (location = 0) uniform uint stage;
layout (location = 1) uniform float alive_time;


out VertexData
{
    vec4 position_ws;
    vec4 color;

    float fade;
    float blink;

    flat int id;
} out_data;



void main()
{
    out_data.position_ws = position;
    
    out_data.color = color;
    
    float fade_start = fade_timing.x;
    float fade_end = fade_timing.y;
    out_data.fade = (stage == 0 || alive_time < fade_start) ? 1.0f : (alive_time > fade_end ? -1.0f : 1.0f - ((alive_time - fade_start) / (fade_end - fade_start)));

    float blink_start = blink_timing.x;
    float blink_freq = blink_timing.y;
    out_data.blink = (stage == 0 || alive_time < blink_start) ? 1.0f : 0.5 * cos(2.0f * 3.14159f * (alive_time - blink_start) / blink_freq) + 0.5f;
    
    out_data.id = gl_VertexID;
}