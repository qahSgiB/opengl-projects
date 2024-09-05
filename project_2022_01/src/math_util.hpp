#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

#include <optional>
#include <random>



class RandomSingleton
{
private:
    static std::optional<RandomSingleton> r;

    std::mt19937 random_generator;
    std::uniform_real_distribution<float> dis01;

public:
    RandomSingleton();

    std::mt19937& _get_random_generator();
    float _random01();


    static void init();
    static void ensure_init();
    static std::mt19937& get_random_generator();
    static float random01();
};


std::mt19937& rnd();
float random01();


glm::vec3 hsv_to_rgb(glm::vec3 rgb);
glm::vec4 hsv_to_rgb(glm::vec4 rgba);


float linmap(float a0, float a1, float b0, float b1, float x);
float linmap01(float b0, float b1, float x);
float linmap01v(float v, float x);


ImVec4 glm_to_imgui_v4(glm::vec4 v);
glm::vec4 imgui_to_glm_v4(ImVec4 v);
ImVec2 glm_to_imgui_v2(glm::vec2 v);
glm::vec2 imgui_to_glm_v2(ImVec2 v);