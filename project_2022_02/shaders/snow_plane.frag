#version 450 core


// fragment input
in VertexData
{
	vec3 position_ws;
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

layout (location = 2) uniform float snow_height_max;

layout (location = 4) uniform bool pbr;

layout (binding = 0) uniform sampler2D material_diffuse_tex;
layout (binding = 1) uniform sampler2D snow_accum_tex;
layout (binding = 2) uniform sampler2D snow_shadow_tex;

layout (binding = 5) uniform sampler2D snow_normal_tex;
layout (binding = 6) uniform sampler2D snow_roughness_tex;


// output
layout (location = 0) out vec4 final_color;



float md(vec3 a, vec3 b) { return max(dot(a, b), 0.0); }
float mdp(vec3 a, vec3 b) { return max(dot(a, b), 0.0001); }

vec3 fresnel_schlick(in vec3 f0, in vec3 V, in vec3 H)
{
    return f0 + (1 - f0) * pow(1 - max(dot(V, H), 0.0), 5.0);
}

float beckmann_distrib(in vec3 N, in vec3 H, in float m)
{
    float nh = md(N, H);
    return exp((nh * nh - 1.0) / max((m * m * nh * nh), 0.0001)) / max((m * m * pow(nh, 4)), 0.0001);
}

float geometric_atten(in vec3 N, in vec3 V, in vec3 L, in vec3 H)
{
    return min(1.0, min(2 * md(N, H) * md(N, V) / mdp(V, H), 2 * md(N, H) * md(N, L) / mdp(V, H)));
}


void main()
{	
	// compute normal
	vec3 texel_size3 = vec3(1.0 / textureSize(snow_accum_tex, 0), 1.0);

	vec4 top_pos_clip4 = top_projection * top_view * vec4(in_data.position_ws, 1.0);
    vec3 top_pos_clip = top_pos_clip4.xyz / top_pos_clip4.w;
    vec3 top_pos_ndc = top_pos_clip * 0.5 + vec3(0.5);

	vec3 top_ex = vec3(1.0, 0.0, 0.0) * texel_size3;
	vec3 top_ey = vec3(0.0, 1.0, 0.0) * texel_size3;

	float xp_sh = texture(snow_accum_tex, (top_pos_ndc + top_ex).xy).r;
	float xn_sh = texture(snow_accum_tex, (top_pos_ndc - top_ex).xy).r;
	float yp_sh = texture(snow_accum_tex, (top_pos_ndc + top_ey).xy).r;
	float yn_sh = texture(snow_accum_tex, (top_pos_ndc - top_ey).xy).r;

	vec3 snow_height_dir = vec3(0.0, 1.0, 0.0);

	vec3 ex = mat3(top_view_inv) * mat3(top_projection_inv) * top_ex;
	vec3 ey = mat3(top_view_inv) * mat3(top_projection_inv) * top_ey;

	vec3 xp =  ex + snow_height_dir * snow_height_max * xp_sh;
	vec3 xn = -ex + snow_height_dir * snow_height_max * xn_sh;
	vec3 yp =  ey + snow_height_dir * snow_height_max * yp_sh;
	vec3 yn = -ey + snow_height_dir * snow_height_max * yn_sh;

	vec3 tx = normalize(xp - xn);
	vec3 ty = normalize(yp - yn);

	vec3 n = normalize(cross(tx, ty));

	vec3 normal_map = texture(snow_normal_tex, top_pos_ndc.xy).xyz * 2.0 - 1.0;

	// vec3 N = n;
	vec3 N = normalize(tx * normal_map.x + ty * normal_map.y + n * normal_map.z);
	// vec3 N = normalize(ty * normal_map.x + tx * normal_map.y + n * normal_map.z);

	// lighting
	vec3 fresnel0 = vec3(0.1);
	float roughness = texture(snow_roughness_tex, top_pos_ndc.xy).r;

	vec3 V = normalize(eye_position - in_data.position_ws);
	
	vec3 amb = global_ambient_color;
	vec3 dif = vec3(0.0);
	vec3 spe = vec3(0.0);

	for (int i = 0; i < lights_count; i++) {
		vec3 L_not_normalized = lights[i].position.xyz - in_data.position_ws * lights[i].position.w;
		vec3 L = normalize(L_not_normalized);
		vec3 H = normalize(L + V);

		vec3 Iamb = vec3(0.0);
		vec3 Idif = vec3(0.0);
		vec3 Ispe = vec3(0.0);

		if (pbr) {
			Iamb = vec3(0.0);
			Idif = vec3(md(N, L));
			Ispe = 0.25 * fresnel_schlick(fresnel0, V, H) * geometric_atten(N, V, L, H) * beckmann_distrib(N, H, roughness) / mdp(N, V);
		} else {
			Iamb = vec3(1.0);
			Idif = vec3(max(dot(N, L), 0.0));
			Ispe = vec3((dot(N, L) > 0.0) ? pow(max(dot(N, H), 0.0), material.shininess) : 0.0);
		}

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
	vec3 mat_specular = pbr ? vec3(1.0) : material.specular;

	// final color
	vec3 final_light = mat_ambient * amb + mat_diffuse * dif + material.specular * spe;
	final_color = vec4(final_light, material.alpha);
}