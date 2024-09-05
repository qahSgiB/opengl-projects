// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
//
// Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#pragma once
#include "camera_ubo.hpp"
#include "light_ubo.hpp"
#include "pv227_application.hpp"
#include "scene_object.hpp"



class Application : public PV227Application {
protected:
    // camera
    glm::mat4 projection_matrix;
    CameraUBO camera_ubo;

    // snow view
    CameraUBO top_camera_ubo;

    size_t snow_view_tex_size;

    // snow accumulation
    bool clear_snow_accum;
    float snow_accum_vel_mult;

    bool snow_accum_output_a;

    GLuint snow_accum_tex_a;
    GLuint snow_accum_tex_b;

    GLuint snow_accum_fbo_a;
    GLuint snow_accum_fbo_b;

    ShaderProgram snow_accum_update_program;    

    // snow accumulation - blur
    float snow_accum_blur_radius;

    ShaderProgram snow_accum_blur_program;

    // snow accumulation - shadow
    GLuint snow_shadow_tex;
    GLuint snow_shadow_fbo;

    bool do_update_snow_shadow;

    // broom
    bool broom_permanent;

    glm::vec2 broom_center;

    SceneObject broom_object;
    GLuint broom_tex;

    // broom - position texture
    GLuint broom_pos_tex;
    GLuint broom_pos_fbo;
    ShaderProgram broom_pos_program;

    // snow plane
    bool show_snow_plane;

    float snow_height_max;
    float snow_plane_tess_factor;

    SceneObject snow_plane_object;

    PhongMaterialUBO snow_material_ubo;

    GLuint snow_albedo_tex;
    GLuint snow_normal_tex;
    GLuint snow_height_tex;
    GLuint snow_roughness_tex;

    ShaderProgram snow_plane_program;

    // snow plane - base
    GLuint snow_plane_base_tex;
    GLuint snow_plane_base_fbo;

    bool do_update_snow_plane_base;

    // snow particles
    int snow_particles_count;
    int snow_particles_count_target;

    GLuint snow_particles_pos_buffer;
    GLuint snow_particles_vao;

    GLuint snow_particle_tex;

    ShaderProgram snow_particles_program;

    // scene
    SceneObject outer_terrain_object;
    SceneObject lake_object;
    SceneObject castel_base_object;
    SceneObject castle_object;

    PhongMaterialUBO brown_material_ubo;
    PhongMaterialUBO castle_material_ubo;

    GLuint ice_albedo_tex;

    // lights
    PhongLightsUBO phong_lights_ubo;

    float light_angle;

    SceneObject light_object;

    // general shadow program
    ShaderProgram shadow_program;

    // debug
    ShaderProgram display_texture_program;

    // general settings
    bool use_snow;
    bool wireframe;
    bool pbr;

    bool do_reset_settings;


public:
    Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

    ~Application() override;

    // init
    void compile_shaders() override;

    void prepare_snow_view();
    void prepare_snow_accum();
    void prepare_snow_shadowing();
    void prepare_broom();
    void prepare_snow_plane();
    void prepare_snow_plane_base();
    void prepare_snow_particles();
    void prepare_scene();
    void prepare_lights();
    void prepare_camera();

    // reset + reload
    void reset_settings();

    void reload_snow_particles(bool first = false);

    // update
    void update(float delta) override;
    void update_snow_accum(float delta);
    void update_snow_shadow();
    void update_snow_plane_base();

    void update_broom_location();

    // render
    void render() override;

    void render_object(const ShaderProgram& program, const SceneObject& object, float uv_multiplier = 1.0f, bool apply_snow = true);
    void render_snow_plane();
    void render_snow_particles();

    void render_texture(GLuint texture);

    void bind_object(const ShaderProgram& program, const SceneObject& object);

    // render gui
    void render_ui() override;

    // input
    void on_resize(int width, int height) override;
    void on_mouse_move(double x, double y) override;
};
