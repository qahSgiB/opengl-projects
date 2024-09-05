#include "math_util.hpp"




//  ===============================================  random singleton  ===============================================

std::optional<RandomSingleton> RandomSingleton::r = std::nullopt;

RandomSingleton::RandomSingleton()
{
    std::random_device rd;
    random_generator = std::mt19937(rd());
    dis01 = std::uniform_real_distribution<float>(0.0f, 1.0f);
}

std::mt19937& RandomSingleton::_get_random_generator()
{
    return random_generator;
}

float RandomSingleton::_random01()
{
    return dis01(random_generator);
}


//  ===============================================  random singleton - static methods  ===============================================

void RandomSingleton::init()
{
    r = RandomSingleton();
}

void RandomSingleton::ensure_init()
{
    if (!r.has_value()) {
        init();
    }
}

std::mt19937& RandomSingleton::get_random_generator()
{
    ensure_init();
    return r.value()._get_random_generator();
}

float RandomSingleton::random01()
{
    ensure_init();
    return r.value()._random01();
}


//  ===============================================  random singleton - non class function  ===============================================

std::mt19937& rnd() { return RandomSingleton::get_random_generator(); }
float random01() { return RandomSingleton::random01(); }


//  ===============================================  hsv rgb conversion  ===============================================

glm::vec3 hsv_to_rgb(glm::vec3 rgb)
{
    float hue = rgb.r;
    float saturation = rgb.g;
    float value = rgb.b;

    float chroma = saturation * value;
    float hue6 = hue * 6.0f;
    float x = chroma * (1.0f - glm::abs(glm::mod(hue6, 2.0f) - 1.0f));

    float m = value - chroma;
    if (hue6 < 1.0f) {
        return glm::vec3(chroma, x, 0.0f) + m;
    } else if (hue6 < 2.0f) {
        return glm::vec3(x, chroma, 0.0f) + m;
    } else if (hue6 < 3.0f) {
        return glm::vec3(0.0f, chroma, x) + m;
    } else if (hue6 < 4.0f) {
        return glm::vec3(0.0f, x, chroma) + m;
    } else if (hue6 < 5.0f) {
        return glm::vec3(x, 0.0f, chroma) + m;
    } else {
        return glm::vec3(chroma, 0.0f, x) + m;
    }

    return glm::vec3();
}

glm::vec4 hsv_to_rgb(glm::vec4 rgba) { return glm::vec4(hsv_to_rgb(glm::vec3(rgba)), rgba.a); }


//  ===============================================  linear mapping  ===============================================

float linmap(float a0, float a1, float b0, float b1, float x)
{
    return (b1 - b0) * (x - a0) / (a1 - a0) + b0;
}

// linmap(0, 1, b0, b1, x)
float linmap01(float b0, float b1, float x)
{
    return (b1 - b0) * x + b0;
}

// linmap(0, 1, -v, v, x)
float linmap01v(float v, float x)
{
    return v * (2.0f * x - 1.0f);
}


//  ===============================================  type conversion  ===============================================

ImVec4 glm_to_imgui_v4(glm::vec4 v) { return ImVec4(v.x, v.y, v.z, v.w); }
glm::vec4 imgui_to_glm_v4(ImVec4 v) { return glm::vec4(v.x, v.y, v.z, v.w); }

ImVec2 glm_to_imgui_v2(glm::vec2 v) { return ImVec2(v.x, v.y); }
glm::vec2 imgui_to_glm_v2(ImVec2 v) { return glm::vec2(v.x, v.y); }