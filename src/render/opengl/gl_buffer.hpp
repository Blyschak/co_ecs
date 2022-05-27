#pragma once


#include <cobalt/render/layout.hpp>
#include <cobalt/render/types.hpp>

#include <cstdint>
#include <span>

namespace cobalt {

class gl_buffer {
public:
    gl_buffer(buffer_type type, buffer_usage usage, std::uint32_t size);
    gl_buffer(buffer_type type, buffer_usage usage, std::span<const std::uint8_t> data);
    gl_buffer(buffer_type type, buffer_usage usage, const void* data, std::size_t size);

    gl_buffer(const gl_buffer&) = delete;
    gl_buffer& operator=(const gl_buffer&) = delete;
    gl_buffer(gl_buffer&&) = delete;
    gl_buffer& operator=(gl_buffer&&) = delete;
    ~gl_buffer();

    void bind() const;
    void unbind() const;

    [[nodiscard]] inline decltype(auto) id() const noexcept {
        return _id;
    }

    [[nodiscard]] inline decltype(auto) size() const noexcept {
        return _size;
    }

    [[nodiscard]] inline decltype(auto) type() const noexcept {
        return _type;
    }

    [[nodiscard]] inline decltype(auto) usage() const noexcept {
        return _usage;
    }

    void updateData(std::span<const std::uint8_t> data);
    void updateData(const void* data, std::size_t size);

private:
    std::uint32_t _id{};
    std::uint32_t _size{};
    buffer_type _type{ buffer_type::vertex };
    buffer_usage _usage{ buffer_usage::static_usage };
};

class gl_index_buffer : public gl_buffer {
public:
    gl_index_buffer(std::uint32_t size, buffer_usage usage = buffer_usage::static_usage);
    gl_index_buffer(std::span<const std::uint32_t> indicies, buffer_usage usage = buffer_usage::static_usage);

    void updateData(std::span<const std::uint32_t> data);

    [[nodiscard]] inline decltype(auto) count() const noexcept {
        return size() / sizeof(std::uint32_t);
    }

private:
    std::uint32_t _count{};
    std::uint32_t _id{};
};

class gl_vertex_buffer : public gl_buffer {
public:
    gl_vertex_buffer(std::uint32_t size, const vertex_layout& layout, buffer_usage usage = buffer_usage::static_usage);
    gl_vertex_buffer(std::span<const std::uint8_t> data,
        const vertex_layout& layout,
        buffer_usage usage = buffer_usage::static_usage);

    [[nodiscard]] const vertex_layout& layout() const noexcept {
        return _layout;
    }

private:
    std::uint32_t _id{};
    vertex_layout _layout;
};

} // namespace cobalt