#version 450 core


// tesselation
layout (triangles, equal_spacing, ccw) in;
// layout (quads, equal_spacing, ccw) in;


// input
in VertexData
{
	vec3 position_ws;
	vec2 tex_coord;
} in_data[];


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

layout (std140, binding = 4) uniform SnowCameraBuffer
{
	mat4 snow_dir_projection;
	mat4 snow_dir_projection_inv;
	mat4 snow_dir_view;
	mat4 snow_dir_view_inv;
	mat3 snow_dir_view_it;
	vec3 snow_dir_eye_position;
};

layout (location = 2) uniform float snow_height_max;

layout (binding = 1) uniform sampler2D snow_accum_tex;
layout (binding = 2) uniform sampler2D snow_shadow_tex;
layout (binding = 3) uniform sampler2D snow_plane_base_tex;

layout (binding = 4) uniform sampler2D snow_height_tex;


// output
out VertexData
{
	vec3 position_ws;
	vec2 tex_coord;
} out_data;



vec3 interpolate_triangle(vec3 v0, vec3 v1, vec3 v2, vec3 bar)
{
    return v0 * bar.x + v1 * bar.y + v2 * bar.z;
}

vec2 interpolate_triangle(vec2 v0, vec2 v1, vec2 v2, vec3 bar)
{
    return v0 * bar.x + v1 * bar.y + v2 * bar.z;
}

vec3 interpolate_quad(vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec2 t)
{
    return mix(mix(v0, v1, t.x), mix(v2, v3, 1 - t.x), t.y);
}

vec2 interpolate_quad(vec2 v0, vec2 v1, vec2 v2, vec2 v3, vec2 t)
{
    return mix(mix(v0, v1, t.x), mix(v2, v3, 1 - t.x), t.y);
}



void main()
{
    vec3 pos = interpolate_triangle(in_data[0].position_ws, in_data[1].position_ws, in_data[2].position_ws, gl_TessCoord.xyz);
    vec2 tex_coord = interpolate_triangle(in_data[0].tex_coord, in_data[1].tex_coord, in_data[2].tex_coord, gl_TessCoord.xyz);

    // out_data.position_ws = interpolate_quad(in_data[0].position_ws, in_data[1].position_ws, in_data[2].position_ws, in_data[3].position_ws, gl_TessCoord.xy);
    // out_data.tex_coord = interpolate_quad(in_data[0].tex_coord, in_data[1].tex_coord, in_data[2].tex_coord, in_data[3].tex_coord, gl_TessCoord.xy);
    
    vec4 snow_dir_pos_clip4 = snow_dir_projection * snow_dir_view * vec4(pos, 1.0);
    vec3 snow_dir_pos_clip = snow_dir_pos_clip4.xyz / snow_dir_pos_clip4.w;
    vec3 snow_dir_pos_ndc = snow_dir_pos_clip * 0.5 + vec3(0.5);

    float snow_shadow_ndc_z = texture(snow_shadow_tex, snow_dir_pos_ndc.xy).r;
    float snow_plane_base_ndc_z = texture(snow_plane_base_tex, snow_dir_pos_ndc.xy).r;

    if (snow_dir_pos_ndc.z <= snow_shadow_ndc_z + 0.01 || snow_shadow_ndc_z >= snow_plane_base_ndc_z - 0.01) {
        float snow_height = texture(snow_accum_tex, snow_dir_pos_ndc.xy).r;

        float snow_height_map = texture(snow_height_tex, snow_dir_pos_ndc.xy).r;

        pos.y += snow_height * snow_height_max * snow_height_map;
    }

    out_data.position_ws = pos;
    out_data.tex_coord = tex_coord;

    gl_Position = projection * view * vec4(pos, 1.0);
}