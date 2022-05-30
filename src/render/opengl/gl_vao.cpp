#include <cobalt/core/assert.hpp>

#include "gl_buffer.hpp"
#include "gl_vao.hpp"

#include <glad/glad.h>


namespace cobalt {

static GLenum to_gl_enum(vertex_format format) {
    switch (format) {
    case vertex_format::bool_dt:
        return GL_BOOL;
    case vertex_format::float_dt:
    case vertex_format::float2_dt:
    case vertex_format::float3_dt:
    case vertex_format::float4_dt:
    case vertex_format::mat3_dt:
    case vertex_format::mat4_dt:
        return GL_FLOAT;
    case vertex_format::int_dt:
    case vertex_format::int2_dt:
    case vertex_format::int3_dt:
    case vertex_format::int4_dt:
        return GL_INT;
    }

    co_unreachable("invalid vertex format");
}

gl_vertex_array::gl_vertex_array() {
    glCreateVertexArrays(1, &vao);
}

gl_vertex_array::~gl_vertex_array() {
    glDeleteVertexArrays(1, &vao);
}

void gl_vertex_array::bind() const {
    glBindVertexArray(vao);
}

void gl_vertex_array::unbind() const {
    glBindVertexArray(0);
}

void gl_vertex_array::push_back(const gl_vertex_buffer& buffer) {
    const auto& layout = buffer.layout();
    int index = 0;
    for (const auto& element : layout) {
        glVertexAttribPointer(index,
            element.count,
            to_gl_enum(element.format),
            element.normalized,
            layout.stride(),
            reinterpret_cast<const void*>(element.offset()));
        index++;
    }
    _size += buffer.size();
}

} // namespace cobalt
