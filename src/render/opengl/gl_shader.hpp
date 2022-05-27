#pragma once

#include <cobalt/asl/result.hpp>

#include <glad/glad.h>
#include <glm/ext.hpp>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <span>

namespace cobalt {

using u32_view = std::span<uint32_t>;

class gl_shader {
public:
    ~gl_shader() {
        glDeleteProgram(_program);
    }

    enum class error {
        linking_failure,
    };

    /// @brief Create shader from SPIR V binaries
    ///
    /// @param vertex Vertex shader binary
    /// @param fragment Fragment shader binary
    /// @return gl_shader Shader object
    static asl::result<gl_shader, error> from_binary(u32_view vertex, u32_view fragment) {
        GLuint program = glCreateProgram();

        auto vertex_shader = create_shader(program, GL_VERTEX_SHADER, vertex);
        auto fragment_shader = create_shader(program, GL_FRAGMENT_SHADER, fragment);

        glLinkProgram(program);

        GLint is_linked;
        glGetProgramiv(program, GL_LINK_STATUS, &is_linked);

        if (is_linked == GL_FALSE) {
            glDeleteProgram(program);
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            return error::linking_failure;
        }

        return gl_shader{ program };
    }

    void bind() const {
        glUseProgram(_program);
    }

    void unbind() const {
        glUseProgram(0);
    }

    void set_int(const std::string& name, int value) {
        upload_uniform_int(name, value);
    }

    void set_int_array(const std::string& name, int* values, uint32_t count) {
        upload_uniform_int_array(name, values, count);
    }

    void set_float(const std::string& name, float value) {
        upload_uniform_float(name, value);
    }

    void set_float2(const std::string& name, const glm::vec2& value) {
        upload_uniform_float2(name, value);
    }

    void set_float3(const std::string& name, const glm::vec3& value) {
        upload_uniform_float_3(name, value);
    }

    void set_float4(const std::string& name, const glm::vec4& value) {
        upload_uniform_float_4(name, value);
    }

    void set_mat4(const std::string& name, const glm::mat4& value) {
        upload_uniform_mat4(name, value);
    }

    void upload_uniform_int(const std::string& name, int value) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniform1i(location, value);
    }

    void upload_uniform_int_array(const std::string& name, int* values, uint32_t count) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniform1iv(location, count, values);
    }

    void upload_uniform_float(const std::string& name, float value) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniform1f(location, value);
    }

    void upload_uniform_float2(const std::string& name, const glm::vec2& value) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniform2f(location, value.x, value.y);
    }

    void upload_uniform_float_3(const std::string& name, const glm::vec3& value) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniform3f(location, value.x, value.y, value.z);
    }

    void upload_uniform_float_4(const std::string& name, const glm::vec4& value) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void UploadUniformMat3(const std::string& name, const glm::mat3& matrix) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void upload_uniform_mat4(const std::string& name, const glm::mat4& matrix) {
        GLint location = glGetUniformLocation(_program, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

private:
    gl_shader(GLuint program) : _program(program) {
    }

    static GLuint create_shader(GLuint program, GLuint stage, u32_view binary) {
        GLuint shader = glCreateShader(stage);
        glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, binary.data(), binary.size() * sizeof(uint32_t));
        glSpecializeShader(shader, "main", 0, nullptr, nullptr);
        glAttachShader(program, shader);
        return shader;
    }
    GLuint _program;
};

} // namespace cobalt