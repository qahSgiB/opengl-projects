#version 450 core


// fragment input
in VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} in_data;


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

struct PhongLight
{
	vec4 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 spot_direction;
	float spot_exponent;
	float spot_cos_cutoff;
	float atten_constant;
	float atten_linear;
	float atten_quadratic;
};

layout (std140, binding = 2) uniform PhongLightsBuffer
{
	vec3 global_ambient_color;
	int lights_count;
	PhongLight lights[8];
};

layout (std140, binding = 3) uniform PhongMaterialBuffer
{
    vec3 ambient;
    vec3 diffuse;
    float alpha;
    vec3 specular;
    float shininess;
} material;

layout (std140, binding = 4) uniform TopCameraBuffer
{
	mat4 top_projection;
	mat4 top_projection_inv;
	mat4 top_view;
	mat4 top_view_inv;
	mat3 top_view_it;
	vec3 top_eye_position;
};

layout (location = 0) uniform bool has_tex;
layout (location = 1) uniform float uv_mult;

layout (location = 2) uniform bool show_snow_accum;

layout (binding = 0) uniform sampler2D material_diffuse_tex;
layout (binding = 1) uniform sampler2D snow_accum_tex;
layout (binding = 2) uniform sampler2D snow_shadow_tex;


// output
layout (location = 0) out vec4 final_color;



void main()
{
	vec3 N = normalize(in_data.normal_ws);
	vec3 V = normalize(eye_position - in_data.position_ws);
	
	vec3 amb = global_ambient_color;
	vec3 dif = vec3(0.0);
	vec3 spe = vec3(0.0);

	for (int i = 0; i < lights_count; i++) {
		vec3 L_not_normalized = lights[i].position.xyz - in_data.position_ws * lights[i].position.w;
		vec3 L = normalize(L_not_normalized);
		vec3 H = normalize(L + V);

		float Iamb = 1.0;
		float Idif = max(dot(N, L), 0.0);
		float Ispe = (Idif > 0.0) ? pow(max(dot(N, H), 0.0), material.shininess) : 0.0;

		// spot light factor
		if (lights[i].spot_cos_cutoff != -1.0) {
			float spot_cos_angle = dot(-L, lights[i].spot_direction);
			float spot_factor = (spot_cos_angle > lights[i].spot_cos_cutoff) ? pow(spot_cos_angle, lights[i].spot_exponent) : 0.0;

			Iamb *= 1.0;
			Idif *= spot_factor;
			Ispe *= spot_factor;
		}

		// attenuation factor
		if (lights[i].position.w != 0.0) {
			float distance_from_light = length(L_not_normalized);
			float atten_factor = lights[i].atten_constant + lights[i].atten_linear * distance_from_light +  lights[i].atten_quadratic * distance_from_light * distance_from_light;
			atten_factor = 1.0 / atten_factor;

			Iamb *= atten_factor;
			Idif *= atten_factor;
			Ispe *= atten_factor;
		}

		amb += Iamb * lights[i].ambient;
		dif += Idif * lights[i].diffuse;
		spe += Ispe * lights[i].specular;
	}

	// material
	vec3 mat_ambient = has_tex ? texture(material_diffuse_tex, in_data.tex_coord * uv_mult).rgb :  material.ambient;
	vec3 mat_diffuse = has_tex ? texture(material_diffuse_tex, in_data.tex_coord * uv_mult).rgb :  material.diffuse;
	vec3 mat_specular = material.specular;

	// apply snow
	if (show_snow_accum) {
		vec4 top_pos_clip4 = top_projection * top_view * vec4(in_data.position_ws, 1.0);
		vec3 top_pos_clip = top_pos_clip4.xyz / top_pos_clip4.w;
		vec3 top_pos_ndc = top_pos_clip * 0.5 + vec3(0.5);
		
		float snow_shadow_ndc_z = texture(snow_shadow_tex, top_pos_ndc.xy).r;
		if (top_pos_ndc.z <= snow_shadow_ndc_z + 0.01) {
			vec3 snow_dir = normalize(mat3(top_view_inv) * vec3(0.0, 0.0, 1.0));
			float snow_dir_mult = max(dot(snow_dir, in_data.normal_ws), 0.0);

			float snow_accum_mult = texture(snow_accum_tex, top_pos_ndc.xy).r;

			float snow_mult = snow_accum_mult * snow_dir_mult;

			vec3 snow_color = vec3(1.0);
			mat_ambient = mix(mat_ambient, snow_color, snow_mult);
			mat_diffuse = mix(mat_diffuse, snow_color, snow_mult);
			mat_specular = mix(mat_specular, snow_color, snow_mult);
		}
	}

	// final color
	vec3 final_light = mat_ambient * amb + mat_diffuse * dif + material.specular * spe;
	final_color = vec4(final_light, material.alpha);
}