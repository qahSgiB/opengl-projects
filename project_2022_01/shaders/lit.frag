#version 450 core



in VertexData
{
    vec3 position_ws;
    vec3 normal_ws;
    vec2 tex_coord;
} in_data;



layout(std140, binding = 0) uniform CameraBuffer
{
    mat4 projection;
    mat4 projection_inv;
    mat4 view;
    mat4 view_inv;
    mat3 view_it; // inverse + transpose of view matrix
    vec3 eye_position;
};

struct PhongLight
{
    vec4 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 spot_direction;   // The direction of the spot light, irrelevant for point lights and directional lights.
    float spot_exponent;   // The spot exponent of the spot light, irrelevant for point lights and directional lights.
    float spot_cos_cutoff; // The cosine of the spot light's cutoff angle, -1 point lights, irrelevant for directional lights.
    float atten_constant;  // The constant attenuation of spot lights and point lights, irrelevant for directional lights. For no attenuation, set this to 1.
    float atten_linear;    // The linear attenuation of spot lights and point lights, irrelevant for directional lights.  For no attenuation, set this to 0.
    float atten_quadratic; // The quadratic attenuation of spot lights and point lights, irrelevant for directional lights. For no attenuation, set this to 0.
};

layout(std430, binding = 2) buffer PhongLightsBuffer
{
    uint count;
    PhongLight data[5];
} lights;

layout(std140, binding = 3) uniform PhongMaterialBuffer
{
    vec3 ambient;
    vec3 diffuse;
    float alpha;
    vec3 specular;
    float shininess;
} material;

layout(std430, binding = 4) buffer FireworkLights
{
    uint count;
    PhongLight data[5];
} firework_lights;


layout(location = 0) uniform bool has_texture;
layout(location = 1) uniform float texture_factor;
layout(location = 2) uniform bool apply_texture_distorion;
layout(location = 3) uniform float texture_distorion_level;

layout(location = 4) uniform float time; // in seconds

layout(binding = 0) uniform sampler2D material_diffuse_texture;



layout(location = 0) out vec4 final_color;



void process_light(PhongLight light, vec3 N, vec3 V, inout vec3 amb, inout vec3 dif, inout vec3 spe)
{
    vec3 L_not_normalized = light.position.xyz - in_data.position_ws * light.position.w;
    vec3 L = normalize(L_not_normalized);
    vec3 H = normalize(L + V);

    // phong factors
    float Iamb = 1.0;
    float Idif = max(dot(N, L), 0.0);
    float Ispe = (Idif > 0.0) ? pow(max(dot(N, H), 0.0), material.shininess) : 0.0;

    // spot light
    float spot_cos_cutoff = light.spot_cos_cutoff;
    if (spot_cos_cutoff != -1.0) {
        float spot_cos_angle = dot(-L, light.spot_direction);

        float spot_factor;
        if (spot_cos_angle > spot_cos_cutoff) {
            spot_factor = pow((spot_cos_angle - spot_cos_cutoff) / (1.0f - spot_cos_cutoff), light.spot_exponent);
        } else {
            spot_factor = 0.0;
        }

        Idif *= spot_factor;
        Ispe *= spot_factor;
    }

    // attenuation
    if (light.position.w != 0.0) {
        float distance_from_light = length(L_not_normalized);
        float atten_factor =
            light.atten_constant +
            light.atten_linear * distance_from_light + 
            light.atten_quadratic * distance_from_light * distance_from_light;
        atten_factor = 1.0 / atten_factor;

        Iamb *= atten_factor;
        Idif *= atten_factor;
        Ispe *= atten_factor;
    }

    // add
    amb += Iamb * light.ambient;
    dif += Idif * light.diffuse;
    spe += Ispe * light.specular;
}



void main()
{
    // lighting
    vec3 N = normalize(in_data.normal_ws);
    vec3 V = normalize(eye_position - in_data.position_ws);
    
    vec3 amb = vec3(0.0);
    vec3 dif = vec3(0.0);
    vec3 spe = vec3(0.0);

    for (int i = 0; i < lights.count; i++) {
        process_light(lights.data[i], N , V, amb, dif, spe);
    }

    for (int i = 0; i < firework_lights.count; i++) {
        process_light(firework_lights.data[i], N , V, amb, dif, spe);
    }

    // material + textures
    vec3 mat_ambient = material.ambient;
    vec3 mat_diffuse = material.diffuse;
    vec3 mat_specular = material.specular;

    if (has_texture && texture_factor > 0.0001f) {
        vec2 uv = in_data.tex_coord;

        if (apply_texture_distorion && texture_distorion_level > 0.0001f) {
            vec2 offset = vec2(0.0f);
            // offset.x = 0.01f * sin((uv.y * 10.0f) * 2.0f * 3.14159f);
            // offset.y = 0.01f * sin((uv.x * 10.0f) * 2.0f * 3.14159f);
            float z = 5000.0f / sin(4.36f * uv.y * uv.x);
            float t = time / 5.0f;

            offset.x = sin((uv.x + z * 0.5f) * 0.005f - t * 20.0f);
            offset.y = cos((z - uv.x * 0.5f) * 0.005f - t * 20.0f);
            offset *= cos(z + uv.x - t * 15.0f) * 1.8f * 200.0f / z;

            uv += texture_distorion_level * offset;
        }

        vec4 tex_color = texture(material_diffuse_texture, uv);

        mat_ambient = mix(material.ambient, tex_color.rgb, texture_factor);
        mat_diffuse = mix(material.diffuse, tex_color.rgb, texture_factor);
        // mat_ambient = mix(material.ambient, tex_color.rgb, texture_factor * tex_color.a);
        // mat_diffuse = mix(material.diffuse, tex_color.rgb, texture_factor * tex_color.a);
        // mat_ambient = material.ambient + tex_color.rgb * texture_factor * tex_color.a;
        // mat_diffuse = material.diffuse + tex_color.rgb * texture_factor * tex_color.a;
    }

    // final color
    vec3 final_light = mat_ambient * amb + mat_diffuse * dif + material.specular * spe;
    final_color = vec4(final_light, 1.0f);
}	