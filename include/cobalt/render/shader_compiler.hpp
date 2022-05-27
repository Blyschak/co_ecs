#pragma once

#include <cobalt/asl/result.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace cobalt {

struct shader_binary {
    std::vector<uint32_t> vertex;
    std::vector<uint32_t> fragment;
};

struct shader_sources {
    std::string vertex;
    std::string fragment;
};

struct shader_compilation_error {
    std::string error;

    static shader_compilation_error missing_token(auto&& token) {
        std::stringstream ss;
        ss << "Syntax error, expected '" << token << "'";
        return shader_compilation_error{ ss.str() };
    }

    static shader_compilation_error with_message(auto&& msg) {
        return shader_compilation_error{ msg };
    }
};

struct shader_resource {
    std::string name;
};

struct shader_reflection {
    struct {
        std::vector<shader_resource> uniforms;
    } vertex, fragment;
};

struct shader_reflection_error {
    std::string message;
};

struct shader_compilation_options {
    bool optimize{ true };
};

class shader_compiler {
public:
    static asl::result<shader_binary, shader_compilation_error>
        compile(std::string source, std::string input_filename, shader_compilation_options opts = {}) {
        shaderc::Compiler compiler;
        shaderc::CompileOptions shaderc_options;

        auto result = pre_process(source);
        if (result.is_error()) {
            return result;
        }

        auto shader_sources = result.get();

        shaderc_options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
        if (opts.optimize) {
            shaderc_options.SetOptimizationLevel(shaderc_optimization_level_performance);
        }

        // TODO: put in shader API

        auto vertex_module = compiler.CompileGlslToSpv(
            shader_sources.vertex, shaderc_glsl_vertex_shader, input_filename.c_str(), shaderc_options);
        if (vertex_module.GetCompilationStatus() != shaderc_compilation_status_success) {
            return shader_compilation_error::with_message(vertex_module.GetErrorMessage());
        }

        auto fragment_module = compiler.CompileGlslToSpv(
            shader_sources.fragment, shaderc_glsl_fragment_shader, input_filename.c_str(), shaderc_options);
        if (fragment_module.GetCompilationStatus() != shaderc_compilation_status_success) {
            return shader_compilation_error::with_message(fragment_module.GetErrorMessage());
        }

        return shader_binary{
            {
                vertex_module.begin(),
                vertex_module.end(),
            },
            {
                fragment_module.begin(),
                fragment_module.end(),
            },
        };
    }

    static asl::result<shader_sources, shader_compilation_error> pre_process(std::string source) {
        constexpr std::string_view vertex_token = "#[vertex]";
        constexpr std::string_view fragment_token = "#[fragment]";

        auto vertex_token_pos{ std::string::npos };
        auto fragment_token_pos{ std::string::npos };

        vertex_token_pos = source.find(vertex_token);
        if (vertex_token_pos == std::string::npos) {
            return shader_compilation_error::missing_token(vertex_token);
        }

        fragment_token_pos = source.find(fragment_token);
        if (fragment_token_pos == std::string::npos) {
            return shader_compilation_error::missing_token(fragment_token);
        }

        auto vertex_code_start_pos = vertex_token_pos + vertex_token.size();
        auto vertex_code_size = fragment_token_pos - vertex_code_start_pos;
        auto fragment_code_start_pos = fragment_token_pos + fragment_token.size();
        auto fragment_code_size = source.size() - fragment_code_start_pos;

        shader_sources sources;
        sources.vertex = source.substr(vertex_code_start_pos, vertex_code_size);
        sources.fragment = source.substr(fragment_code_start_pos, fragment_code_size);

        return sources;
    }

    static asl::result<shader_reflection, shader_reflection_error> reflect(const shader_binary& binary) {
        // TODO fill in needed structs

        shader_reflection reflection{};
        {
            spirv_cross::Compiler compiler(binary.vertex);
            spirv_cross::ShaderResources resources = compiler.get_shader_resources();

            for (const auto& uniform : resources.uniform_buffers) {
                reflection.vertex.uniforms.emplace_back(shader_resource{ uniform.name });
            }
        }

        {
            spirv_cross::Compiler compiler(binary.fragment);
            spirv_cross::ShaderResources resources = compiler.get_shader_resources();

            for (const auto& uniform : resources.uniform_buffers) {
                reflection.fragment.uniforms.emplace_back(shader_resource{ uniform.name });
            }
        }

        return reflection;
    }

private:
};

} // namespace cobalt