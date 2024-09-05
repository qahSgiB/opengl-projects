#pragma once

// header only (templates)

#include "light_ubo.hpp" // used only for PhongLightData
#include "opengl_object.hpp"

#include <vector>



// sizeof(T) must be multiple of aligment
template<typename T, size_t aligment>
class UBOVector : public OpenGLObject
{
protected:
    size_t max_count;
    std::vector<T> data;

    GLbitfield flags;

public:
    UBOVector(size_t max_count = 0, GLbitfield flags = 0, GLenum target = GL_UNIFORM_BUFFER) : OpenGLObject(target, false), max_count(max_count), data(), flags(flags)
    {
        gpu_create();
    }

    UBOVector(const UBOVector<T, aligment>& other) = delete;
    // [todo] OpenGLObject constructor use move
    UBOVector(UBOVector<T, aligment>&& other) : OpenGLObject(other), max_count(max_count), data(std::move(other.data)), flags(flags) {}

    UBOVector<T, aligment>& operator=(const UBOVector<T, aligment>& other) = delete;
    UBOVector<T, aligment>& operator=(UBOVector<T, aligment>&& other)
    {
        // OpenGLObject::operator=(other); // [todo] not working
        opengl_object = other.opengl_object;
        target = other.target;
        cpu_only = other.cpu_only;

        max_count = other.max_count;
        data = std::move(other.data);
        flags = other.flags;

        return *this;
    }

protected:
    void gpu_create()
    {
        glCreateBuffers(1, &opengl_object);
        glNamedBufferStorage(opengl_object, aligment + sizeof(T) * max_count, nullptr, flags);
    }

public:
    void update_opengl_data() override
    {
        assert((flags & GL_DYNAMIC_STORAGE_BIT) == GL_DYNAMIC_STORAGE_BIT);
        
        size_t count = data.size();
        glNamedBufferSubData(opengl_object, 0, sizeof(unsigned int), &count);
        if (max_count > 0 && count > 0) {
            glNamedBufferSubData(opengl_object, aligment, sizeof(T) * std::min(count, max_count), data.data());
        }
    }

    // [todo] (name) bind_buffer_base?
    void bind(GLuint index)
    {
        glBindBufferBase(target, index, opengl_object);
    }

    std::vector<T>& get_data() { return data; }
    const std::vector<T>& get_data() const { return data; }
};



using PhongLightsUBOVector = UBOVector<PhongLightData, sizeof(float) * 4>;