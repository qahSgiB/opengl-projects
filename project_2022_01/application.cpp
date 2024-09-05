#include "application.hpp"

#include "utils.hpp"

#include "src/math_util.hpp"



Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments) : PV227Application(initial_width, initial_height, arguments)
{
    hash31_seed_dis = std::uniform_real_distribution<float>(0, 10.0f);

    // prepare
    compile_shaders();

    prepare_cameras();
    prepare_scene();

    prepare_fireworks();
    prepare_hdr();
    prepare_mirror();
    
    // reset settings
    reset_global_config();
    reset_fireworks_config();
    reset_hdr_config();

    // other initialization
    spawn_default = false;
    spawn_random = false;
    spawn_random_at = false;

    mouse_plane_y = 0.16f;

    show_main_menu = true;
    show_fireworks_menu1 = false;
    show_fireworks_menu2 = false;

    auto_spawn_pause = false;
    auto_spawn_delta = auto_spawn_delay;

    // opengl initialization
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0);
}

Application::~Application() {}


void Application::compile_shaders()
{
    default_unlit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "unlit.frag");
    default_lit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "lit.frag");

    particle_textured_program = ShaderProgram();
    particle_textured_program.add_vertex_shader(lecture_shaders_path / "particle_textured.vert");
    particle_textured_program.add_fragment_shader(lecture_shaders_path / "particle_textured.frag");
    particle_textured_program.add_geometry_shader(lecture_shaders_path / "particle_textured.geom");
    particle_textured_program.link();

    update_firework_program = ShaderProgram();
    update_firework_program.add_compute_shader(lecture_shaders_path / "fireworks.comp");
    update_firework_program.link();

    hdr_to_ldr_program = ShaderProgram(lecture_shaders_path / "fullscreen_quad.vert", lecture_shaders_path / "hdr_to_ldr.frag");

    std::cout << "Shaders are reloaded." << std::endl;
}


//  ===============================================  init  ===============================================

void Application::prepare_scene()
{
    // objects
    lake_size = glm::vec2(24.0f, 24.0f);
    lake_outside_offset = glm::vec2(1.0f);

    outer_terrain_object = SceneObject(
        cube,
        ModelUBO(glm::translate(glm::vec3(0.0f, -0.08f, 0.0f)) * glm::scale(glm::vec3(lake_size.x * 0.5f + lake_outside_offset.x, 0.1f, lake_size.y * 0.5f + lake_outside_offset.y))),
        PhongMaterialData(glm::vec3(0.05f, 0.25f, 0.01f), 1.0f, 200.0f, true)
    );

    castel_base = SceneObject(
        cube,
        ModelUBO(glm::translate(glm::vec3(0.0f, 0.05f, 0.0f)) * glm::scale(glm::vec3(3.8f, 0.1f, 3.8f))),
        PhongMaterialData(glm::vec3(0.05f, 0.25f, 0.01f), 1.0f, 200.0f, true)
    );

    lake_object = SceneObject(
        cube,
        ModelUBO(glm::translate(glm::vec3(0.0f, -0.05f, 0.0f)) * glm::scale(glm::vec3(lake_size.x * 0.5f, 0.1f, lake_size.y * 0.5f))),
        PhongMaterialData(glm::vec3(0.1f, 0.3f, 0.75f), 1.0f, 200.0f, true)
    );

    // Geometry castle = Geometry::from_file(lecture_folder_path / "models/castle3.obj");
    // castle_object = SceneObject(
    //     castle,
    //     ModelUBO(glm::translate(glm::vec3(0.0f, 1.98f, 0.0f)) * glm::scale(glm::vec3(7.f, 7.f, 7.f))),
    //     PhongMaterialData(glm::vec3(0.25f, 0.22f, 0.2f), 1.0f, 200.0f, true)
    // );
    Geometry castle = Geometry::from_file(lecture_folder_path / "models/castle2.obj");
    castle_object = SceneObject(
        castle,
        ModelUBO(glm::translate(glm::vec3(0.0f, 1.06f, 0.0f)) * glm::scale(glm::vec3(7.f, 7.f, 7.f))),
        PhongMaterialData(glm::vec3(0.25f, 0.22f, 0.2f), 1.0f, 200.0f, true)
    );

    mouse_box = SceneObject(
        cube,
        ModelUBO(),
        red_material_ubo
    );

    // lights
    phong_lights_bo = PhongLightsUBOVector(5, GL_DYNAMIC_STORAGE_BIT, GL_SHADER_STORAGE_BUFFER);
    phong_lights_bo.get_data().push_back(PhongLightData::CreateDirectionalLight(glm::vec3(3.0f, 2.0f, 4.0f), glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(0.0f)));
    // phong_lights_bo.get_data().push_back(PhongLightData::CreateDirectionalLight(glm::vec3(3.0f, 2.0f, 4.0f), 1.7f * glm::vec3(0.313f, 0.408f, 0.525f), 1.7f * glm::vec3(0.313f, 0.408f, 0.525f), glm::vec3(0.0f)));
    // phong_lights_bo.get_data().push_back(PhongLightData::CreateSpotLight(glm::vec3(4.0f, 2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.9f), glm::vec3(0.0f), glm::normalize(glm::vec3(-4.0f, -4.0f, 0.0f)), 1.0f, glm::cos(glm::radians(30.0f)), 0.0f, 0.0f, 0.2f));
    phong_lights_bo.update_opengl_data();
}

void Application::prepare_cameras()
{
    camera.set_eye_position(glm::radians(-45.f), glm::radians(35.f), 70.f);
    on_resize_cameras();
}


void Application::prepare_hdr()
{
    glCreateFramebuffers(1, &hdr_fbo);
    on_resize_hdr();
}

void Application::prepare_mirror()
{
    // framebuffer + textures
    mirror_texture_size = 1024;

    glCreateFramebuffers(1, &mirror_fbo);

    glCreateTextures(GL_TEXTURE_2D, 1, &mirror_fbo_color_texture);
    glCreateTextures(GL_TEXTURE_2D, 1, &mirror_fbo_depth_texture);

    glTextureStorage2D(mirror_fbo_color_texture, 1, GL_RGBA32F, mirror_texture_size, mirror_texture_size);
    glTextureStorage2D(mirror_fbo_depth_texture, 1, GL_DEPTH_COMPONENT24, mirror_texture_size, mirror_texture_size);

    TextureUtils::set_texture_2d_parameters(mirror_fbo_color_texture, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    TextureUtils::set_texture_2d_parameters(mirror_fbo_depth_texture, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

    glNamedFramebufferTexture(mirror_fbo, GL_COLOR_ATTACHMENT0, mirror_fbo_color_texture, 0);
    glNamedFramebufferTexture(mirror_fbo, GL_DEPTH_ATTACHMENT, mirror_fbo_depth_texture, 0);
    FBOUtils::check_framebuffer_status(mirror_fbo, "mirror framebuffer");

    // camera
    glm::mat4 projection = glm::perspective(glm::radians(90.f), 1.0f, 0.1f, 5000.0f);
    mirror_camera_ubo.set_projection(projection);
}

void Application::prepare_fireworks()
{
    fireworks_max_count = 5;
    firework_max_particle_count = 4096;

    fireworks.reserve(fireworks_max_count);

    for (size_t i = 0; i < fireworks_max_count; i++) {
        fireworks.emplace_back(firework_max_particle_count);
    }

    firework_lights = PhongLightsUBOVector(fireworks_max_count, GL_DYNAMIC_STORAGE_BIT, GL_SHADER_STORAGE_BUFFER);

    particle_texture = TextureUtils::load_texture_2d(lecture_textures_path / "star.png");
    TextureUtils::set_texture_2d_parameters(particle_texture, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}


//  ===============================================  window resizing  ===============================================

void Application::on_resize_cameras()
{
    glm::mat4 projection = glm::perspective(glm::radians(45.f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 5000.0f);
    normal_camera_ubo.set_projection(projection);
}


void Application::on_resize_hdr()
{
    glDeleteTextures(1, &hdr_fbo_color_texture);
    glDeleteTextures(1, &hdr_fbo_depth_texture);

    glCreateTextures(GL_TEXTURE_2D, 1, &hdr_fbo_color_texture);
    glCreateTextures(GL_TEXTURE_2D, 1, &hdr_fbo_depth_texture);

    glTextureStorage2D(hdr_fbo_color_texture, 1, GL_RGBA32F, width, height);
    glTextureStorage2D(hdr_fbo_depth_texture, 1, GL_DEPTH_COMPONENT24, width, height);

    TextureUtils::set_texture_2d_parameters(hdr_fbo_color_texture, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    TextureUtils::set_texture_2d_parameters(hdr_fbo_depth_texture, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

    glNamedFramebufferTexture(hdr_fbo, GL_COLOR_ATTACHMENT0, hdr_fbo_color_texture, 0);
    glNamedFramebufferTexture(hdr_fbo, GL_DEPTH_ATTACHMENT, hdr_fbo_depth_texture, 0);
    FBOUtils::check_framebuffer_status(hdr_fbo, "hdr framebuffer");
}


//  ===============================================  settings reset  ===============================================

void Application::reset_global_config()
{
    time_multiplier = 1.0f;
    elapsed_time_m = 0.0f;

    gravity = 0.000015f;

    use_mirror = true;
    mirror_factor = 0.72f;
    mirror_distortion = 0.25f;
}

void Application::reset_fireworks_config()
{
    auto_spawn_delay = 1400.0f;
    auto_spawn_delay_variance = 0.4f;

    FireworkRandomizationParams& fr = firework_randomization;

    fr.particle_count_min = 450;
    fr.particle_count_max = 570;

    float pos_spread = 1.0f;
    fr.start_pos_min = glm::vec3(-pos_spread, 0.3f, -pos_spread);
    fr.start_pos_max = glm::vec3(pos_spread, 0.5f, pos_spread);

    fr.up = glm::vec3(0.0f, 1.0f, 0.0f);
    fr.max_angle = glm::radians(8.0f);
    fr.vel_size_base = 0.0215f;
    fr.vel_size_variance = 0.2f;
    fr.explosion_force_base = 0.00007f;
    fr.explosion_force_variance = 0.4f;
    fr.explosion_force_variance_base = 0.01f;
    fr.explosion_force_variance_variance = 0.2f;

    fr.particle_size_base_base = 0.45f;
    fr.particle_size_base_variance = 0.4f;
    fr.rocket_size_mult = 3.0f;

    fr.hue_base = 0.115f;
    // fr.hue_range = 0.05f;
    fr.hue_range = 1.0f;
    // fr.saturation_base = 0.67f;
    fr.saturation_base = 0.31f;
    fr.saturation_range = 0.1f;

    // fr.hue_variance_base = 0.03f;
    // fr.hue_variance_variance = 0.25f;
    fr.hue_variance_base = 0.11f;
    fr.hue_variance_variance = 0.8f;
    // fr.saturation_variance_base = 0.13f;
    // fr.saturation_variance_variance = 0.25f;
    fr.saturation_variance_base = 0.25f;
    fr.saturation_variance_variance = 0.7f;

    fr.flying1_duration_base = 750.0f;
    fr.flying1_duration_variance = 0.13f;
    fr.explosion_duration_base = 70.0f;
    fr.explosion_duration_variance = 0.1f;
    fr.flying2_duration_base = 1100.0f;
    fr.flying2_duration_variance = 0.25f;
}

void Application::reset_hdr_config()
{
    // exposure = 1.3f;
    // gamma = 1.15f;

    use_hdr_mapping = false;

    exposure = 1.0f;
    gamma = 1.0f;
}


//  ===============================================  update  ===============================================

void Application::update(float delta)
{
    float delta_m = delta * time_multiplier;
    elapsed_time_m += delta_m;

    PV227Application::update(delta);
    update_fireworks(delta_m);
}

void Application::update_fireworks(float delta)
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    update_firework_program.use();

    update_firework_program.uniform(0, delta);
    update_firework_program.uniform(2, gravity);

    for (Firework& firework : fireworks) {
        firework.update(delta, gravity);

        float hash31_seed = hash31_seed_dis(rnd());
        update_firework_program.uniform(1, hash31_seed);
        
        firework.update_gpu(update_firework_program, 256);
    }

    glFinish();

    bool spawn_random_auto = false;

    if (!auto_spawn_pause) {
        auto_spawn_delta -= delta;
    }

    if (auto_spawn_delta <= 0.0f) {
        spawn_random_auto = true;
        auto_spawn_delta = auto_spawn_delay * (1.0f + linmap01v(auto_spawn_delay_variance, random01()));
    }

    if (spawn_random_auto) {
        try_spawn_firework(FireworkParams::create_random(firework_randomization));
    }
    if (spawn_random_at) {
        try_spawn_firework(FireworkParams::create_random_at(firework_randomization, spawn_random_at_pos));
        spawn_random_at = false;
    }
    if (spawn_random) {
        try_spawn_firework(FireworkParams::create_random(firework_randomization));
        spawn_random = false;
    }
    if (spawn_default) {
        try_spawn_firework(FireworkParams::create_default(firework_randomization));
        spawn_default = false;
    }
}

bool Application::try_spawn_firework(const FireworkParams& params)
{
    for (Firework& firework : fireworks) {
        if (!firework.active) {
            firework.activate(params);
            return true;
        }
    }

    return false;
}

void Application::update_firework_lights()
{
    std::vector<PhongLightData>& lights = firework_lights.get_data();
    lights.clear();

    for (Firework& firework : fireworks) {
        if (firework.active) {
            lights.push_back(firework.generate_light().value());
        }
    }

    firework_lights.update_opengl_data();
}

void Application::update_cameras()
{
    // same as glm::lookAt
    // glm::vec3 front = glm::normalize(glm::vec3(camera.get_eye_position()));
    // glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), front));
    // glm::vec3 up = glm::cross(front, right);
    // glm::mat4 view_matrix = glm::inverse(glm::mat4(glm::vec4(r, 0.0f), glm::vec4(u, 0.0f), glm::vec4(f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))) * glm::translate(-camera.get_eye_position());

    glm::mat4 view_matrix = glm::lookAt(camera.get_eye_position(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    normal_camera_ubo.set_view(view_matrix);
    normal_camera_ubo.update_opengl_data();

    // compute custom view matrix
    glm::vec3 mirror_center(0.0f, 0.15f, 0.0f);
    glm::vec3 mirror_eye = camera.get_eye_position() * glm::vec3(1.0f, -1.0f, 1.0f);

    glm::vec3 front = mirror_eye - mirror_center;
    glm::vec3 right(0.0f, 0.0f, lake_size.y * 0.5);
    glm::vec3 up(lake_size.x * 0.5f, 0.0f, 0.0f);

    glm::mat4 mirror_view_matrix = glm::inverse(glm::mat4(glm::vec4(up, 0.0f), glm::vec4(-right, 0.0f), glm::vec4(front, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))) * glm::translate(-mirror_eye);
    mirror_camera_ubo.set_view(mirror_view_matrix);
    
    mirror_camera_ubo.update_opengl_data();

    // mirror_clip_distance = glm::length(front);
    mirror_clip_distance = 1.0f;
}


//  ===============================================  render  ===============================================

void Application::render()
{
    glBeginQuery(GL_TIME_ELAPSED, render_time_query);

    // compute cameras and firework lights
    update_cameras();
    update_firework_lights();

    // rendering
    if (use_mirror) {
        glBindFramebuffer(GL_FRAMEBUFFER, mirror_fbo);
        render_mirror();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, use_hdr_mapping ? hdr_fbo : 0);

    render_from_normal_camera();

    if (use_hdr_mapping) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        render_hdr_to_ldr();
    }

    // timing
    glEndQuery(GL_TIME_ELAPSED);
    glFinish();
    GLuint64 render_time;
    glGetQueryObjectui64v(render_time_query, GL_QUERY_RESULT, &render_time);
    fps_gpu = 1.0f / (static_cast<float>(render_time) * 1e-9f);
}

void Application::render_hdr_to_ldr()
{
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    hdr_to_ldr_program.use();

    hdr_to_ldr_program.uniform(0, exposure);
    hdr_to_ldr_program.uniform(1, gamma);

    glBindTextureUnit(0, hdr_fbo_color_texture);

    glBindVertexArray(empty_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_DEPTH_TEST);
}

void Application::render_mirror()
{
    glViewport(0, 0, mirror_texture_size, mirror_texture_size);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mirror_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

    render_scene_with_lights(true);
}

void Application::render_from_normal_camera()
{
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    normal_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

    render_scene_with_lights(false);
}

void Application::render_scene_with_lights(bool from_mirror)
{
    phong_lights_bo.bind(PhongLightsUBO::DEFAULT_LIGHTS_BINDING);
    firework_lights.bind(4);

    render_scene(default_lit_program, from_mirror);
    if (!from_mirror) {
        render_mouse_box(default_unlit_program);
    }
    render_fireworks(from_mirror);
}

void Application::render_scene(const ShaderProgram& program, bool from_mirror)
{
    render_object(castle_object, program);
    render_object(castel_base, program);
    if (!from_mirror) {
        render_object(outer_terrain_object, program);
        render_lake(program);
    }
}

void Application::render_object(const SceneObject& object, const ShaderProgram& program)
{
    program.use();

    object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    object.get_material().bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);

    program.uniform(0, object.has_texture());
    if (object.has_texture()) {
        glBindTextureUnit(0, object.get_texture());
        program.uniform(1, 0.0f);
        program.uniform(2, false);
    }

    object.get_geometry().bind_vao();
    object.get_geometry().draw();
}

void Application::render_lake(const ShaderProgram& program)
{
    program.use();

    lake_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
    lake_object.get_material().bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);

    program.uniform(0, use_mirror);
    if (use_mirror) {
        glBindTextureUnit(0, mirror_fbo_color_texture);
        program.uniform(1, mirror_factor);
        program.uniform(2, true);
        program.uniform(3, mirror_distortion);
        program.uniform(4, static_cast<float>(elapsed_time_m / 1000.0));
    }

    lake_object.get_geometry().bind_vao();
    lake_object.get_geometry().draw();
}

void Application::render_fireworks(bool from_mirror)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    particle_textured_program.use();

    glBindTextureUnit(0, particle_texture);

    particle_textured_program.uniform(2, from_mirror ? mirror_clip_distance : 0.0f);

    for (const Firework& firework : fireworks) {
        firework.render(particle_textured_program);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void Application::render_mouse_box(const ShaderProgram& program)
{
    std::optional<glm::vec3> mouse_box_pos = get_mouse_box_pos_ws();
    if (!mouse_box_pos.has_value()) {
        return;
    }

    ModelUBO& mouse_box_model_ubo = mouse_box.get_model_ubo();
    mouse_box_model_ubo.set_matrix(glm::translate(mouse_box_pos.value()) * glm::scale(glm::vec3(0.25f, 0.01f, 0.25f)));
    mouse_box_model_ubo.update_opengl_data();

    render_object(mouse_box, program);
}


//  ===============================================  ui  ===============================================

void Application::render_ui()
{
    float unit = ImGui::GetFontSize();
    float width = 20 * unit;
    ImVec2 spacing_size(10.0f, unit * 0.2f);

    // main menu
    ImGui::Begin("main menu", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetWindowPos(ImVec2(0.5f * unit, 0.5f * unit));
    ImGui::SetWindowSize(ImVec2(width, 0.0f));
    
    if (!show_main_menu) {
        ImGui::Text("press [q] to show main menu");
    } else {
        ImGui::Text("press [q] to hide main menu");
        render_main_menu();
    }

    ImGui::End();

    // fireworks menu 1
    ImGui::Begin("fireworks menu (1)", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetWindowPos(ImVec2(1.0f * unit + width, 0.5f * unit));
    ImGui::SetWindowSize(ImVec2(width, 0.0f));
    
    if (!show_fireworks_menu1) {
        ImGui::Text("press [w] to show fireworks menu (1)");
    } else {
        ImGui::Text("press [w] to hide fireworks menu (1)");
        render_fireworks_menu1();
    }

    ImGui::End();

    // fireworks menu 2
    ImGui::Begin("fireworks menu (2)", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetWindowPos(ImVec2(1.5f * unit + 2.0f * width, 0.5f * unit));
    ImGui::SetWindowSize(ImVec2(width, 0.0f));
    
    if (!show_fireworks_menu2) {
        ImGui::Text("press [e] to show fireworks menu (2)");
    } else {
        ImGui::Text("press [e] to hide fireworks menu (2)");
        render_fireworks_menu2();
    }

    ImGui::End();
}

void Application::render_main_menu()
{
    float unit = ImGui::GetFontSize();
    float width = ImGui::GetWindowWidth();
    ImVec2 spacing_size(10.0f, unit * 0.2f);

    ImGui::PushItemWidth(width * 0.5f);

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  help  ========");
    ImGui::Dummy(spacing_size);
    ImGui::Text("[d] - spawn average firework");
    ImGui::Text("[space] - spawn randomized firework");
    ImGui::Text("[mouse middle] / [f]\n - spawn randomized firework at mouse");

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  fps  ========");
    ImGui::Dummy(spacing_size);

    ImGui::Text("FPS (CPU): %.2f", fps_cpu);
    ImGui::Text("FPS (GPU): %.2f", fps_gpu);


    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  settings  ========");
    ImGui::Dummy(spacing_size);

    if (ImGui::Button("reset settings")) {
        reset_global_config();
    }

    ImGui::SliderFloat("time multiplier", &time_multiplier, 0.0f, 5.0f, "%.2f");
    ImGui::SliderFloat("gravity", &gravity, 0.0f, 0.0001f, "%.6f");
    
    ImGui::Checkbox("mirror", &use_mirror);
    ImGui::SliderFloat("mirror factor", &mirror_factor, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("waves", &mirror_distortion, 0.0f, 1.0f, "%.2f");

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  hdr mapping  ========");
    ImGui::Dummy(spacing_size);

    if (ImGui::Button("reset hdr settings")) {
        reset_hdr_config();
    }
    
    ImGui::Checkbox("hdr mapping", &use_hdr_mapping);
    ImGui::SliderFloat("exposure", &exposure, 0.0f, 10.0f, "%.2f");
    ImGui::SliderFloat("gamma", &gamma, 0.0f, 10.0f, "%.2f");

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  fireworks  ========");
    ImGui::Dummy(spacing_size);

    std::string firework_name_format = fireworks_max_count < 10 ? "firework %1d" : "firework %2d";

    for (int firework_index = 0; firework_index < fireworks_max_count; firework_index++) {
        const Firework firework = fireworks[firework_index];

        ImGui::Text(firework_name_format.c_str(), firework_index + 1);
        ImGui::SameLine();

        if (firework.active) {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(glm_to_imgui_v4(glm::vec4(firework.state.color, 1.0f))));
            ImGui::ProgressBar(firework.state.alive_time / firework.state.end_time);
            ImGui::PopStyleColor(1);
        } else {
            ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0.0f), "not active");
        }
    }

    ImGui::PopItemWidth();
}

void Application::render_fireworks_menu1()
{
    float unit = ImGui::GetFontSize();
    float width = ImGui::GetWindowWidth();
    ImVec2 spacing_size(10.0f, unit * 0.2f);

    ImGui::PushItemWidth(width * 0.4f);

    ImGui::Dummy(spacing_size);
    ImGui::Text("(p) - per particle setting\n      (applied after explosion)");
    ImGui::Dummy(spacing_size);

    ImGui::Dummy(spacing_size);
    if (ImGui::Button("reset fireworks settings")) {
        reset_fireworks_config();
    }

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  auto spawn  ========");
    ImGui::Dummy(spacing_size);

    if (ImGui::Button(auto_spawn_pause ? "play" : "pause")) {
        auto_spawn_pause = !auto_spawn_pause;
    }

    float auto_spawn_delay_s = auto_spawn_delay / 1000.0f;
    if (ImGui::SliderFloat("delay", &auto_spawn_delay_s, 0.0f, 10.0f, "%.2f")) {
        auto_spawn_delay = auto_spawn_delay_s * 1000.0f;
    }
    ImGui::SliderFloat(" > variance##delay", &auto_spawn_delay_variance, 0.0f, 1.0f, "%.2f");

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  particle count  ========");
    ImGui::Dummy(spacing_size);

    int particle_count_min_int = static_cast<int>(firework_randomization.particle_count_min);
    if (ImGui::SliderInt("particle count min", &particle_count_min_int, 10, firework_max_particle_count)) {
        firework_randomization.particle_count_min = static_cast<size_t>(particle_count_min_int);
        if (firework_randomization.particle_count_min > firework_randomization.particle_count_max) {
            firework_randomization.particle_count_max = firework_randomization.particle_count_min;
        }
    }

    int particle_count_max_int = static_cast<int>(firework_randomization.particle_count_max);
    if (ImGui::SliderInt("particle count max", &particle_count_max_int, 10, firework_max_particle_count)) {
        firework_randomization.particle_count_max = static_cast<size_t>(particle_count_max_int);
        if (firework_randomization.particle_count_max < firework_randomization.particle_count_min) {
            firework_randomization.particle_count_min = firework_randomization.particle_count_max;
        }
    }

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  physics  ========");
    ImGui::Dummy(spacing_size);

    float max_angle_deg = glm::degrees(firework_randomization.max_angle);
    if (ImGui::SliderFloat("max start angle", &max_angle_deg, 0.0f, 90.0f, "%.2f")) {
        firework_randomization.max_angle = glm::radians(max_angle_deg);
    }

    ImGui::SliderFloat("vel size", &firework_randomization.vel_size_base, 0.0f, 0.05f, "%.4f");
    ImGui::SliderFloat(" > variance##vel size", &firework_randomization.vel_size_variance, 0.0f, 1.0f, "%.2f");

    ImGui::SliderFloat("explosion force", &firework_randomization.explosion_force_base, 0.0f, 0.0005f, "%.5f");
    ImGui::SliderFloat(" > variance##explosion force", &firework_randomization.explosion_force_variance, 0.0f, 1.0f, "%.2f");

    ImGui::PushItemWidth(90.0f);
    ImGui::SliderFloat("explosion force variance (p)", &firework_randomization.explosion_force_variance_base, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SliderFloat(" > variance##explosion force variance", &firework_randomization.explosion_force_variance_variance, 0.0f, 1.0f, "%.2f");

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  sizing  ========");
    ImGui::Dummy(spacing_size);

    ImGui::SliderFloat("particle size base", &firework_randomization.particle_size_base_base, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat(" > variance##particle size base", &firework_randomization.particle_size_base_variance, 0.0f, 1.0f, "%.2f");

    ImGui::SliderFloat("rocket size multiplier", &firework_randomization.rocket_size_mult, 0.0f, 10.0f, "%.2f");

    ImGui::PopItemWidth();
}

void Application::render_fireworks_menu2()
{
    float unit = ImGui::GetFontSize();
    float width = ImGui::GetWindowWidth();
    ImVec2 spacing_size(10.0f, unit * 0.2f);

    ImGui::PushItemWidth(width * 0.4f);

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  color  ========");
    ImGui::Dummy(spacing_size);

    ImGui::SliderFloat("hue", &firework_randomization.hue_base, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(" > range##hue", &firework_randomization.hue_range, 0.0f, 1.0f, "%.2f");

    ImGui::SliderFloat("saturation", &firework_randomization.saturation_base, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(" > range##saturation", &firework_randomization.saturation_range, 0.0f, 1.0f, "%.2f");

    ImGui::Text("color range visualization");

    float sat_min = glm::max(firework_randomization.saturation_base - firework_randomization.saturation_range * 0.5f, 0.0f);
    float sat_max = glm::min(firework_randomization.saturation_base + firework_randomization.saturation_range * 0.5f, 1.0f);
    glm::vec3 hsv_base(firework_randomization.hue_base, (sat_min + sat_max) * 0.5f, 1.0f);
    glm::vec3 hsv_max_var(firework_randomization.hue_range * 0.5f, (sat_max - sat_min) * 0.5f, 0.0f);

    size_t hsv_rect_count = 5;
    glm::vec2 hsv_rect_size_full = glm::vec2(3.0f);

    glm::vec2 imgui_draw_pos = imgui_to_glm_v2(ImGui::GetCursorScreenPos());
    glm::vec2 hsv_rect_size = hsv_rect_size_full * unit / hsv_rect_count;

    for (int dy = 0; dy <= hsv_rect_count; dy++) {
        for (int dx = 0; dx <= hsv_rect_count; dx++) {
            glm::vec3 hsv = hsv_base + hsv_max_var * (2.0f * glm::vec3(dx, dy, 0.0f) / hsv_rect_count - 1.0f);
            hsv.r = glm::mod(hsv.r, 1.0f);
            glm::vec2 pos = imgui_draw_pos + hsv_rect_size * glm::vec2(dx, dy);
            ImGui::GetWindowDrawList()->AddRectFilled(glm_to_imgui_v2(pos), glm_to_imgui_v2(pos + hsv_rect_size), ImColor(glm_to_imgui_v4(glm::vec4(hsv_to_rgb(hsv), 1.0f))));
        }
    }

    ImGui::Dummy(ImVec2(10.0f, hsv_rect_size_full.y * unit + 8.0f)); // [todo] calculate spacing between items

    ImGui::SliderFloat("hue variance (p)", &firework_randomization.hue_variance_base, 0.0f, 0.5f, "%.2f");
    ImGui::SliderFloat(" > variance##hue variance", &firework_randomization.hue_variance_variance, 0.0f, 1.0f, "%.2f");

    ImGui::SliderFloat("saturation variance (p)", &firework_randomization.saturation_variance_base, 0.0f, 0.5f, "%.2f");
    ImGui::SliderFloat(" > variance##saturation variance", &firework_randomization.saturation_variance_variance, 0.0f, 1.0f, "%.2f");

    ImGui::Dummy(spacing_size);
    ImGui::Text("  ========  timing  ========");
    ImGui::Dummy(spacing_size);

    float flying1_duration_base_s = firework_randomization.flying1_duration_base / 1000.0f;
    if (ImGui::SliderFloat("flying 1 duration", &flying1_duration_base_s, 0.0f, 3.0f, "%.2f")) {
        firework_randomization.flying1_duration_base = flying1_duration_base_s * 1000.0f;
    }
    ImGui::SliderFloat(" > variance##flying 1 duration", &firework_randomization.flying1_duration_variance, 0.0f, 1.0f, "%.2f");

    float explosion_duration_base_s = firework_randomization.explosion_duration_base / 1000.0f;
    if (ImGui::SliderFloat("explosion duration", &explosion_duration_base_s, 0.0f, .5f, "%.3f")) {
        firework_randomization.explosion_duration_base = explosion_duration_base_s * 1000.0f;
    }
    ImGui::SliderFloat(" > variance##explosion duration", &firework_randomization.explosion_duration_variance, 0.0f, 1.0f, "%.2f");

    float flying2_duration_base_s = firework_randomization.flying2_duration_base / 1000.0f;
    if (ImGui::SliderFloat("flying 2 duration", &flying2_duration_base_s, 0.0f, 5.0f, "%.2f")) {
        firework_randomization.flying2_duration_base = flying2_duration_base_s * 1000.0f;
    }
    ImGui::SliderFloat(" > variance##flying 2 duration", &firework_randomization.flying2_duration_variance, 0.0f, 1.0f, "%.2f");

    ImGui::PopItemWidth();
}


//  ===============================================  input  ===============================================

void Application::on_resize(int width, int height)
{
    PV227Application::on_resize(width, height);
    on_resize_cameras();
    on_resize_hdr();
}

void Application::on_mouse_button(int button, int action, int mods)
{
    PV227Application::on_mouse_button(button, action, mods);

    if (button == GLFW_MOUSE_BUTTON_3) {
        if (action == GLFW_RELEASE) {
            std::optional<glm::vec3> mouse_box_pos = get_mouse_box_pos_ws();
            if (mouse_box_pos.has_value()) {
                spawn_random_at = true;
                spawn_random_at_pos = mouse_box_pos.value();
            }
        }
    }
}

void Application::on_key_pressed(int key, int scancode, int action, int mods)
{
    GUIApplication::on_key_pressed(key, scancode, action, mods);

    // firework spawning
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            spawn_default = true;
        }
    } else if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_PRESS) {
            spawn_random = true;
        }
    } else if (key == GLFW_KEY_F) {
        if (action == GLFW_PRESS) {
            std::optional<glm::vec3> mouse_box_pos = get_mouse_box_pos_ws();
            if (mouse_box_pos.has_value()) {
                spawn_random_at = true;
                spawn_random_at_pos = mouse_box_pos.value();
            }
        }
    }

    // gui show/hide
    if (key == GLFW_KEY_Q) {
        if (action == GLFW_PRESS) {
            show_main_menu = !show_main_menu;
        }
    } else if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            show_fireworks_menu1 = !show_fireworks_menu1;
        }
    } else if (key == GLFW_KEY_E) {
        if (action == GLFW_PRESS) {
            show_fireworks_menu2 = !show_fireworks_menu2;
        }
    }
}


//  ===============================================  mouse box  ===============================================

std::optional<glm::vec3> Application::get_mouse_box_pos_ws()
{
    double mouse_x;
    double mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    glm::vec2 mouse(mouse_x, mouse_y);

    glm::vec2 mouse_cs2 = (mouse / glm::vec2(width, height)) * 2.0f - 1.0f;
    glm::vec4 mouse_cs(mouse_cs2.x, -mouse_cs2.y, 1.0f, 1.0f);

    const CameraData& camera_data = normal_camera_ubo.get_data()[0];

    glm::vec4 mouse_ws4 = camera_data.view_inv * camera_data.projection_inv * mouse_cs;
    glm::vec3 mouse_ws(mouse_ws4 / mouse_ws4.w);

    glm::vec3 mouse_dir = mouse_ws - glm::vec3(camera_data.eye_position);

    glm::vec3 start_pos_min = firework_randomization.start_pos_min;
    glm::vec3 start_pos_max = firework_randomization.start_pos_max;

    glm::vec3 mouse_plane_center(0.0f, mouse_plane_y, 0.0f);
    glm::vec3 mouse_plane_a(lake_size.x * 0.5f + lake_outside_offset.x, 0.0f, 0.0f);
    glm::vec3 mouse_plane_b(0.0f, 0.0f, lake_size.y * 0.5f + lake_outside_offset.y);

    glm::mat3 m(mouse_plane_a, mouse_plane_b, -mouse_dir);

    if (glm::abs(glm::determinant(m)) <= 0.00001f) { // mouse_dir is parralel with start plane
        return std::nullopt;
    }

    glm::vec3 t = glm::inverse(m) * (glm::vec3(camera_data.eye_position) - mouse_plane_center);

    if (glm::abs(t.x) > 1.0f || glm::abs(t.y) > 1.0f) {
        return std::nullopt;
    }

    return mouse_plane_center + t.x * mouse_plane_a + t.y * mouse_plane_b;
}