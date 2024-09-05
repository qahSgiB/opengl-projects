#include "application.hpp"
#include "utils.hpp"
#include <map>



Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments) : PV227Application(initial_width, initial_height, arguments)
{
    Application::compile_shaders();

    reset_settings();
    do_reset_settings = false;

    prepare_snow_view();
    prepare_snow_accum();
    prepare_snow_shadowing();
    prepare_broom();
    prepare_snow_plane();
    prepare_snow_plane_base();
    prepare_snow_particles();
    prepare_scene();
    prepare_lights();
    prepare_camera();

    glEnable(GL_CULL_FACE);
}

Application::~Application() {}


//  ===============================================  init  ===============================================
void Application::compile_shaders()
{
    default_unlit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "unlit.frag");
    default_lit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "lit.frag");

    display_texture_program = ShaderProgram(lecture_shaders_path / "fullscreen_quad.vert", lecture_shaders_path / "display_texture.frag");

    snow_accum_update_program = ShaderProgram(lecture_shaders_path / "fullscreen_quad.vert", lecture_shaders_path / "update_snow.frag");
    snow_accum_blur_program = ShaderProgram(lecture_shaders_path / "fullscreen_quad.vert", lecture_shaders_path / "gauss_blur.frag");

    shadow_program = ShaderProgram(lecture_shaders_path / "no_frag.vert", lecture_shaders_path / "no_frag.frag");

    broom_pos_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "1f.frag");

    snow_plane_program = ShaderProgram();
    snow_plane_program.add_tess_control_shader(lecture_shaders_path / "snow_plane.tesc");
    snow_plane_program.add_tess_evaluation_shader(lecture_shaders_path / "snow_plane.tese");
    snow_plane_program.add_vertex_shader(lecture_shaders_path / "snow_plane.vert");
    snow_plane_program.add_fragment_shader(lecture_shaders_path / "snow_plane.frag");
    snow_plane_program.link();

    snow_particles_program = ShaderProgram();
    snow_particles_program.add_vertex_shader(lecture_shaders_path / "snow.vert");
    snow_particles_program.add_geometry_shader(lecture_shaders_path / "snow.geom");
    snow_particles_program.add_fragment_shader(lecture_shaders_path / "snow.frag");
    snow_particles_program.link();

    std::cout << "shaders compiled\n";
}


void Application::prepare_snow_view()
{
    float camera_y = 5.0f;
    top_camera_ubo.set_projection(glm::ortho(-13.0f, 13.0f, -13.0f, 13.0f, 0.1f, camera_y + 0.5f));
    top_camera_ubo.set_view(glm::lookAt(glm::vec3(0.0f, camera_y, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    top_camera_ubo.update_opengl_data();

    snow_view_tex_size = 1024;
}

void Application::prepare_snow_accum()
{
    // snow accumulation textures
    glCreateTextures(GL_TEXTURE_2D, 1, &snow_accum_tex_a);
    glCreateTextures(GL_TEXTURE_2D, 1, &snow_accum_tex_b);

    glTextureStorage2D(snow_accum_tex_a, 1, GL_R32F, snow_view_tex_size, snow_view_tex_size);
    glTextureStorage2D(snow_accum_tex_b, 1, GL_R32F, snow_view_tex_size, snow_view_tex_size);

    TextureUtils::set_texture_2d_parameters(snow_accum_tex_a, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    TextureUtils::set_texture_2d_parameters(snow_accum_tex_b, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

    // snow accumulation framebuffers
    glCreateFramebuffers(1, &snow_accum_fbo_a);
    glCreateFramebuffers(1, &snow_accum_fbo_b);

    glNamedFramebufferTexture(snow_accum_fbo_a, GL_COLOR_ATTACHMENT0, snow_accum_tex_a, 0);
    glNamedFramebufferTexture(snow_accum_fbo_b, GL_COLOR_ATTACHMENT0, snow_accum_tex_b, 0);

    //
    clear_snow_accum = false;
    snow_accum_output_a = true;
}

void Application::prepare_snow_shadowing()
{
    glCreateTextures(GL_TEXTURE_2D, 1, &snow_shadow_tex);
    glTextureStorage2D(snow_shadow_tex, 1, GL_DEPTH_COMPONENT24, snow_view_tex_size, snow_view_tex_size);
    TextureUtils::set_texture_2d_parameters(snow_shadow_tex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

    glCreateFramebuffers(1, &snow_shadow_fbo);
    glNamedFramebufferTexture(snow_shadow_fbo, GL_DEPTH_ATTACHMENT, snow_shadow_tex, 0);

    do_update_snow_shadow = true;
}

void Application::prepare_broom()
{
    broom_center = glm::vec2(0.0f);

    // broom object
    broom_tex = TextureUtils::load_texture_2d(lecture_textures_path / "wood.jpg");
    TextureUtils::set_texture_2d_parameters(broom_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

    Geometry broom = Geometry::from_file(lecture_folder_path / "models/broom.obj");
    broom_object = SceneObject(broom, ModelUBO(), white_material_ubo, broom_tex);

    // broom position
    glCreateTextures(GL_TEXTURE_2D, 1, &broom_pos_tex);
    glTextureStorage2D(broom_pos_tex, 1, GL_R32F, snow_view_tex_size, snow_view_tex_size);
    TextureUtils::set_texture_2d_parameters(broom_pos_tex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

    glCreateFramebuffers(1, &broom_pos_fbo);
    glNamedFramebufferTexture(broom_pos_fbo, GL_COLOR_ATTACHMENT0, broom_pos_tex, 0);
}

void Application::prepare_snow_plane()
{
    snow_albedo_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_albedo.png");
    snow_normal_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_normal.png");
    snow_height_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_height.png");
    snow_roughness_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_roughness.png");

    TextureUtils::set_texture_2d_parameters(snow_albedo_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);
    TextureUtils::set_texture_2d_parameters(snow_normal_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);
    TextureUtils::set_texture_2d_parameters(snow_height_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);
    TextureUtils::set_texture_2d_parameters(snow_roughness_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

    snow_material_ubo.set_material(PhongMaterialData(glm::vec3(0.1f), glm::vec3(0.9f), true, glm::vec3(0.1), 2.0f));
    snow_material_ubo.update_opengl_data();

    Geometry plane = Geometry::from_file(lecture_folder_path / "models/plane.obj");
    snow_plane_object = SceneObject(plane, ModelUBO(glm::scale(glm::vec3(26.f, 1.f, 26.f))), snow_material_ubo, snow_albedo_tex);
}

void Application::prepare_snow_plane_base()
{
    glCreateTextures(GL_TEXTURE_2D, 1, &snow_plane_base_tex);
    glTextureStorage2D(snow_plane_base_tex, 1, GL_DEPTH_COMPONENT24, snow_view_tex_size, snow_view_tex_size);
    TextureUtils::set_texture_2d_parameters(snow_plane_base_tex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

    glCreateFramebuffers(1, &snow_plane_base_fbo);
    glNamedFramebufferTexture(snow_plane_base_fbo, GL_DEPTH_ATTACHMENT, snow_plane_base_tex, 0);

    do_update_snow_plane_base = true;
}

void Application::prepare_snow_particles()
{
    // snow particle positions
    snow_particles_count = 0;
    reload_snow_particles(true);

    // snow particle texture
    snow_particle_tex = TextureUtils::load_texture_2d(lecture_textures_path / "star.png");
    TextureUtils::set_texture_2d_parameters(snow_particle_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}

void Application::prepare_scene()
{
    // base (outer, castle)
    glm::vec3 brown_color = glm::vec3(0.5f, 0.25f, 0.0f);
    brown_material_ubo.set_material(PhongMaterialData(brown_color * 0.1, brown_color * 0.9, true, glm::vec3(0.1), 2.0f));
    brown_material_ubo.update_opengl_data();
    
    outer_terrain_object = SceneObject(cube, ModelUBO(glm::translate(glm::vec3(0.0f, -0.08f, 0.0f)) * glm::scale(glm::vec3(13.0f, 0.1f, 13.0f))), brown_material_ubo);
    castel_base_object = SceneObject(cube, ModelUBO(glm::translate(glm::vec3(0.0f, 0.05f, 0.0f)) * glm::scale(glm::vec3(3.8f, 0.1f, 3.8f))), brown_material_ubo);

    // lake
    ice_albedo_tex = TextureUtils::load_texture_2d(lecture_textures_path / "ice_albedo.png");
    TextureUtils::set_texture_2d_parameters(ice_albedo_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

    lake_object = SceneObject(cube, ModelUBO(glm::translate(glm::vec3(0.0f, -0.05f, 0.0f)) * glm::scale(glm::vec3(12.f, 0.1f, 12.f))), blue_material_ubo, ice_albedo_tex);

    // castle
    glm::vec3 castle_color = glm::vec3(0.95f, 0.75f, 0.55f);
    castle_material_ubo.set_material(PhongMaterialData(castle_color * 0.1, castle_color * 0.9, true, glm::vec3(0.1), 2.0f));
    castle_material_ubo.update_opengl_data();

    Geometry castle = Geometry::from_file(lecture_folder_path / "models/castle.obj");
    castle_object = SceneObject(castle, ModelUBO(glm::translate(glm::vec3(0.0f, 1.06f, 0.0f)) * glm::scale(glm::vec3(7.f, 7.f, 7.f))), castle_material_ubo);
}

void Application::prepare_lights()
{
    phong_lights_ubo.set_global_ambient(glm::vec3(0.0f));

    light_object = SceneObject(sphere, ModelUBO(), white_material_ubo);
}

void Application::prepare_camera()
{
    camera.set_eye_position(glm::radians(-45.f), glm::radians(20.f), 50.f);

    projection_matrix = glm::perspective(glm::radians(45.f), static_cast<float>(this->width) / static_cast<float>(this->height), 0.1f, 5000.0f);
    camera_ubo.set_projection(projection_matrix);

    camera_ubo.update_opengl_data();
}


//  ===============================================  reset + reload  ===============================================
void Application::reset_settings()
{
    use_snow = true;
    snow_accum_vel_mult = 1.0f;
    snow_accum_blur_radius = 3.0f;
    broom_permanent = false;

    show_snow_plane = true;
    snow_height_max = 0.75f;
    snow_plane_tess_factor = 100.0f;
    
    snow_particles_count_target = 2048;

    light_angle = glm::radians(180.0f);

    wireframe = false;
    pbr = true;
}


void Application::reload_snow_particles(bool first)
{
    snow_particles_count = snow_particles_count_target;

    std::vector<glm::vec3> snow_particles_pos(snow_particles_count);

    float rand_max = static_cast<float>(RAND_MAX);

    for (int i = 0; i < snow_particles_count; i++) {
        float r1 = static_cast<float>(rand()) / rand_max;
        float r2 = static_cast<float>(rand()) / rand_max;
        float r3 = static_cast<float>(rand()) / rand_max;

        snow_particles_pos[i] = glm::vec3((r1 - 0.5f) * 26, r2 * 50.0f, (r3 - 0.5f) * 26);
    }

    if (!first) {
        glDeleteBuffers(1, &snow_particles_pos_buffer);
        glDeleteVertexArrays(1, &snow_particles_vao);
    }

    glCreateBuffers(1, &snow_particles_pos_buffer);
    glNamedBufferStorage(snow_particles_pos_buffer, sizeof(float) * 3 * snow_particles_count, snow_particles_pos.data(), 0);

    glCreateVertexArrays(1, &snow_particles_vao);
    glVertexArrayVertexBuffer(snow_particles_vao, Geometry_Base::DEFAULT_POSITION_LOC, snow_particles_pos_buffer, 0, 3 * sizeof(float));

    glEnableVertexArrayAttrib(snow_particles_vao, Geometry_Base::DEFAULT_POSITION_LOC);
    glVertexArrayAttribFormat(snow_particles_vao, Geometry_Base::DEFAULT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(snow_particles_vao, Geometry_Base::DEFAULT_POSITION_LOC, Geometry_Base::DEFAULT_POSITION_LOC);
}

//  ===============================================  update  ===============================================
void Application::update(float delta)
{
    if (do_reset_settings) {
        reset_settings();
    }

    PV227Application::update(delta);

    // camera
    glm::vec3 eye_position = camera.get_eye_position();
    camera_ubo.set_view(glm::lookAt(eye_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    camera_ubo.update_opengl_data();

    // light
    float light_elevation = light_angle / 6.0f;
    glm::vec3 light_position = 15 * glm::vec3(cosf(light_elevation) * sinf(light_angle), sinf(light_elevation), cosf(light_elevation) * cosf(light_angle));;

    light_object.get_model_ubo().set_matrix(glm::translate(light_position) * glm::scale(glm::vec3(0.2f)));
    light_object.get_model_ubo().update_opengl_data();

    phong_lights_ubo.clear();
    phong_lights_ubo.add(PhongLightData::CreatePointLight(light_position, glm::vec3(0.1f), glm::vec3(0.9f), glm::vec3(1.0f)));
    phong_lights_ubo.update_opengl_data();

    // snow
    if (use_snow) {
        if (do_update_snow_shadow) {
            update_snow_shadow();
            do_update_snow_shadow = false;
        }

        if (do_update_snow_plane_base) {
            update_snow_plane_base();
            do_update_snow_plane_base = false;
        }

        update_snow_accum(delta);

        if (snow_particles_count != snow_particles_count_target) {
            reload_snow_particles();
        }
    }
}

void Application::update_snow_accum(float delta)
{
    // broom position texture
    glBindFramebuffer(GL_FRAMEBUFFER, broom_pos_fbo);
    glViewport(0, 0, snow_view_tex_size, snow_view_tex_size);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (broom_permanent && !clear_snow_accum) {
        glClear(0);
    } else {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    broom_pos_program.use();

    top_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);
    broom_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);

    broom_object.get_geometry().bind_vao();
    broom_object.get_geometry().draw();

    glEnable(GL_DEPTH_TEST);

    // snow accumulation
    snow_accum_output_a = !snow_accum_output_a;

    glBindFramebuffer(GL_FRAMEBUFFER, snow_accum_output_a ? snow_accum_fbo_a : snow_accum_fbo_b);
    glViewport(0, 0, snow_view_tex_size, snow_view_tex_size);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    snow_accum_update_program.use();

    size_t snow_particles_count_exp = static_cast<size_t>(log2(snow_particles_count) - 8);
    float snow_accum_vel = (0.04f + 0.05f * snow_particles_count_exp) / 1000.0f; // [todo]

    snow_accum_update_program.uniform(0, snow_accum_vel * snow_accum_vel_mult);
    snow_accum_update_program.uniform(1, clear_snow_accum);
    snow_accum_update_program.uniform(2, delta);

    glBindTextureUnit(0, snow_accum_output_a ? snow_accum_tex_b : snow_accum_tex_a);
    glBindTextureUnit(1, broom_pos_tex);

    glBindVertexArray(empty_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // snow accumulation blur
    snow_accum_output_a = !snow_accum_output_a;

    glBindFramebuffer(GL_FRAMEBUFFER, snow_accum_output_a ? snow_accum_fbo_a : snow_accum_fbo_b);
    glViewport(0, 0, snow_view_tex_size, snow_view_tex_size);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    snow_accum_blur_program.use();

    snow_accum_blur_program.uniform(0, snow_accum_blur_radius);

    glBindTextureUnit(0, snow_accum_output_a ? snow_accum_tex_b : snow_accum_tex_a);

    glBindVertexArray(empty_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void Application::update_snow_shadow()
{
    glBindFramebuffer(GL_FRAMEBUFFER, snow_shadow_fbo);
    glViewport(0, 0, snow_view_tex_size, snow_view_tex_size);

    glEnable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadow_program.use();

    top_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

    outer_terrain_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    outer_terrain_object.get_geometry().bind_vao();
    outer_terrain_object.get_geometry().draw();

    lake_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    lake_object.get_geometry().bind_vao();
    lake_object.get_geometry().draw();

    castel_base_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    castel_base_object.get_geometry().bind_vao();
    castel_base_object.get_geometry().draw();

    castle_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    castle_object.get_geometry().bind_vao();
    castle_object.get_geometry().draw();
}

void Application::update_snow_plane_base()
{
    glBindFramebuffer(GL_FRAMEBUFFER, snow_plane_base_fbo);
    glViewport(0, 0, snow_view_tex_size, snow_view_tex_size);

    glEnable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadow_program.use();

    top_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

    outer_terrain_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    outer_terrain_object.get_geometry().bind_vao();
    outer_terrain_object.get_geometry().draw();

    lake_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    lake_object.get_geometry().bind_vao();
    lake_object.get_geometry().draw();

    castel_base_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    castel_base_object.get_geometry().bind_vao();
    castel_base_object.get_geometry().draw();
}

void Application::update_broom_location()
{
    float clip_z = 0;
    glReadPixels(broom_center.x, broom_center.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &clip_z);

    glm::vec4 clip_space = glm::vec4((broom_center.x / width) * 2.0f - 1.0f, (broom_center.y / height) * 2.0f - 1.0f, clip_z * 2.0 - 1.0, 1.0f);
    glm::vec4 world_space = glm::inverse(camera.get_view_matrix()) * glm::inverse(projection_matrix) * clip_space;
    glm::vec3 position = glm::vec3(world_space)  / world_space.w;

    position.y += 4.0f;

    broom_object.get_model_ubo().set_matrix(glm::translate(position) * glm::scale(glm::vec3(8.f)));
    broom_object.get_model_ubo().update_opengl_data();
}


//  ===============================================  render  ===============================================
void Application::render()
{
    // timing - start
    glBeginQuery(GL_TIME_ELAPSED, render_time_query);

    // fbo
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // uniforms
    phong_lights_ubo.bind_buffer_base(PhongLightsUBO::DEFAULT_LIGHTS_BINDING);
    camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);
    top_camera_ubo.bind_buffer_base(4);

    glBindTextureUnit(1, snow_accum_output_a ? snow_accum_tex_a : snow_accum_tex_b);
    glBindTextureUnit(2, snow_shadow_tex);

    // render objects
    render_object(default_lit_program, outer_terrain_object);
    render_object(default_lit_program, lake_object, 10);
    render_object(default_lit_program, castel_base_object);
    render_object(default_lit_program, castle_object);

    if (use_snow && show_snow_plane) {
        render_snow_plane();
    }

    update_broom_location();
    render_object(default_lit_program, broom_object, 1.0f, false);

    render_object(default_unlit_program, light_object, 1.0f, false);

    if (use_snow) {
        render_snow_particles();
    }

    // timing - end
    glEndQuery(GL_TIME_ELAPSED);

    // finish opengl
    glFinish();

    // timing
    GLuint64 render_time;
    glGetQueryObjectui64v(render_time_query, GL_QUERY_RESULT, &render_time);
    fps_gpu = 1000.f / (static_cast<float>(render_time) * 1e-6f);
}

void Application::render_object(const ShaderProgram& program, const SceneObject& object, float uv_multiplier, bool apply_snow)
{
    program.use();
    program.uniform(1, uv_multiplier);
    program.uniform(2, apply_snow && use_snow);
    bind_object(program, object);    

    object.get_geometry().draw();
}

void Application::render_snow_plane()
{
    glDisable(GL_CULL_FACE);

    snow_plane_program.use();
    
    top_camera_ubo.bind_buffer_base(5);

    snow_plane_program.uniform(1, 1.0f);
    snow_plane_program.uniform(2, snow_height_max);
    snow_plane_program.uniform(3, snow_plane_tess_factor);
    snow_plane_program.uniform(4, pbr);
    
    glBindTextureUnit(3, snow_plane_base_tex);
    glBindTextureUnit(4, snow_height_tex);
    glBindTextureUnit(5, snow_normal_tex);
    glBindTextureUnit(6, snow_roughness_tex);
    
    bind_object(snow_plane_program, snow_plane_object);

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    if (snow_plane_object.get_geometry().draw_elements_count > 0) {
        glDrawElements(GL_PATCHES, snow_plane_object.get_geometry().draw_elements_count, GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(GL_PATCHES, 0, snow_plane_object.get_geometry().draw_arrays_count);
    }

    glEnable(GL_CULL_FACE);
}

void Application::render_snow_particles()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDepthMask(GL_FALSE);

    snow_particles_program.use();

    snow_particles_program.uniform(0, static_cast<float>(elapsed_time) * 1e-3f);
    glBindTextureUnit(0, snow_particle_tex);

    glBindVertexArray(snow_particles_vao);
    glDrawArrays(GL_POINTS, 0, snow_particles_count);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void Application::render_texture(GLuint texture)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    display_texture_program.use();

    glBindTextureUnit(0, texture);

    glBindVertexArray(empty_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_DEPTH_TEST);
}

void Application::bind_object(const ShaderProgram& program, const SceneObject& object)
{
    object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    object.get_material().bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);

    program.uniform(0, object.has_texture());
    
    glBindTextureUnit(0, object.has_texture() ? object.get_texture() : 0);

    object.get_geometry().bind_vao();
}


//  ===============================================  render gui  ===============================================
void Application::render_ui()
{
    const float unit = ImGui::GetFontSize();

    ImGui::Begin("settings", nullptr, ImGuiWindowFlags_NoDecoration);

    ImGui::SetWindowSize(ImVec2(20 * unit, 0.0f));
    ImGui::SetWindowPos(ImVec2(2 * unit, 2 * unit));

    float width = ImGui::GetWindowWidth();
    ImVec2 spacing_size(10.0f, unit * 0.2f);

    ImGui::PushItemWidth(width * 0.5f);

    std::string fps_cpu_s = "fps (cpu) : " + std::to_string(fps_cpu);
    ImGui::TextUnformatted(fps_cpu_s.c_str());

    std::string fps_gpu_s = "fps (gpu): " + std::to_string(fps_gpu);
    ImGui::TextUnformatted(fps_gpu_s.c_str());

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  snow  ========");
    ImGui::Dummy(spacing_size);

    ImGui::Checkbox("snow", &use_snow);

    ImGui::SliderFloat("snow speed", &snow_accum_vel_mult, 0.0f, 10.0f, "%.2f");

    ImGui::SliderFloat("snow blur", &snow_accum_blur_radius, 1.0f, 10.0f, "%.2f");

    ImGui::Checkbox("permanent broom", &broom_permanent);

    ImGui::Checkbox("show snow plane", &show_snow_plane);

    ImGui::SliderFloat("snow height", &snow_height_max, 0.1f, 2.5f, "%.2f");

    ImGui::SliderFloat("tesselation", &snow_plane_tess_factor, 1.0f, 200.0f, "%.1f");

    const char* particle_labels[10] = {"256", "512", "1024", "2048", "4096", "8192", "16384", "32768", "65536", "131072"};
    int exponent = static_cast<int>(log2(snow_particles_count) - 8);
    if (ImGui::Combo("particle count", &exponent, particle_labels, IM_ARRAYSIZE(particle_labels))) {
        snow_particles_count_target = static_cast<int>(glm::pow(2, exponent + 8));
    }

    clear_snow_accum = ImGui::Button("clear snow");

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  general  ========");
    ImGui::Dummy(spacing_size);

    ImGui::SliderAngle("light angle", &light_angle, 0);

    ImGui::Checkbox("wireframe", &wireframe);
    ImGui::Checkbox("pbr", &pbr);

    do_reset_settings = ImGui::Button("reset setting");

    ImGui::End();
}


//  ===============================================  input  ===============================================
void Application::on_resize(int width, int height)
{
    PV227Application::on_resize(width, height);
}

void Application::on_mouse_move(double x, double y)
{
    PV227Application::on_mouse_move(x, y);
    broom_center = glm::vec2(static_cast<float>(x), static_cast<float>(height - y));
}