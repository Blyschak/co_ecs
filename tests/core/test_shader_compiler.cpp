#include <cobalt/render/shader_compiler.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

TEST(shader_pre_process, error_empty) {
    auto source_code = R"#()#";

    auto result = shader_compiler::pre_process(source_code);

    ASSERT_TRUE(result.is_error());
    ASSERT_EQ(result.error().error, "Syntax error, expected '#[vertex]'");
}

TEST(shader_pre_process, error_garbage) {
    auto source_code = R"#(#[vertex)#";

    auto result = shader_compiler::pre_process(source_code);

    ASSERT_TRUE(result.is_error());
    ASSERT_EQ(result.error().error, "Syntax error, expected '#[vertex]'");
}

TEST(shader_pre_process, error_missing_fragment) {
    auto source_code = R"#(
#[vertex]
#version 330 core

layout(location = 0) in vec3 a_Position;

layout(location = 0) uniform mat4 u_ViewProjection;
layout(location = 1) uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

)#";

    auto result = shader_compiler::pre_process(source_code);

    ASSERT_TRUE(result.is_error());
    ASSERT_EQ(result.error().error, "Syntax error, expected '#[fragment]'");
}

TEST(shader_pre_process, error_missing_vertex) {
    auto source_code = R"#(
#[fragment]
#version 330 core

layout(location = 0) out vec4 color;

layout(location = 0) uniform vec4 u_Color;

void main()
{
    color = u_Color;
}
)#";

    auto result = shader_compiler::pre_process(source_code);

    ASSERT_TRUE(result.is_error());
    ASSERT_EQ(result.error().error, "Syntax error, expected '#[vertex]'");
}

TEST(shader_pre_process, successful) {
    auto source_code = R"#(
#[vertex]
#version 330 core

layout(location = 0) in vec3 a_Position;

layout(location = 0) uniform mat4 u_ViewProjection;
layout(location = 1) uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#[fragment]
#version 330 core

layout(location = 0) out vec4 color;

layout(location = 0) uniform vec4 u_Color;

void main()
{
    color = u_Color;
}
)#";

    auto result = shader_compiler::pre_process(source_code);

    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(result.get().vertex, R"#(
#version 330 core

layout(location = 0) in vec3 a_Position;

layout(location = 0) uniform mat4 u_ViewProjection;
layout(location = 1) uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

)#");
    EXPECT_EQ(result.get().fragment, R"#(
#version 330 core

layout(location = 0) out vec4 color;

layout(location = 0) uniform vec4 u_Color;

void main()
{
    color = u_Color;
}
)#");
}

constexpr auto source_code = R"#(
#[vertex]
#version 450 core

layout(location = 0) in vec3 a_Position;

layout(location = 0) uniform mat4 u_ViewProjection;
layout(location = 1) uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#[fragment]
#version 450 core

layout(location = 0) out vec4 color;

layout(location = 0) uniform vec4 u_Color;

void main()
{
    color = u_Color;
}
)#";

TEST(shader_compilation, successful) {
    auto result = shader_compiler::compile(source_code, "my_shader");
    ASSERT_TRUE(result.is_ok());
}

TEST(shader_reflect, reflection) {
    auto result = shader_compiler::compile(source_code, "my_shader");
    ASSERT_TRUE(result.is_ok());

    auto reflect_result = shader_compiler::reflect(result.get());
    ASSERT_TRUE(reflect_result.is_ok());

    for (const auto& uniform : reflect_result.get().vertex.uniforms) {
        std::cout << uniform.name << std::endl;
    }
}