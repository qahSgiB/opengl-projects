#pragma once

#include "program.hpp"
#include "ubo.hpp"
#include "light_ubo.hpp"

#include <glm/glm.hpp>

#include <optional>



// randomization parameters for creating random FireworkParams
// random FireworkParams can be created using these functions:
//     FireworkParams::create_default
//     FireworkParams::create_random
//     FireworkParams::create_random_at
struct FireworkRandomizationParams
{
    // particle count
    size_t particle_count_min;
    size_t particle_count_max;

    // physics
    glm::vec3 start_pos_min;
    glm::vec3 start_pos_max;

    glm::vec3 up;
    float max_angle;
    float vel_size_base;
    float vel_size_variance;
    float explosion_force_base;
    float explosion_force_variance;
    float explosion_force_variance_base;
    float explosion_force_variance_variance;

    // sizing
    float particle_size_base_base;
    float particle_size_base_variance;
    float rocket_size_mult; // not randomized

    // color
    float hue_base;
    float hue_range;
    float saturation_base;
    float saturation_range;
    float hue_variance_base;
    float hue_variance_variance;
    float saturation_variance_base;
    float saturation_variance_variance;

    // timing
    float flying1_duration_base;
    float flying1_duration_variance;
    float explosion_duration_base;
    float explosion_duration_variance;
    float flying2_duration_base;
    float flying2_duration_variance;
};


// parameters for firework creation
struct FireworkParams
{
    size_t particle_count;

    // physics
    glm::vec3 pos;
    glm::vec3 vel;
    float explosion_force;
    float explosion_force_variance;

    // sizing
    float particle_size_base;
    float rocket_size_mult;

    // color
    glm::vec3 color;
    float hue_variance;
    float saturation_variance;

    // timing
    float flying1_duration;
    float explosion_duration;
    float flying2_duration;

    // fading
    float fade_delay;
    float fade_start_variance;
    float fade_end_variance;
    float fade_size_mult;

    // blinking
    float blink_delay;
    float blink_freq;
    float blink_start_variance;
    float blink_freq_variance;
    float blink_size_mult;


    FireworkParams();
    FireworkParams(unsigned int particle_count, glm::vec3 pos, glm::vec3 vel, float explosion_force, float explosion_force_variance, float particle_size_base, float rocket_size_mult, glm::vec3 color, float hue_variance, float saturation_variance, float flying1_duration, float explosion_duration, float flying2_duration, float fade_delay, float fade_start_variance, float fade_end_variance, float fade_size_mult, float blink_delay, float blink_freq, float blink_start_variance, float blink_freq_variance, float blink_size_mult);


    void set_particle_count(unsigned int particle_count_);
    void set_physics(glm::vec3 pos_, glm::vec3 vel_, float explosion_force_, float explosion_force_variance_);
    void set_sizing(float particle_size_base_, float rocket_size_mult_);
    void set_color(glm::vec3 color_, float hue_variance_, float saturation_variance_);
    void set_timing(float flying1_duration_, float explosion_duration_, float flying2_duration_);
    void set_fading(float fade_delay_, float fade_start_variance_, float fade_end_variance_, float fade_size_mult_);
    void set_blinking(float blink_delay_, float blink_freq_, float blink_start_variance_, float blink_freq_variance_, float blink_size_mult_);

    float get_explosion_time() const;
    float get_flying2_time() const;
    float get_end_time() const;
    
    float get_fade_start() const;
    float get_blink_start() const;


    static FireworkParams create_default(const FireworkRandomizationParams& fr);
    static FireworkParams create_random(const FireworkRandomizationParams& fr);
    static FireworkParams create_random_at(const FireworkRandomizationParams& fr, glm::vec3 pos);
};


// firework params on gpu
struct FireworkParamsGpu
{
    unsigned int particle_count;

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

    FireworkParamsGpu();
    FireworkParamsGpu(const FireworkParams& params);
};

class FireworkParamsGpuUBO : public UBO<FireworkParamsGpu>
{
    // check correct layout for gpu
    static_assert(offsetof(FireworkParamsGpu, particle_count) == 0, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, explosion_force) == 4, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, explosion_force_variance) == 8, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, particle_size_base) == 12, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, rocket_size_mult) == 16, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, hue_variance) == 20, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, saturation_variance) == 24, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, end_time) == 28, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, fade_start) == 32, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, fade_start_variance) == 36, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, fade_end_variance) == 40, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, fade_size_mult) == 44, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, blink_start) == 48, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, blink_freq) == 52, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, blink_start_variance) == 56, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, blink_freq_variance) == 60, "incorrect FireworkParamsGpu layout");
    static_assert(offsetof(FireworkParamsGpu, blink_size_mult) == 64, "incorrect FireworkParamsGpu layout");
    static_assert(sizeof(FireworkParamsGpu) == 68, "incorrect FireworkParamsGpu layout");

public:
    FireworkParamsGpuUBO();

    void set_state(const FireworkParams& params);
};


enum class FireworkStage : unsigned int {
    FLYING1 = 0,
    EXPLOSION = 1,
    FLYING2 = 2,
};


// firework state on cpu
struct FireworkState
{
    unsigned int particle_count;

    float explosion_force;

    float explosion_time;
    float flying2_time;
    float end_time;

    glm::vec3 color;

    float fade_start;
    float fade_size_mult;

    float alive_time;
    FireworkStage stage;
    FireworkStage last_stage;

    glm::vec3 avg_pos;
    glm::vec3 avg_vel;
    glm::vec3 avg_acc;


    FireworkState();
    FireworkState(const FireworkParams& params);
};


// structure holding one firework
// can be inactive
// contains and manages gpu buffers for one firework
struct Firework
{
    size_t max_particle_count;
    bool active;

    // state
    FireworkParamsGpuUBO params_gpu;
    FireworkState state;

    // buffers (firework gpu state)
    GLuint vbo_pos;
    GLuint vbo_vel;
    GLuint vbo_acc;
    GLuint vbo_color;
    GLuint vbo_fade_timing;
    GLuint vbo_blink_timing;

    GLuint vao;


    Firework(size_t max_particle_count);

    void activate(FireworkParams params);
    void deactivate();

    void update(float delta, float gravity);
    void update_gpu(const ShaderProgram& compute_program, unsigned int local_size);

    void render(const ShaderProgram& program) const;

    std::optional<PhongLightData> generate_light() const;
};