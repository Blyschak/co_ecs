#pragma once

#include <cstddef>
#include <cstdint>

#include <cobalt/render/types.hpp>

namespace cobalt::renderer {

class gl_vertex_array {
public:
    gl_vertex_array();

    gl_vertex_array(const gl_vertex_array&) = delete;
    gl_vertex_array& operator=(const gl_vertex_array&) = delete;
    gl_vertex_array(gl_vertex_array&&) = delete;
    gl_vertex_array& operator=(gl_vertex_array&&) = delete;

    ~gl_vertex_array();

    void bind() const;
    void unbind() const;

    void push_back(const gl_vertex_buffer& buffer);

    [[nodiscard]] decltype(auto) size() const noexcept {
        return _size;
    }
    [[nodiscard]] decltype(auto) id() const noexcept {
        return vao;
    }

private:
    std::uint32_t vao{};
    std::size_t _size{};
};

} // namespace cobalt::renderer