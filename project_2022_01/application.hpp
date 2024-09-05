#pragma once

#include "program.hpp"
#include "camera_ubo.hpp"
#include "light_ubo.hpp"
#include "pv227_application.hpp"
#include "scene_object.hpp"

#include "src/firework.hpp"
#include "src/ubo_vector.hpp"

#include <optional>
#include <random>



class Application : public PV227Application
{
protected:
    // gpu seed distribution
    std::uniform_real_distribution<float> hash31_seed_dis;

    // scene objects
    glm::vec2 lake_size;
    glm::vec2 lake_outside_offset;

    SceneObject lake_object;
    SceneObject outer_terrain_object;
    SceneObject castel_base;
    SceneObject castle_object;

    PhongLightsUBOVector phong_lights_bo;

    // camera
    CameraUBO normal_camera_ubo;

    // hdr mapping
    bool use_hdr_mapping;

    float exposure;
    float gamma;

    GLuint hdr_fbo;
    GLuint hdr_fbo_color_texture;
    GLuint hdr_fbo_depth_texture;

    ShaderProgram hdr_to_ldr_program;

    // physics
    float time_multiplier;
    float elapsed_time_m;

    float gravity;

    // mirror (lake reflection)
    bool use_mirror;
    float mirror_factor;
    float mirror_distortion;

    size_t mirror_texture_size;

    GLuint mirror_fbo;
    GLuint mirror_fbo_color_texture;
    GLuint mirror_fbo_depth_texture;

    CameraUBO mirror_camera_ubo;
    
    float mirror_clip_distance;

    // fireworks
    FireworkRandomizationParams firework_randomization;

    size_t fireworks_max_count;
    size_t firework_max_particle_count;
    
    std::vector<Firework> fireworks;

    PhongLightsUBOVector firework_lights;

    ShaderProgram update_firework_program;

    GLuint particle_texture;
    ShaderProgram particle_textured_program;

    // fireworks spawning (user input)
    bool spawn_default;
    bool spawn_random;
    bool spawn_random_at;

    glm::vec3 spawn_random_at_pos;

    bool auto_spawn_pause;
    float auto_spawn_delay;
    float auto_spawn_delay_variance;
    float auto_spawn_delta;

    // mouse box
    SceneObject mouse_box;

    float mouse_plane_y;

    // gui
    bool show_main_menu;
    bool show_fireworks_menu1;
    bool show_fireworks_menu2;

public:
    Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

    virtual ~Application();

    // shaders
    void compile_shaders() override;

    // init
    void prepare_scene();
    void prepare_cameras();

    void prepare_hdr();
    void prepare_mirror();
    void prepare_fireworks();

    // window resizing
    void on_resize_cameras();

    void on_resize_hdr();

    // settings reset
    void reset_global_config();
    void reset_fireworks_config();
    void reset_hdr_config();

    // update
    void update(float delta) override;
    void update_fireworks(float delta);

    bool try_spawn_firework(const FireworkParams& params);

    void update_firework_lights();

    void update_cameras();

    // render
    void render() override;

    void render_hdr_to_ldr();

    void render_mirror();
    void render_from_normal_camera();

    void render_scene_with_lights(bool from_mirror);
    void render_scene(const ShaderProgram& program, bool from_mirror);
    void render_object(const SceneObject& object, const ShaderProgram& program);

    void render_lake(const ShaderProgram& program);
    void render_fireworks(bool from_mirror);
    void render_mouse_box(const ShaderProgram& program);

    // gui
    void render_ui() override;
    void render_main_menu();
    void render_fireworks_menu1();
    void render_fireworks_menu2();

    // events
    void on_resize(int width, int height) override;
    void on_mouse_button(int button, int action, int mods) override;
    void on_key_pressed(int key, int scancode, int action, int mods) override;

    // mouse box
    std::optional<glm::vec3> get_mouse_box_pos_ws();
};
