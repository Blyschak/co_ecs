#include "gl_buffer.hpp"

#include <cobalt/asl/check.hpp>
#include <cobalt/core/logging.hpp>

#include <glad/glad.h>

namespace cobalt {

static GLenum to_gl_enum(buffer_usage usage) {
    switch (usage) {
    case buffer_usage::static_usage:
        return GL_STATIC_DRAW;
    case buffer_usage::dynamic_usage:
        return GL_DYNAMIC_DRAW;
    }

    asl::check_failed("unknown buffer usage type");
}

static GLenum to_gl_enum(buffer_type type) {
    switch (type) {
    case buffer_type::vertex:
        return GL_ARRAY_BUFFER;
    case buffer_type::index:
        return GL_ELEMENT_ARRAY_BUFFER;
    }

    asl::check_failed("unknown buffer type");
}

std::uint32_t create_gl_buffer(GLenum type, GLsizeiptr size, const void* ptr, GLenum usage) {
    std::uint32_t id{};
    glCreateBuffers(1, &id);
    glBindBuffer(type, id);
    glBufferData(type, size, ptr, usage);
    return id;
}

void DeleteGLgl_buffer(std::uint32_t id) {
    glDeleteBuffers(1, &id);
}

gl_buffer::gl_buffer(buffer_type type, buffer_usage usage, std::uint32_t size) : gl_buffer(type, usage, nullptr, size) {
}

gl_buffer::gl_buffer(buffer_type type, buffer_usage usage, std::span<const std::uint8_t> data) :
    gl_buffer(type, usage, data.data(), data.size_bytes()) {
}

gl_buffer::gl_buffer(buffer_type type, buffer_usage usage, const void* data, std::size_t size) :
    _id(create_gl_buffer(to_gl_enum(type), size, data, to_gl_enum(usage))), _size(size), _type(type), _usage(usage) {
}

gl_buffer::~gl_buffer() {
    DeleteGLgl_buffer(_id);
}

void gl_buffer::updateData(std::span<const std::uint8_t> data) {
    updateData(data.data(), data.size());
}

void gl_buffer::updateData(const void* data, std::size_t size) {
    asl::check(_size <= size, "gl_buffer data overflow");
    glBufferData(to_gl_enum(type()), size, data, to_gl_enum(usage()));
}

void gl_buffer::bind() const {
    glBindBuffer(to_gl_enum(_type), _id);
}

void gl_buffer::unbind() const {
    glBindBuffer(to_gl_enum(_type), _id);
}

gl_index_buffer::gl_index_buffer(std::uint32_t size, buffer_usage usage) : gl_buffer(buffer_type::index, usage, size) {
}

gl_index_buffer::gl_index_buffer(std::span<const std::uint32_t> indicies, buffer_usage usage) :
    gl_buffer(buffer_type::index, usage, indicies.data(), indicies.size_bytes()) {
}

void gl_index_buffer::updateData(std::span<const std::uint32_t> indicies) {
    gl_buffer::updateData(indicies.data(), indicies.size_bytes());
}

gl_vertex_buffer::gl_vertex_buffer(std::uint32_t size, const vertex_layout& layout, buffer_usage usage) :
    gl_buffer(buffer_type::vertex, usage, size), _layout(layout) {
}

gl_vertex_buffer::gl_vertex_buffer(std::span<const std::uint8_t> data,
    const vertex_layout& layout,
    buffer_usage usage) :
    gl_buffer(buffer_type::vertex, usage, data),
    _layout(layout) {
}

} // namespace cobalt
