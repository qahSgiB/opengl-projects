#include "firework.hpp"

#include "math_util.hpp"

#include "ubo_impl.hpp"

#include <glm/gtc/constants.hpp>

#include <random>



//  ===============================================  FireworkParams  ===============================================

FireworkParams::FireworkParams() = default;
FireworkParams::FireworkParams(unsigned int particle_count, glm::vec3 pos, glm::vec3 vel, float explosion_force, float explosion_force_variance, float particle_size_base, float rocket_size_mult, glm::vec3 color, float hue_variance, float saturation_variance, float flying1_duration, float explosion_duration, float flying2_duration, float fade_delay, float fade_start_variance, float fade_end_variance, float fade_size_mult, float blink_delay, float blink_freq, float blink_start_variance, float blink_freq_variance, float blink_size_mult)
    : particle_count(particle_count)
    , pos(pos), vel(vel), explosion_force(explosion_force), explosion_force_variance(explosion_force_variance)
    , particle_size_base(particle_size_base), rocket_size_mult(rocket_size_mult)
    , color(color), hue_variance(hue_variance), saturation_variance(saturation_variance)
    , flying1_duration(flying1_duration), explosion_duration(explosion_duration), flying2_duration(flying2_duration)
    , fade_delay(fade_delay), fade_start_variance(fade_start_variance), fade_end_variance(fade_end_variance), fade_size_mult(fade_size_mult)
    , blink_delay(blink_delay), blink_freq(blink_freq), blink_start_variance(blink_start_variance), blink_freq_variance(blink_freq_variance), blink_size_mult(blink_size_mult) {}

void FireworkParams::set_particle_count(unsigned int particle_count_)
{
    particle_count = particle_count_;
}

void FireworkParams::set_physics(glm::vec3 pos_, glm::vec3 vel_, float explosion_force_, float explosion_force_variance_)
{
    pos = pos_;
    vel = vel_;
    explosion_force = explosion_force_;
    explosion_force_variance = explosion_force_variance_;
}

void FireworkParams::set_sizing(float particle_size_base_, float rocket_size_mult_)
{
    particle_size_base = particle_size_base_;
    rocket_size_mult = rocket_size_mult_;
}

void FireworkParams::set_color(glm::vec3 color_, float hue_variance_, float saturation_variance_)
{
    color = color_;
    hue_variance = hue_variance_;
    saturation_variance = saturation_variance_;
}

void FireworkParams::set_timing(float flying1_duration_, float explosion_duration_, float flying2_duration_)
{
    flying1_duration = flying1_duration_;
    explosion_duration = explosion_duration_;
    flying2_duration = flying2_duration_;
}

void FireworkParams::set_fading(float fade_delay_, float fade_start_variance_, float fade_end_variance_, float fade_size_mult_)
{
    fade_delay = fade_delay_;
    fade_start_variance = fade_start_variance_;
    fade_end_variance = fade_end_variance_;
    fade_size_mult = fade_size_mult_;
}

void FireworkParams::set_blinking(float blink_delay_, float blink_freq_, float blink_start_variance_, float blink_freq_variance_, float blink_size_mult_)
{
    blink_delay = blink_delay_;
    blink_freq = blink_freq_;
    blink_start_variance = blink_start_variance_;
    blink_freq_variance = blink_freq_variance_;
    blink_size_mult = blink_size_mult_;
}

float FireworkParams::get_explosion_time() const
{
    return flying1_duration;
}

float FireworkParams::get_flying2_time() const
{
    return flying1_duration + explosion_duration;
}

float FireworkParams::get_end_time() const
{
    return flying1_duration + explosion_duration + flying2_duration;
}

float FireworkParams::get_fade_start() const
{
    return get_flying2_time() + fade_delay * flying2_duration;
}

float FireworkParams::get_blink_start() const
{
    return get_flying2_time() + blink_delay * flying2_duration;
}


//  ===============================================  FireworkParams - randomized creation  ===============================================

FireworkParams FireworkParams::create_default(const FireworkRandomizationParams& fr)
{
    FireworkParams params;
    // params.set_color(glm::vec3(0.75f), 0.33333f);
    params.set_fading(0.3f, 0.25f, 0.3f, 0.5f);
    params.set_blinking(0.6f, 100.f, 0.5f, 0.8f, 0.45f);

    params.set_particle_count((fr.particle_count_min + fr.particle_count_max) / 2);

    glm::vec3 pos = 0.5f * (fr.start_pos_min + fr.start_pos_max);
    glm::vec3 vel = fr.up * fr.vel_size_base;
    params.set_physics(pos, vel, fr.explosion_force_base, fr.explosion_force_variance_base);

    params.set_sizing(fr.particle_size_base_base, fr.rocket_size_mult);

    glm::vec3 color = hsv_to_rgb(glm::vec3(fr.hue_base, fr.saturation_base, 1.0f));
    params.set_color(color, fr.hue_variance_base, fr.saturation_variance_base);

    params.set_timing(fr.flying1_duration_base, fr.explosion_duration_base, fr.flying2_duration_base);
    // params.set_timing(750.0f, 70.0f, 1100.0f);

    return params;
}

FireworkParams FireworkParams::create_random(const FireworkRandomizationParams& fr)
{
    // physics - position
    float x = linmap01(fr.start_pos_min.x, fr.start_pos_max.x, random01());
    float y = linmap01(fr.start_pos_min.y, fr.start_pos_max.y, random01());
    float z = linmap01(fr.start_pos_min.z, fr.start_pos_max.z, random01());

    glm::vec3 pos(x, y, z);

    return create_random_at(fr, pos);
}

FireworkParams FireworkParams::create_random_at(const FireworkRandomizationParams& fr, glm::vec3 pos)
{
    FireworkParams params;

    params.set_timing(750.0f, 70.0f, 1100.0f);
    params.set_fading(0.3f, 0.25f, 0.3f, 0.5f);
    params.set_blinking(0.6f, 100.f, 0.5f, 0.8f, 0.45f);

    // particle count
    size_t particle_count = std::uniform_int_distribution(fr.particle_count_min, fr.particle_count_max)(rnd());
    params.set_particle_count(particle_count);

    // physics - velocity
    glm::vec3 up = fr.up;
    glm::vec3 a = (up.x == 0.0f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(up.y, -up.x, 0.0f);
    glm::vec3 b = glm::vec3(a.y * up.z - a.z * up.y, - a.x * up.z + a.z * up.x, a.x * up.y - a.y * up.x);

    a = glm::normalize(a);
    b = glm::normalize(b);

    float max_radius = glm::length(up) * glm::sin(fr.max_angle);
    float x_rnd = random01();
    x_rnd = glm::pow(x_rnd, 2.0f);
    float radius = glm::sqrt(x_rnd) * max_radius;
    float angle = linmap01(0.0f, 2.0f * glm::pi<float>(), random01());
    glm::vec2 dir(glm::cos(angle) * radius, glm::sin(angle) * radius);

    float vel_size_variance = fr.vel_size_variance;
    float vel_size = fr.vel_size_base * (1.0f + linmap01v(vel_size_variance, random01()));
    glm::vec3 vel = glm::normalize(up + a * dir.x + b * dir.y) * vel_size;

    // physics - explosion force
    float explosion_force = fr.explosion_force_base * (1.0f + linmap01v(fr.explosion_force_variance, random01()));
    float explosion_force_variance = fr.explosion_force_variance_base * (1.0f + linmap01v(fr.explosion_force_variance_variance, random01()));

    params.set_physics(pos, vel, explosion_force, explosion_force_variance);

    // sizing
    float particle_size_base = fr.particle_size_base_base * (1.0f + linmap01v(fr.particle_size_base_variance, random01()));

    params.set_sizing(particle_size_base, fr.rocket_size_mult);

    // color
    float hue = glm::mod(fr.hue_base + linmap01v(fr.hue_range * 0.5, random01()), 1.0f);

    float sat_min = glm::max(fr.saturation_base - fr.saturation_range * 0.5f, 0.0f);
    float sat_max = glm::min(fr.saturation_base + fr.saturation_range * 0.5f, 1.0f);
    float sat = linmap01(sat_min, sat_max, random01());

    glm::vec3 color = hsv_to_rgb(glm::vec3(hue, sat, 1.0f));

    float hue_variance = fr.hue_variance_base * (1.0f + linmap01v(fr.hue_variance_variance, random01()));
    float saturation_variance = fr.saturation_variance_base * (1.0f + linmap01v(fr.saturation_variance_variance, random01()));

    params.set_color(color, hue_variance, saturation_variance);

    // timing
    float flying1_duration = fr.flying1_duration_base * (1.0f + linmap01v(fr.flying1_duration_variance, random01()));
    float explosion_duration = fr.explosion_duration_base * (1.0f + linmap01v(fr.explosion_duration_variance, random01()));
    float flying2_duration = fr.flying2_duration_base * (1.0f + linmap01v(fr.flying2_duration_variance, random01()));

    params.set_timing(flying1_duration, explosion_duration, flying2_duration);

    return params;
}


//  ===============================================  FireworkParamsGpu  ===============================================

FireworkParamsGpu::FireworkParamsGpu() = default;

FireworkParamsGpu::FireworkParamsGpu(const FireworkParams& params) :
    particle_count(params.particle_count),
    explosion_force(params.explosion_force),
    explosion_force_variance(params.explosion_force_variance),
    particle_size_base(params.particle_size_base),
    rocket_size_mult(params.rocket_size_mult),
    hue_variance(params.hue_variance),
    saturation_variance(params.saturation_variance),
    end_time(params.get_end_time()),
    fade_start(params.get_fade_start()),
    fade_start_variance(params.fade_start_variance),
    fade_end_variance(params.fade_end_variance),
    fade_size_mult(params.fade_size_mult),
    blink_start(params.get_blink_start()),
    blink_freq(params.blink_freq),
    blink_start_variance(params.blink_start_variance),
    blink_freq_variance(params.blink_freq_variance),
    blink_size_mult(params.blink_size_mult) {}


FireworkParamsGpuUBO::FireworkParamsGpuUBO() : UBO<FireworkParamsGpu>(OpenGLUtils::get_opengl_version() >= 4.5f ? GL_DYNAMIC_STORAGE_BIT : GL_DYNAMIC_DRAW, GL_SHADER_STORAGE_BUFFER) {}

void FireworkParamsGpuUBO::set_state(const FireworkParams& params)
{
    data[0] = FireworkParamsGpu(params);
}


//  ===============================================  FireworkState  ===============================================

FireworkState::FireworkState() = default;

FireworkState::FireworkState(const FireworkParams& params) :
    particle_count(params.particle_count),
    explosion_time(params.get_explosion_time()),
    flying2_time(params.get_flying2_time()),
    end_time(params.get_end_time()),
    color(params.color),
    fade_start(params.get_fade_start()),
    fade_size_mult(params.fade_size_mult),
    alive_time(0.0f),
    stage(FireworkStage::FLYING1),
    avg_pos(params.pos),
    avg_vel(params.vel),
    avg_acc(0.0f, 0.0f, 0.0f) {}


//  ===============================================  Firework  ===============================================

Firework::Firework(size_t max_particle_count) : max_particle_count(max_particle_count)
{
    active = false;

    glCreateBuffers(1, &vbo_pos);
    glNamedBufferStorage(vbo_pos, 4 * sizeof(float) * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &vbo_vel);
    glNamedBufferStorage(vbo_vel, 4 * sizeof(float) * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &vbo_acc);
    glNamedBufferStorage(vbo_acc, 4 * sizeof(float) * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &vbo_color);
    glNamedBufferStorage(vbo_color, 4 * sizeof(float) * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &vbo_fade_timing);
    glNamedBufferStorage(vbo_fade_timing, 2 * sizeof(float) * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &vbo_blink_timing);
    glNamedBufferStorage(vbo_blink_timing, 2 * sizeof(float) * max_particle_count, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo_pos, 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, false, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glVertexArrayVertexBuffer(vao, 1, vbo_color, 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 4, GL_FLOAT, false, 0);
    glVertexArrayAttribBinding(vao, 1, 1);

    glVertexArrayVertexBuffer(vao, 2, vbo_fade_timing, 0, 2 * sizeof(float));
    glEnableVertexArrayAttrib(vao, 2);
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, false, 0);
    glVertexArrayAttribBinding(vao, 2, 2);

    glVertexArrayVertexBuffer(vao, 3, vbo_blink_timing, 0, 2 * sizeof(float));
    glEnableVertexArrayAttrib(vao, 3);
    glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, false, 0);
    glVertexArrayAttribBinding(vao, 3, 3);
}

void Firework::activate(FireworkParams params)
{
    params.particle_count = std::min(params.particle_count, max_particle_count);

    active = true;

    state = FireworkState(params);

    params_gpu.set_state(params);
    params_gpu.update_opengl_data();

    glm::vec4 pos4(params.pos, 1.0f);
    glNamedBufferSubData(vbo_pos, 0, 4 * sizeof(float), &pos4);

    glm::vec4 vel4(params.vel, 0.0f);
    glNamedBufferSubData(vbo_vel, 0, 4 * sizeof(float), &vel4);

    glm::vec4 color4(params.color, 0.0f);
    glNamedBufferSubData(vbo_color, 0, 4 * sizeof(float), &color4);
}

void Firework::deactivate()
{
    active = false;
}

void Firework::update(float delta, float gravity)
{
    if (!active) {
        return;
    }

    state.alive_time += delta;

    state.last_stage = state.stage;

    if (state.alive_time > state.end_time) {
        deactivate();
        return;
    } else if (state.alive_time > state.flying2_time) {
        state.stage = FireworkStage::FLYING2;
    } else if (state.alive_time > state.explosion_time) {
        state.stage = FireworkStage::EXPLOSION;
    } else {
        state.stage = FireworkStage::FLYING1;
    }

    // update
    glm::vec3 g(0.0f, -gravity, 0.0f);
    glm::vec3 v = state.avg_vel;
    glm::vec3 a = state.avg_acc + g;

    state.avg_pos += v * delta + 0.5f * a * delta * delta;
    state.avg_vel += a * delta;
}

void Firework::update_gpu(const ShaderProgram& compute_program, unsigned int local_size)
{
    if (!active) {
        return;
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_pos);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_vel);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_acc);
    
    if (state.last_stage == FireworkStage::FLYING1 && state.stage == FireworkStage::EXPLOSION) { // [todo] is this optimization?
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo_color);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, vbo_fade_timing);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, vbo_blink_timing);
    }

    params_gpu.bind_buffer_base(6);

    compute_program.uniform(3, static_cast<unsigned int>(state.stage));
    compute_program.uniform(4, static_cast<unsigned int>(state.last_stage));

    glDispatchCompute((state.particle_count - 1) / local_size + 1, 1, 1);
}

void Firework::render(const ShaderProgram& program) const
{
    if (!active) {
        return;
    }

    params_gpu.bind_buffer_base(1);

    program.uniform(0, static_cast<unsigned int>(state.stage));
    program.uniform(1, state.alive_time);
    
    glBindVertexArray(vao);
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glDrawArrays(GL_POINTS, 0, state.stage == FireworkStage::FLYING1 ? 1 : state.particle_count);
}

std::optional<PhongLightData> Firework::generate_light() const
{
    if (!active) {
        return std::nullopt;
    }

    float fade_mult = state.alive_time < state.fade_start ? 1.0f : 1.0f - glm::pow(1.0f - state.fade_size_mult, 1.5f) * (state.alive_time - state.fade_start) / (state.end_time - state.fade_start);
    float count_mult = state.stage == FireworkStage::FLYING1 ? 0.5f : (state.stage == FireworkStage::EXPLOSION ? 0.5f + 1.0f * (state.alive_time - state.explosion_time) / (state.flying2_time - state.explosion_time) : 1.5f);
    return PhongLightData::CreatePointLight(state.avg_pos, glm::vec3(0.0f), count_mult * fade_mult * state.color, glm::vec3(0.0f), 1.0f, 0.0f, 0.0f);
}