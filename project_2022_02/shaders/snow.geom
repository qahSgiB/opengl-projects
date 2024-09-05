#version 450 core


// geometry specification
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;


// vertex input
in VertexData
{
	vec3 position_ws;
	vec4 position_vs;
} in_data[1];


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


// output
out VertexData
{
	vec2 tex_coord;
	vec3 position_ws;
} out_data;



const vec2 quad_tex_coords[4] = vec2[4](
	vec2(0.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0)
);
const vec4 quad_offsets[4] = vec4[4](
	vec4(-0.5, +0.5, 0.0, 0.0),
	vec4(-0.5, -0.5, 0.0, 0.0),
	vec4(+0.5, +0.5, 0.0, 0.0),
	vec4(+0.5, -0.5, 0.0, 0.0)
);


void main()
{
	float particle_size_vs = 0.5;

	for (int i = 0; i < 4; i++) {
		out_data.tex_coord = quad_tex_coords[i];
		out_data.position_ws = in_data[0].position_ws;
		gl_Position = projection * (in_data[0].position_vs + particle_size_vs * quad_offsets[i]);
		
		EmitVertex();
	}
	EndPrimitive();
}
